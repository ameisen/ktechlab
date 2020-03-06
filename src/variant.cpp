/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <algorithm>
#include <cmath>

#include "colorcombo.h"
#include "cnitem.h"
#include <qdebug.h>
#include <klocalizedstring.h>
using namespace std;

// this value is taken from ColorCombo and should ideally be put somewhere...
static constexpr const char DefaultColor[] = "#f62a2a";

Variant::Variant(const QString &id, TypeValue type) :
	QObject(),
	m_id(id),
	m_type(type),
	m_colorScheme(ColorCombo::QtStandard)
{
	if (type == TypeValue::Color)
	{
		m_value = m_defaultValue = DefaultColor;
	}
}

Variant::Variant(QString &&id, TypeValue type) :
	QObject(),
	m_id(id),
	m_type(type),
	m_colorScheme(ColorCombo::QtStandard)
{
	if (type == TypeValue::Color)
	{
		m_value = m_defaultValue = DefaultColor;
	}
}

Variant::~Variant()
{
}

void Variant::appendAllowed(const QString &id, const QString &i18nName)
{
	m_allowed[id] = i18nName;
}

void Variant::appendAllowed(QString &&id, const QString &i18nName)
{
	m_allowed[id] = i18nName;
}

void Variant::appendAllowed(const QString &id, QString &&i18nName)
{
	m_allowed[id] = i18nName;
}

void Variant::appendAllowed(QString &&id, QString &&i18nName)
{
	m_allowed[id] = i18nName;
}

void Variant::setAllowed(const QStringList &allowed)
{
	m_allowed.clear();
	for (auto &&value : allowed) {
		m_allowed[value] = value;
	}
	/*
	QStringList::const_iterator end = allowed.end();
	for ( QStringList::const_iterator it = allowed.begin(); it != end; ++it )
		m_allowed[ *it ] = *it;
	*/
}

void Variant::appendAllowed(const QString &allowed)
{
	m_allowed[allowed] = allowed;
}

void Variant::appendAllowed(QString &&allowed)
{
	m_allowed[allowed] = allowed;
}

void Variant::setMinValue(double value)
{
	m_minValue = value;

	if (value != 0.0)
	{
		m_minAbsValue = std::min(m_minAbsValue, std::abs(value));
	}
}

void Variant::setMaxValue(double value)
{
	m_maxValue = value;

	if (value != 0.0)
	{
		m_minAbsValue = std::min(m_minAbsValue, std::abs(value));
	}
}

QString Variant::displayString() const
{
	switch(type())
	{
		case TypeValue::Double:
		{
			auto numValue = m_value.toDouble();
			return QString::number(numValue / CNItem::getMultiplier(numValue)) + " " + CNItem::getNumberMag(numValue) + m_unit;
		}

		case TypeValue::Int:
			return m_value.toString() + " " + m_unit;

		case TypeValue::Bool:
			return i18n(m_value.toBool() ? "True" : "False");

		case TypeValue::Select:
			return m_allowed[m_value.toString()];

		default:
			return m_value.toString();
	}
}

template <typename T>
void Variant::_setValue(T val)
{
	qDebug() << Q_FUNC_INFO << "val=" << val << " old=" << m_value;
	if (type() == TypeValue::Select && !m_allowed.contains(val.toString()))
	{
		// Our value is being set to an i18n name, not the actual string id.
		// So change val to the id (if it exists)

		auto i18nName = val.toString();

		// Range-for on QMap gives you values, not key-value pairs. Frustrating.
		QStringMap::iterator end = m_allowed.end();
		for (QStringMap::iterator it = m_allowed.begin(); it != end; ++it)
		{
			if (it.value() == i18nName)
			{
				val = it.key();
				break;
			}
		}
	}

	if (!m_bSetDefault)
	{
		m_defaultValue = val;
		m_bSetDefault = true;
	}

	if (m_value == val)
		return;

	const QVariant old = std::move(m_value);
	m_value = val;
	emit( valueChanged(val, old) );

	switch (type())
	{
		case TypeValue::String:
		case TypeValue::FileName:
		case TypeValue::PenCapStyle:
		case TypeValue::PenStyle:
		case TypeValue::Port:
		case TypeValue::Pin:
		case TypeValue::VarName:
		case TypeValue::Combo:
		case TypeValue::Select:
		case TypeValue::SevenSegment:
		case TypeValue::KeyPad:
		case TypeValue::Multiline:
		case TypeValue::RichText:
		{
			auto dispString = displayString();
			qDebug() << Q_FUNC_INFO << "dispString=" << dispString << " value=" << m_value;
			emit valueChanged(dispString);
			emit valueChangedStrAndTrue(dispString, true);
		}
		break;

		case TypeValue::Int:
			emit valueChanged(value().toInt());
			break;

		case TypeValue::Double:
			emit valueChanged(value().toDouble());
			break;

		case TypeValue::Color:
			emit valueChanged(value().value<QColor>());
			break;

		case TypeValue::Bool:
			emit valueChanged(value().toBool());
			break;

		case TypeValue::Raw:
		case TypeValue::None:
			break;
	}
	qDebug() << Q_FUNC_INFO << "result m_value=" << m_value;
}

void Variant::setValue(const QVariant &val)
{
	_setValue(val);
}

void Variant::setValue(QVariant &&val)
{
	_setValue(val);
}

void Variant::setMinAbsValue(double val)
{
	m_minAbsValue = val;
}


bool Variant::changed() const
{
	// Have to handle double slightly differently due inperfect storage of real numbers
	if (type() == TypeValue::Double)
	{
		double cur = value().toDouble();
		double def = defaultValue().toDouble();

		double diff = std::abs(cur - def);
		if (diff == 0) {
			return false;
		}

		// denom cannot be zero
		double denom = std::max(
			std::abs(cur),
			std::abs(def)
		);

		// not changed if within 1e-4% of each other's value
		return ( (diff / denom) > 1e-6 );
	}
	return value() != defaultValue();
}

#include "moc_variant.cpp"
