/***************************************************************************
 *   Copyright (C) 2003-2006 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "cells.h"
#include "utils.h"


//BEGIN class Cells
Cells::Cells(const QRect &canvasRect)
{
	init( canvasRect );
}


Cells::~Cells()
{
	auto count = m_cellsRect.width();
	if (m_cells) {
		for (decltype(count) i = 0; i < count; ++i)
			delete [] m_cells[i];
		delete [] m_cells;
	}
}


Cells::Cells(const Cells &c)
{
	init( QRect( c.cellsRect().topLeft() * 8, c.cellsRect().size() * 8 ) );

	if (!m_cells) return;

	auto w = m_cellsRect.width();
	auto h = m_cellsRect.height();

	for (decltype(w) i = 0; i < w; i++)
	{
		for (decltype(h) j = 0; j < h; j++)
		{
			if (m_cells[i]) {
				m_cells[i][j] = c.cell(i, j);
			}
		}
	}
}

Cells::Cells(Cells &&c) :
	m_cellsRect(c.m_cellsRect),
	m_cells(c.m_cells)
{
	c.m_cellsRect = {};
	c.m_cells = nullptr;
}

void Cells::init( const QRect & canvasRect )
{
	if (m_cells) {
		auto count = m_cellsRect.width();
		for (decltype(count) i = 0; i < count; ++i)
			delete [] m_cells[i];
		delete [] m_cells;
	}

	m_cellsRect = QRect( roundDown( canvasRect.topLeft(), 8 ), canvasRect.size()/8 ).normalized();

	auto w = m_cellsRect.width();
	auto h = m_cellsRect.height();

	using cellptr = Cell *;
	m_cells = new cellptr[w];
	for (decltype(w) i = 0; i < w; ++i)
	{
		m_cells[i] = new Cell[h];
	}
}


void Cells::reset()
{
	if (!m_cells) return;

	auto w = m_cellsRect.width();
	auto h = m_cellsRect.height();

	for (decltype(w) i = 0; i < w; i++)
	{
		for (decltype(h) j = 0; j < h; j++)
			m_cells[i][j].reset();
	}
}
//END class Cells
