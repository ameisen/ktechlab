/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pin.h"

#include <cassert>
#include <qdebug.h>

#include <qdebug.h>

#include <algorithm>

Pin::Pin( ECNode * parent )
{
	assert(parent);
	m_pECNode = parent;
	m_voltage = 0.;
	m_current = 0.;
	m_eqId = -2;
	m_bCurrentIsKnown = false;
	m_groundType = Pin::gt_never;
}


Pin::~Pin()
{
	for (auto &wire : m_inputWireList) {
		if (wire.isNull()) continue;
		delete (Wire *)wire;
	}

	for (auto &wire : m_outputWireList) {
		if (wire.isNull()) continue;
		delete (Wire *)wire;
	}
}


QPtrList<Pin> Pin::localConnectedPins( ) const
{
// 	qDebug() << Q_FUNC_INFO << "Input wires: "<<m_inputWireList.size()<<"   Output wires: " << m_outputWireList.size() << "   Switch connected: " << m_switchConnectedPins.size() << endl;

	QPtrList<Pin> pins;

	for (auto &wire : m_inputWireList) {
		if (wire.isNull() || !(Wire *)wire) continue;
		pins << wire->startPin();
	}

	for (auto &wire : m_outputWireList) {
		if (wire.isNull() || !(Wire *)wire) continue;
		pins << wire->endPin();
	}

	pins += m_switchConnectedPins;

	return pins;
}


void Pin::setSwitchConnected( Pin * pin, bool isConnected )
{
	if (!pin)
		return;

	if (isConnected)
	{
		if ( !m_switchConnectedPins.contains(pin) )
			m_switchConnectedPins.append(pin);
	}
	else
		m_switchConnectedPins.removeAll(pin);
}

void Pin::setSwitchCurrentsUnknown() {
    if (!m_switchList.empty()) {
        m_switchList.removeAt( 0l );
    } else {
        qDebug() << "Pin::setSwitchCurrentsUnknown - WARN - unexpected empty switch list";
    }
    m_unknownSwitchCurrents = m_switchList;
}

void Pin::addCircuitDependentPin( Pin * pin )
{
	if ( pin && !m_circuitDependentPins.contains(pin) )
		m_circuitDependentPins.append(pin);
}


void Pin::addGroundDependentPin( Pin * pin )
{
	if ( pin && !m_groundDependentPins.contains(pin) )
		m_groundDependentPins.append(pin);
}


void Pin::removeDependentPins()
{
	m_circuitDependentPins.clear();
	m_groundDependentPins.clear();
}


void Pin::addElement( Element * e )
{
	if ( !e || m_elementList.contains(e) )
		return;
	m_elementList.append(e);
}


void Pin::removeElement( Element * e )
{
	m_elementList.removeAll(e);
}


void Pin::addSwitch( Switch * sw )
{
	if ( !sw || m_switchList.contains( sw ) )
		return;
	m_switchList << sw;
}


void Pin::removeSwitch( Switch * sw )
{
	m_switchList.removeAll( sw );
}


void Pin::addInputWire( Wire * wire )
{
	if ( wire && !m_inputWireList.contains(wire) )
		m_inputWireList << wire;
}


void Pin::addOutputWire( Wire * wire )
{
	if ( wire && !m_outputWireList.contains(wire) )
		m_outputWireList << wire;
}


bool Pin::calculateCurrentFromWires()
{
	m_inputWireList.removeAll(nullptr);
	m_outputWireList.removeAll(nullptr);

	const auto &inputs = m_inputWireList;
	const auto &outputs = m_outputWireList;

	m_current = 0.0;

	// If a pin has no outputs, then it cannot have a circuituous connection and thus
	// cannot pass current
	if (outputs.isEmpty() || inputs.isEmpty()) {
		m_bCurrentIsKnown = false;
		return false;
	}

	double current = 0.0;

	for (auto &wire : inputs) {
		if (Ptr::isNull(wire)) continue;
		if (!wire->currentIsKnown()) {
			m_bCurrentIsKnown = false;
			return false;
		}

		current -= wire->current();
	}

	for (auto &wire : outputs) {
		if (Ptr::isNull(wire)) continue;
		if (!wire->currentIsKnown()) {
			m_bCurrentIsKnown = false;
			return false;
		}

		current += wire->current();
	}

	m_current = current;

	m_bCurrentIsKnown = true;
	return true;
}
