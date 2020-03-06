/***************************************************************************
 *   Copyright (C) 2002 Lucijan Busch <lucijan@gmx.at>                     *
 *   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>            *
 *   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>                     *
 *   Copyright (C) 2006 David Saxton <david@bluehaze.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "drawpart.h"
#include "propertyeditor.h"
#include "propertyeditoritem.h"

#include <qdebug.h>
#include <kiconloader.h>
#include <klocalizedstring.h>

#include <qcolor.h>
#include <qcursor.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qsize.h>


//BEGIN Class PropertyEditorItem
PropertyEditorItem::PropertyEditorItem(
	[[maybe_unused]] PropertyEditorItem *par,
	Property *property
) :
	QTableWidgetItem(property->editorCaption() /*, property->displayString() */),
	m_property(property)
{
}

PropertyEditorItem::PropertyEditorItem(
	QTableWidget *parent,
	const QString &text
) :
	QTableWidgetItem(text)
{
	setParent(parent);
	setText(text);

	setFlags(flags() & ~Qt::ItemIsSelectable);
}

PropertyEditorItem::PropertyEditorItem(
	QTableWidget *parent,
	QString &&text
) :
	QTableWidgetItem(text)
{
	setParent(parent);
	setText(text);

	setFlags(flags() & ~Qt::ItemIsSelectable);
}


void PropertyEditorItem::propertyValueChanged()
{
	setText(m_property ? m_property->displayString() : QString{});
}

PropertyEditorItem::~PropertyEditorItem()
{
}

void PropertyEditorItem::updateValue(bool alsoParent)
{
	QString text = m_property ? m_property->displayString() : QString{};
	qDebug() << Q_FUNC_INFO << "text= " << text;
	setText(text);
	if (alsoParent && QObject::parent()) {
		static_cast<PropertyEditorItem*>(QObject::parent())->updateValue();
	}
}

//END class PropertyEditorItem

#include "moc_propertyeditoritem.cpp"
