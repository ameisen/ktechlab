/***************************************************************************
 *   Copyright (C) 1999-2005 Trolltech AS                                  *
 *   Copyright (C) 2006 David Saxton <david@bluehaze.org>                  *
 *                                                                         *
 *   This file may be distributed and/or modified under the terms of the   *
 *   GNU General Public License version 2 as published by the Free         *
 *   Software Foundation                                                   *
 ***************************************************************************/

#pragma once

#include <qlist.h>

#include <map>

class KtlQCanvasItem;

using SortedCanvasItems = std::multimap<double, KtlQCanvasItem *>;

class KtlQCanvasItemList : public QList<KtlQCanvasItem *>
{
	public:
	void sort();
	KtlQCanvasItemList operator + (const KtlQCanvasItemList &l) const { return *this + l; }
};
