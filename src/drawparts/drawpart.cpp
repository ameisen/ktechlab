#include "itemdocument.h"
#include "itemdocumentdata.h"
#include "drawpart.h"
#include "variant.h"

#include <klocalizedstring.h>
#include <QBitArray>

#include <initializer_list>

DrawPart::DrawPart(ItemDocument *itemDocument, bool newItem, const char *id) :
	Super(itemDocument, newItem, id)
{
	if (itemDocument)
		itemDocument->registerItem(this);
}

DrawPart::~DrawPart() {}

Variant * DrawPart::createProperty(const QString &id, Variant::Type type) {
	const auto createStyleProperty = [this, id, type] (const std::initializer_list<QString> &styles) -> Variant & {
		auto &v = Item::createPropertyRef( id, Variant::Type::String);
		v.setType(type);
		v.setAllowed(QStringList{styles});
		return v;
	};

	switch (type) {
		case Variant::Type::PenStyle: {
			return &createStyleProperty({
				DrawPart::penStyleToName(Qt::SolidLine),
				DrawPart::penStyleToName(Qt::DashLine),
				DrawPart::penStyleToName(Qt::DotLine),
				DrawPart::penStyleToName(Qt::DashDotLine),
				DrawPart::penStyleToName(Qt::DashDotDotLine)
			});
		}
		case Variant::Type::PenCapStyle: {
			return &createStyleProperty({
				DrawPart::penCapStyleToName(Qt::FlatCap),
				DrawPart::penCapStyleToName(Qt::SquareCap),
				DrawPart::penCapStyleToName(Qt::RoundCap)
			});
		}
		default:
			return Item::createProperty( id, type );
	}
}

Qt::PenStyle DrawPart::getDataPenStyle(const QString &id) {
	return nameToPenStyle( dataString(id) );
}

Qt::PenCapStyle DrawPart::getDataPenCapStyle(const QString &id) {
	return nameToPenCapStyle( dataString(id) );
}

void DrawPart::setDataPenStyle(const QString &id, Qt::PenStyle value) {
	setDataPenStyle(property(id), value);
}

void DrawPart::setDataPenStyle(Variant *property, Qt::PenStyle value) {
	property->setValue( penStyleToName(value) );
}

void DrawPart::setDataPenStyle(Variant &property, Qt::PenStyle value) {
	setDataPenStyle(&property, value);
}

void DrawPart::setDataPenCapStyle(const QString &id, Qt::PenCapStyle value) {
	setDataPenCapStyle(property(id), value);
}

void DrawPart::setDataPenCapStyle(Variant *property, Qt::PenCapStyle value) {
	property->setValue( penCapStyleToName(value) );
}

void DrawPart::setDataPenCapStyle(Variant &property, Qt::PenCapStyle value) {
	setDataPenCapStyle(&property, value);
}

ItemData DrawPart::itemData() const {
	auto itemData = Super::itemData();

	// QMap, unlike the stdlib meow_map, does not have the iterator dereference return a pair.
	// It, instead, returns the value, which prevents one from using a range-for loop to get both.
	const auto end = m_variantData.end();
	for (auto it = m_variantData.begin(); it != end; ++it) {
		switch(it.value()->type()) {
			case Variant::Type::PenStyle:
				itemData.dataString[it.key()] = penStyleToID( nameToPenStyle( it.value()->value().toString() ) );
				break;
			case Variant::Type::PenCapStyle:
				itemData.dataString[it.key()] = penCapStyleToID( nameToPenCapStyle( it.value()->value().toString() ) );
				break;
			default:
				// The rest are handled by Item
				break;
		}
	}

	return itemData;
}

void DrawPart::restoreFromItemData(const ItemData &itemData) {
	Super::restoreFromItemData(itemData);

	const auto stringEnd = itemData.dataString.end();
	for (auto it = itemData.dataString.begin(); it != stringEnd; ++it) {
		const auto &key = it.key();

		const auto vit = m_variantData.find(key);
		if (vit == m_variantData.end())
			continue;

		switch (vit.value()->type()) {
			case Variant::Type::PenStyle:
				setDataPenStyle(
					key,
					idToPenStyle(it.value())
				);
				break;
			case Variant::Type::PenCapStyle:
				setDataPenCapStyle(
					key,
					idToPenCapStyle(it.value())
				);
				break;
			default:
				break;
		}
	}
}

QString DrawPart::penStyleToID(Qt::PenStyle style) {
	// TODO : Static initialize the QStrings to limit allocations
	switch (style) {
		case Qt::SolidLine:
			return "SolidLine";
		case Qt::NoPen:
			return "NoPen";
		case Qt::DashLine:
			return "DashLine";
		case Qt::DotLine:
			return "DotLine";
		case Qt::DashDotLine:
			return "DashDotLine";
		case Qt::DashDotDotLine:
			return "DashDotDotLine";
		case Qt::MPenStyle:
		default:
			return ""; // ?!
	}
}

Qt::PenStyle DrawPart::idToPenStyle(const QString &id) {
	// TODO : Replace with a constexpr hash-switch
	if (id == "NoPen")
		return Qt::NoPen;
	if (id == "DashLine")
		return Qt::DashLine;
	if (id == "DotLine")
		return Qt::DotLine;
	if (id == "DashDotLine")
		return Qt::DashDotLine;
	if (id == "DashDotDotLine")
		return Qt::DashDotDotLine;
	return Qt::SolidLine;
}

QString DrawPart::penCapStyleToID(Qt::PenCapStyle style) {
	// TODO : Static initialize the QStrings to limit allocations
	switch (style) {
		case Qt::FlatCap:
			return "FlatCap";
		case Qt::SquareCap:
			return "SquareCap";
		case Qt::RoundCap:
			return "RoundCap";
		case Qt::MPenCapStyle:
		default:
			return ""; // ?!
	}
}

Qt::PenCapStyle DrawPart::idToPenCapStyle(const QString &id) {
	// TODO : Replace with a constexpr hash-switch
	if (id == "SquareCap")
		return Qt::SquareCap;
	if (id == "RoundCap")
		return Qt::RoundCap;
	return Qt::FlatCap;
}

QString DrawPart::penStyleToName(Qt::PenStyle style) {
	switch (style) {
		case Qt::SolidLine:
			return i18n("Solid");
		case Qt::NoPen:
			return i18n("None");
		case Qt::DashLine:
			return i18n("Dash");
		case Qt::DotLine:
			return i18n("Dot");
		case Qt::DashDotLine:
			return i18n("Dash Dot");
		case Qt::DashDotDotLine:
			return i18n("Dash Dot Dot");
		case Qt::MPenStyle:
		default:
			return ""; // ?!
	}
}

Qt::PenStyle DrawPart::nameToPenStyle(const QString &name) {
	if (name == i18n("None"))
		return Qt::NoPen;
	if (name == i18n("Dash"))
		return Qt::DashLine;
	if (name == i18n("Dot"))
		return Qt::DotLine;
	if (name == i18n("Dash Dot"))
		return Qt::DashDotLine;
	if (name == i18n("Dash Dot Dot"))
		return Qt::DashDotDotLine;
	return Qt::SolidLine;
}

QString DrawPart::penCapStyleToName(Qt::PenCapStyle style) {
	switch (style) {
		case Qt::FlatCap:
			return i18n("Flat");
		case Qt::SquareCap:
			return i18n("Square");
		case Qt::RoundCap:
			return i18n("Round");
		case Qt::MPenCapStyle:
		default:
			return ""; // ?!
	}
}

Qt::PenCapStyle DrawPart::nameToPenCapStyle(const QString &name) {
	if ( name == i18n("Square") )
		return Qt::SquareCap;
	if ( name == i18n("Round") )
		return Qt::RoundCap;
	return Qt::FlatCap;
}
