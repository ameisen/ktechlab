#pragma once

#include "drawpart.h"

/**
@short Represents editable text on the canvas
@author David Saxton
*/
class DPText final : public DrawPart {
	Q_OBJECT

	public:
		DPText( ItemDocument *itemDocument, bool newItem, const char *id );
		~DPText() override;

		static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
		static LibraryItem *libraryItem();
		static LibraryItem *libraryItemOld();

		void setSelected( bool selected ) override;

		QSize minimumSize() const override;

	protected:
		void postResize() override;

	private:
		void drawShape( QPainter &p ) override;
		void dataChanged() override;
		QString m_text;
		QColor m_backgroundColor;
		QColor m_frameColor;
		RectangularOverlay * const m_rectangularOverlay = nullptr;
		Property &m_Text;
		Property &m_Background;
		Property &m_BackgroundColor;
		Property &m_FrameColor;
		bool b_displayBackground = false;
};
