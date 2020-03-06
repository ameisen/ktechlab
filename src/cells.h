/***************************************************************************
 *   Copyright (C) 2003-2006 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#pragma once

#include <limits>
#include <cassert>
#include <map>
#include <qrect.h>
#include "utils.h"

static constexpr const short startCellPos = -(1 << 14);

struct Point
{
	short x = startCellPos;
	short y = startCellPos;
	short prevX = startCellPos;
	short prevY = startCellPos;
};

// Key = cell, data = previous cell, compare = score
using TempLabelMap = std::multimap<unsigned short, Point>;

/**
@short Used for mapping out connections
*/
struct Cell
{
	using score_t = unsigned short;
	static constexpr const auto max_score = std::numeric_limits<score_t>::max();

	/**
	 * Resets bestScore, prevX, prevY, addedToLabels, it, permanent for
	 * each cell.
	 */
	void reset() {
		addedToLabels = Cell{}.addedToLabels;
		permanent = Cell{}.permanent;
		bestScore = Cell{}.bestScore;
	}

	/**
	 * Pointer to the point in the TempLabelMap.
	 */
	Point *point = nullptr;
	/**
	 * 'Penalty' of using the cell from CNItem.
	 */
	score_t CIpenalty = 0;
	/**
	 * 'Penalty' of using the cell from Connector.
	 */
	score_t Cpenalty = 0;
	/**
	 * Best (lowest) score so far, _the_ best if it is permanent.
	 */
	score_t bestScore = max_score;
	/**
	 * Number of connectors through that point.
	 */
	unsigned short numCon = 0;
	/**
	 * Which cell this came from, (startCellPos,startCellPos) if originating
	 * cell.
	 */
	short prevX, prevY;
	/**
	 * Whether the score can be improved on.
	 */
	bool permanent = false;
	/**
	 * Whether the cell has already been added to the list of cells to
	 * check.
	 */
	bool addedToLabels = false;
};

/**
@author David Saxton
*/
class Cells
{
	public:
		Cells( const QRect &canvasRect );
		Cells(Cells &&);
		~Cells();
		/**
		 * Resets bestScore, prevX, prevY, addedToLabels, it, permanent for each cell
	 	*/
		void reset();

		const QRect & cellsRect() const { return m_cellsRect; }

		/**
		 * Returns the cell containing the given position on the canvas.
		 */
		Cell & cellContaining( int x, int y ) const
		{
			return cell(
				roundDown( x, 8 ),
				roundDown( y, 8 )
			);
		}
		/**
		 * @return if the given cell exists.
		 */
		bool haveCell( int i, int j ) const
		{
			if ( (i < m_cellsRect.left()) || (i >= m_cellsRect.right()) )
				return false;

			if ( (j < m_cellsRect.top()) || (j >= m_cellsRect.bottom()) )
				return false;

			return true;
		}
		/**
		 * @return if there is a cell containg the given canvas point.
		 */
		bool haveCellContaing( int x, int y ) const
		{
			return haveCell(
				roundDown( x, 8 ),
				roundDown( y, 8 )
			);
		}

		Cell & cell( int i, int j ) const
		{
			assert( i < m_cellsRect.right() );
			assert( j < m_cellsRect.bottom() );
			i -= m_cellsRect.left();
			j -= m_cellsRect.top();
			return m_cells[i][j];
		}

	protected:
		void init(const QRect &canvasRect);

		QRect m_cellsRect;
		Cell **m_cells = nullptr;

	private:
		Cells(const Cells &);
		Cells & operator = (const Cells &) = delete;
		Cells & operator = (Cells &&) = delete;
};
