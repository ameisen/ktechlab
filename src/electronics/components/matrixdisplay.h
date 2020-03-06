/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MATRIXDISPLAY_H
#define MATRIXDISPLAY_H

#include <component.h>
// #include <q3valuevector.h>

const unsigned max_md_width = 100;
const unsigned max_md_height = 20;

/**
@author David Saxton
*/
class MatrixDisplay final : public Component
{
	public:
		MatrixDisplay( ICNDocument *icnDocument, bool newItem, const char *id = 0L );
		~MatrixDisplay() override;

		static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
		static LibraryItem *libraryItem();

		void stepNonLogic() override;
		bool doesStepNonLogic() const override { return true; }

	protected:
		void drawShape( QPainter &p ) override;
		void dataChanged() override;

		void initPins( unsigned numRows, unsigned numCols );
		QString colPinID( int col ) const;
		QString rowPinID( int row ) const;


		QVector< QVector<double> > m_avgBrightness;
		QVector< QVector<unsigned> > m_lastBrightness;
		QVector< QVector<Diode*> > m_pDiodes;

		ECNode * m_pRowNodes[max_md_height];
		ECNode * m_pColNodes[max_md_width];

		double m_lastUpdatePeriod;

		double m_r, m_g, m_b;
		bool m_bRowCathode;

		unsigned m_numRows;
		unsigned m_numCols;
};

#endif
