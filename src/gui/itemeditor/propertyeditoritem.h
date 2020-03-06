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

#pragma once

#include "property.h"

#include <qtablewidget.h>

/** This class is a subclass of K3ListViewItem which is associated to a property.
    It also takes care of drawing custom contents.
 **/
 //! An item in PropertyEditorItem associated to a property
class PropertyEditorItem : public QObject, public QTableWidgetItem
{
	Q_OBJECT

	public:
	/**
	 * Creates a PropertyEditorItem child of \a parent, associated to
	 * \a property. Within property editor, items are created in
	 * PropertyEditor::fill(), every time the buffer is updated. It
	 * \a property has not desctiption set, its name (i.e. not i18n'ed) is
	 * reused.
	 */
	PropertyEditorItem(PropertyEditorItem *parent, Property *property);

	/**
	 * Creates PropertyEditor Top Item which is necessary for drawing all
	 * branches.
	 */
	PropertyEditorItem(QTableWidget *parent, const QString &text);
	PropertyEditorItem(QTableWidget *parent, QString &&text);

	~PropertyEditorItem() override;

	/**
	 * \return property's name.
	 */
	QString name() const { return m_property ? m_property->id() : QString{}; }
	/**
	 * \return properties's type.
	 */
	Variant::TypeValue type() { return m_property ? m_property->type() : Variant::TypeValue::None; }
	/**
	 * \return a pointer to the property associated to this item.
	 */
	Property * property() { return m_property; }
	/**
	 * Updates text on of this item, for current property value. If
	 * \a alsoParent is true, parent item (if present) is also updated.
	 */
	virtual void updateValue(bool alsoParent = true);

	protected slots:
	virtual void propertyValueChanged();

	private:
	Property *m_property = nullptr;
};
