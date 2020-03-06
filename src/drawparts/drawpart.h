#pragma once

#include <item.h>

class ICNDocument;
class ItemDocument;
class LibraryItem;
class RectangularOverlay;

// Note: values are used for popup menu
enum class DrawAction : int {
	invalid = -1,
	text = 0,
	line,
	arrow,
	rectangle,
	ellipse,
	image
};

/**
@author David Saxton
*/
class DrawPart : public Item {
	using Super = Item;

	public:
		DrawPart( ItemDocument *itemDocument, bool newItem, const char *id );
		~DrawPart() override;

		bool canResize() const override { return true; }

		Variant * createProperty( const QString &id, Variant::TypeValue type ) override;

		Qt::PenStyle getDataPenStyle( const QString &id );
		Qt::PenCapStyle getDataPenCapStyle( const QString &id );

		void setDataPenStyle( const QString &id, Qt::PenStyle value );
		void setDataPenStyle( Variant *property, Qt::PenStyle value );
		void setDataPenStyle( Variant &property, Qt::PenStyle value );
		void setDataPenCapStyle( const QString &id, Qt::PenCapStyle value );
		void setDataPenCapStyle( Variant *property, Qt::PenCapStyle value );
		void setDataPenCapStyle( Variant &property, Qt::PenCapStyle value );

		ItemData itemData() const override;
		void restoreFromItemData( const ItemData &itemData ) override;

		// Convention for following functions: name is i18n'd name of style, id is the english one

		static QString penStyleToID( Qt::PenStyle style );
		static Qt::PenStyle idToPenStyle( const QString &id );
		static QString penCapStyleToID( Qt::PenCapStyle style );
		static Qt::PenCapStyle idToPenCapStyle( const QString &id );

		static QString penStyleToName( Qt::PenStyle style );
		static Qt::PenStyle nameToPenStyle( const QString &name );
		static QString penCapStyleToName( Qt::PenCapStyle style );
		static Qt::PenCapStyle nameToPenCapStyle( const QString &name );
};
