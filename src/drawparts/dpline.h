#pragma once

#include "drawpart.h"

class LineOverlay;

/**
@author David Saxton
*/
class DPLine : public DrawPart {
	using Super = DrawPart;
	Q_OBJECT

	public:
		DPLine( ItemDocument *itemDocument, bool newItem, const char *id = nullptr );
		~DPLine() override;

		static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
		static LibraryItem *libraryItem();

		void setSelected( bool selected ) override;

	protected:
		void postResize() override;
		void dataChanged() override;
		void drawShape( QPainter &p ) override;

		LineOverlay * const m_pLineOverlay = nullptr;
		Property &m_LineColor;
		Property &m_LineWidth;
		Property &m_LineStyle;
		Property &m_CapStyle;
};

/**
@author David Saxton
*/
class DPArrow final : public DPLine {
	using Super = DPLine;
	public:
		DPArrow( ItemDocument *itemDocument, bool newItem, const char *id = nullptr );
		~DPArrow() override;

		static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
		static LibraryItem *libraryItem();

	protected:
		void dataChanged() override;
		void drawShape( QPainter &p ) override;

		Property &m_HeadAngle;
		double m_headAngle = 20.0;
};
