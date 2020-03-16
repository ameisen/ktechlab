#include <algorithm>
#include <cmath>

#include "colorcombo.h"
#include "cnitem.h"

#include <QDebug>
#include <KLocalizedString>

using namespace std;

// this value is taken from ColorCombo and should ideally be put somewhere...
static constexpr const char DefaultColor[] = "#f62a2a";

Variant::Variant(const QString &id, Type type) :
	Super(),
	m_id(id),
	m_type(type),
	m_colorScheme(ColorCombo::QtStandard)
{
	if (type == Type::Color) {
		m_value = m_defaultValue = DefaultColor;
	}
}

Variant::Variant(QString &&id, Type type) : Variant(id, type)
{}

void Variant::appendAllowed(const QString &id, const QString &i18nName) {
	m_allowed[id] = i18nName;
}

void Variant::appendAllowed(QString &&id, const QString &i18nName) {
	m_allowed[id] = i18nName;
}

void Variant::appendAllowed(const QString &id, QString &&i18nName) {
	m_allowed[id] = i18nName;
}

void Variant::appendAllowed(QString &&id, QString &&i18nName) {
	m_allowed[id] = i18nName;
}

void Variant::setAllowed(const QStringList &allowed) {
	m_allowed.clear();
	for (auto &&value : allowed) {
		m_allowed[value] = value;
	}
}

void Variant::appendAllowed(const QString &allowed) {
	m_allowed[allowed] = allowed;
}

void Variant::appendAllowed(QString &&allowed) {
	m_allowed[allowed] = allowed;
}

void Variant::setMinValue(double value) {
	m_minValue = value;

	if (value != 0.0) {
		m_minAbsValue = std::min(m_minAbsValue, std::abs(value));
	}
}

void Variant::setMaxValue(double value) {
	m_maxValue = value;

	if (value != 0.0) {
		m_minAbsValue = std::min(m_minAbsValue, std::abs(value));
	}
}

QString Variant::displayString() const {
	switch(type()) {
		case Type::Double: {
			auto numValue = m_value.toDouble();
			return QString::number(numValue / CNItem::getMultiplier(numValue)) + " " + CNItem::getNumberMag(numValue) + m_unit;
		}

		case Type::Int:
			return m_value.toString() + " " + m_unit;

		case Type::Bool:
			return i18n(m_value.toBool() ? "True" : "False");

		case Type::Select:
			return m_allowed[m_value.toString()];

		default:
			return m_value.toString();
	}
}

template <typename T>
void Variant::_setValue(T val) {
	qDebug() << Q_FUNC_INFO << "val=" << val << " old=" << m_value;
	if (type() == Type::Select && !m_allowed.contains(val.toString())) {
		// Our value is being set to an i18n name, not the actual string id.
		// So change val to the id (if it exists)

		auto i18nName = val.toString();

		// Range-for on QMap gives you values, not key-value pairs. Frustrating.
		const auto end = m_allowed.end();
		for (auto it = m_allowed.begin(); it != end; ++it) {
			if (it.value() == i18nName) {
				val = it.key();
				break;
			}
		}
	}

	if (!m_bSetDefault) {
		m_defaultValue = val;
		m_bSetDefault = true;
	}

	if (m_value == val) {
		return;
	}

	const QVariant old = std::move(m_value);
	m_value = val;
	emit(valueChanged(val, old));

	switch (type()) {
		case Type::String:
		case Type::FileName:
		case Type::PenCapStyle:
		case Type::PenStyle:
		case Type::Port:
		case Type::Pin:
		case Type::VarName:
		case Type::Combo:
		case Type::Select:
		case Type::SevenSegment:
		case Type::KeyPad:
		case Type::Multiline:
		case Type::RichText: {
			auto dispString = displayString();
			qDebug() << Q_FUNC_INFO << "dispString=" << dispString << " value=" << m_value;
			emit valueChanged(dispString);
			emit valueChangedStrAndTrue(dispString, true);
		}
		break;

		case Type::Int:
			emit valueChanged(value().toInt());
			break;

		case Type::Double:
			emit valueChanged(value().toDouble());
			break;

		case Type::Color:
			emit valueChanged(value().value<QColor>());
			break;

		case Type::Bool:
			emit valueChanged(value().toBool());
			break;

		case Type::Raw:
		case Type::None:
			break;
	}
	qDebug() << Q_FUNC_INFO << "result m_value=" << m_value;
}

void Variant::setValue(const QVariant &val) {
	_setValue(val);
}

void Variant::setValue(QVariant &&val) {
	_setValue(val);
}

void Variant::setMinAbsValue(double val) {
	m_minAbsValue = val;
}

bool Variant::changed() const {
	// Have to handle double slightly differently due inperfect storage of real numbers
	if (type() == Type::Double) {
		const double cur = value().toDouble();
		const double def = defaultValue().toDouble();

		const double diff = std::abs(cur - def);
		if (diff == 0.0) {
			return false;
		}

		// denom cannot be zero
		const double denom = std::max(
			std::abs(cur),
			std::abs(def)
		);

		// not changed if within 1e-4% of each other's value
		return (diff / denom) > 1.0e-6;
	}
	return value() != defaultValue();
}

#include "moc_variant.cpp"
