/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ECSEVENSEGMENT_H
#define ECSEVENSEGMENT_H

#include "component.h"

class Diode;
class ECNode;

/**
@short Seven segment display component
@author David Saxton
*/
class ECSevenSegment final : public Component
{
public:
	ECSevenSegment( ICNDocument *icnDocument, bool newItem, const char *id = 0L );
	~ECSevenSegment() override;

	static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
	static LibraryItem *libraryItem();

	void stepNonLogic() override;
	bool doesStepNonLogic() const override { return true; }
	void dataChanged() override;

private:
	void drawShape( QPainter &p ) override;

	bool m_bCommonCathode;
	double lastUpdatePeriod;
	double avg_brightness[8];
	uint last_brightness[8];
	Diode *m_diodes[8];
	ECNode *m_nodes[8];
	ECNode *m_nNode;
	double r, g, b;
};

#endif
