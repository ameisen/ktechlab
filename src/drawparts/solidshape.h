#pragma once

#include <drawpart.h>

class RectangularOverlay;

/**
@short Represents a drawable rectangle on the canvas
@author David Saxton
*/
class DPRectangle : public DrawPart {
	using Super = DrawPart;
	public:
		DPRectangle( ItemDocument *itemDocument, bool newItem, const char *id = nullptr );
		~DPRectangle() override;

		static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
		static LibraryItem *libraryItem();

		void setSelected( bool selected ) override;

		QSize minimumSize() const override;

	protected:
		void drawShape( QPainter &p ) override;
		void dataChanged() override;
		void postResize() override;

		/** Returns the rectangle to draw in, taking into account the line
		  * width */
		QRect drawRect() const;

	private:
		RectangularOverlay * const m_pRectangularOverlay = nullptr;
		Property &m_Background;
		Property &m_BackgroundColor;
		Property &m_LineColor;
		Property &m_LineWidth;
		Property &m_LineStyle;
};

/**
@short Represents a drawable rectangle on the canvas
@author David Saxton
*/
class DPEllipse final : public DPRectangle {
	using Super = DPRectangle;
	public:
		DPEllipse( ItemDocument *itemDocument, bool newItem, const char *id = nullptr );
		~DPEllipse() override;

		static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
		static LibraryItem *libraryItem();

	protected:
		void postResize() override;

	private:
		void drawShape( QPainter &p ) override;
};
