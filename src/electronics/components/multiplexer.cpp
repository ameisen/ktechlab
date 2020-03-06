/***************************************************************************
 *   Copyright (C) 2004-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "multiplexer.h"

#include "logic.h"
#include "libraryitem.h"

#include <kiconloader.h>
#include <klocalizedstring.h>

#include <cmath>
#include <algorithm>

Item* Multiplexer::construct( ItemDocument *itemDocument, bool newItem, const char *id )
{
	return new Multiplexer( (ICNDocument*)itemDocument, newItem, id );
}

LibraryItem* Multiplexer::libraryItem()
{
	return new LibraryItem(
		QStringList(QString("ec/multiplexer")),
		i18n("Multiplexer"),
		i18n("Integrated Circuits"),
		"ic1.png",
		LibraryItem::lit_component,
		Multiplexer::construct
	);
}

Multiplexer::Multiplexer( ICNDocument *icnDocument, bool newItem, const char *id ) :
	Component( icnDocument, newItem, id ? id : "multiplexer" ),
	m_output(nullptr)
{
	m_name = i18n("Multiplexer");

	auto *addressSize = createProperty( "addressSize", Variant::Type::Int );
	addressSize->setCaption( i18n("Address Size") );
	addressSize->setMinValue(1);
	addressSize->setMaxValue(8);
	addressSize->setValue(1);

	// For backwards compatibility
	auto *numInput = createProperty( "numInput", Variant::Type::Int );
	numInput->setMinValue(-1);
	numInput->setValue(-1);
	numInput->setHidden(true);
}

Multiplexer::~Multiplexer()
{
}


void Multiplexer::dataChanged()
{
	Variant * const numInput = hasProperty("numInput") ? property("numInput") : nullptr;
	if (numInput) {
		const int numInputInt = numInput->value().toInt();
		if ( numInputInt != -1 )
		{
			const int addressSize = std::clamp(
				int(std::ceil( std::log( double(numInputInt) ) / std::log(2.0))),
				1,
				8
			);
			numInput->setValue(-1);

			// This function will get called again when we set the value of numInput
			property("addressSize")->setValue(addressSize);
			return;
		}

		m_variantData["numInput"]->deleteLater();
		m_variantData.remove("numInput");
	}

	initPins( dataInt("addressSize") );
}


void Multiplexer::inStateChanged( [[maybe_unused]] bool state )
{
	using position_t = llong;
	constexpr int max_bits = sizeof(position_t) * 8;
	position_t pos = 0;
	auto size = m_aLogic.size();
	if (size > max_bits) {
		//qWarning() << Q_FUNC_INFO << " m_aLogic.size() is greater than max bits = " << size << endl;
		size = max_bits;
	}
	for ( int i = 0; i < size; ++i )
	{
		if ( m_aLogic[i]->isHigh() )
			pos |= 1 << i;
	}
	m_output->setHigh( m_xLogic[pos]->isHigh() );
}

// TODO : This function is just all sorts of broken.
void Multiplexer::initPins( int newAddressSize )
{
	using count_t = llong;
	constexpr int max_bits = (sizeof(count_t) * CHAR_BIT) - 1;

	int oldAddressSize = m_aLogic.size();

	if (oldAddressSize > max_bits) {
		//qWarning() << Q_FUNC_INFO << " oldAddressSize is greater than max bits = " << size << endl;
		oldAddressSize = max_bits;
	}

	count_t oldXLogicCount = m_xLogic.size();
	count_t newXLogicCount = count_t(1) << newAddressSize;

	if ( newXLogicCount == oldXLogicCount )
		return;

	QStringList pins;

	const llong length = newAddressSize + newXLogicCount;

	for ( llong i=0; i<newAddressSize; ++i )
		pins += "A"+QString::number(i);
	for ( llong i=0; i<newXLogicCount; ++i )
		pins += "X"+QString::number(i);
	for ( llong i=0; i<(length-(length%2))/2; ++i )
		pins += "";
	pins += "X";
	for ( llong i=0; i<((length+(length%2))/2)-1; ++i )
		pins += "";

	initDIPSymbol( pins, 64 );
	initDIP(pins);

	if (!m_output)
	{
		ECNode *node =  ecNodeWithID("X");
		m_output = createLogicOut( node, false );
	}

	if ( newXLogicCount > oldXLogicCount )
	{
		m_xLogic.resize(newXLogicCount);
		for ( llong i=oldXLogicCount; i<newXLogicCount; ++i )
		{
			ECNode *node = ecNodeWithID("X"+QString::number(i));
			m_xLogic.insert( i, createLogicIn(node) );
			m_xLogic[i]->setCallback( this, (CallbackPtr)(&Multiplexer::inStateChanged) );
		}

		m_aLogic.resize(newAddressSize);
		for ( llong i=oldAddressSize; i<newAddressSize; ++i )
		{
			ECNode *node = ecNodeWithID("A"+QString::number(i));
			m_aLogic.insert( i, createLogicIn(node) );
			m_aLogic[i]->setCallback( this, (CallbackPtr)(&Multiplexer::inStateChanged) );
		}
	}
	else
	{
		for ( llong i = newXLogicCount; i < oldXLogicCount; ++i )
		{
			QString id = "X"+QString::number(i);
			removeDisplayText(id);
			removeElement( m_xLogic[i], false );
			removeNode(id);
		}
		m_xLogic.resize(newXLogicCount);

		for ( llong i = newAddressSize; i < oldAddressSize; ++i )
		{
			QString id = "A"+QString::number(i);
			removeDisplayText(id);
			removeElement( m_aLogic[i], false );
			removeNode(id);
		}
		m_aLogic.resize(newAddressSize);
	}
}
