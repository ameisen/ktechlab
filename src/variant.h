/***************************************************************************
 *   Copyright (C) 2003-2004 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#pragma once

#include "pch.hpp"

#include <utility>

#include <qobject.h>
#include <qvariant.h>
#include <qstringlist.h>

/// \todo Replace "Variant" with "Property"
class Variant;
using Property = Variant;

class QColor;
class QString;

using QStringMap = QMap<QString, QString>;

/**
For information:
QVariant::type() returns an enum for the current data type
contained. e.g. returns QVariant::Color or QVariant::Rect
@author Daniel Clarke
@author David Saxton
*/
class Variant : public QObject
{
	Q_OBJECT
	public:
	class Type
	{
		public:
		enum Value : unsigned int
		{
			None,
			Int,					// Integer
			Raw,					// QByteArray
			Double,				// Real number
			String,				// Editable string
			Multiline,		// String that may contain linebreaks
			RichText,			// HTML formatted text
			Select,				// Selection of strings
			Combo,				// Editable combination of strings
			FileName,			// Filename
			Color,				// Color
			Bool,					// Boolean
			VarName,			// Variable name
			Port,					// Port name
			Pin,					// Pin name
			PenStyle,			// Pen Style
			PenCapStyle,	// Pen Cap Style
			SevenSegment,	// Pin Map for Seven Segment Display
			KeyPad				// Pin Map for Keypad
		};
	};
	using TypeValue = Type::Value;

	Variant(const QString &id, TypeValue type);
	Variant(QString &&id, TypeValue type);
	~Variant() override;

	const QString & id() const { return m_id; }

	/**
	 * Returns the type of Variant (see Variant::TypeValue)
	 */
	TypeValue type() const { return m_type; }
	/**
	 * Sets the variant type
	 */
	void setType(TypeValue type) { m_type = type; }
	/**
	 * Returns the filter used for file dialogs (if this is of type Type::FileName)
	 */
	const QString & filter() const { return m_filter; }
	void setFilter(const QString &filter) { m_filter = filter; }
	/**
	 * The selection of colours to be used in the combo box - e.g.
	 * ColorCombo::LED.
	 * @see ColorCombo::ColorScheme
	 */
	int colorScheme() const { return m_colorScheme; }
	void setColorScheme(int colorScheme) { m_colorScheme = colorScheme; }
	/**
	 * This function is for convenience; it sets both the toolbar and editor
	 * caption.
	 */
	void setCaption(const QString &caption)
	{
		setToolbarCaption(caption);
		setEditorCaption(caption);
	}
	/**
	 * This text is displayed to the left of the entry widget in the toolbar
	 */
	const QString & toolbarCaption() const { return m_toolbarCaption; }
	void setToolbarCaption(const QString &caption) { m_toolbarCaption = caption; }
	void setToolbarCaption(QString &&caption) { m_toolbarCaption = caption; }
	/**
	 * This text is displayed to the left of the entry widget in the item editor
	 */
	const QString & editorCaption() const { return m_editorCaption; }
	void setEditorCaption(const QString &caption) { m_editorCaption = caption; }
	void setEditorCaption(QString &&caption) { m_editorCaption = caption; }
	/**
	 * Unit of number, (e.g. V (volts) / F (farads))
	 */
	const QString & unit() const { return m_unit; }
	void setUnit(const QString &unit) { m_unit = unit; }
	void setUnit(QString &&unit) { m_unit = unit; }
	/**
	 * The smallest (as in negative, not absoluteness) value that the user can
	 * set this to.
	 */
	double minValue() const { return m_minValue; }
	void setMinValue(double value);
	/**
	 * The largest (as in positive, not absoluteness) value that the user can
	 * set this to.
	 */
	double maxValue() const { return m_maxValue; }
	void setMaxValue(double value);
	/**
	 * The smallest absolute value that the user can set this to, before the
	 * value is considered zero.
	 */
	double minAbsValue() const { return m_minAbsValue; }
	void setMinAbsValue(double val);
	const QVariant & defaultValue() const { return m_defaultValue; }
	/**
	 * If this data is marked as advanced, it will only display in the item
	 * editor (and not in the toolbar)
	 */
	bool isAdvanced() const { return m_bAdvanced; }
	void setAdvanced(bool advanced) { m_bAdvanced = advanced; }
	/**
	 * If this data is marked as hidden, it will not be editable from anywhere
	 * in the user interface
	 */
	bool isHidden() const { return m_bHidden; }
	void setHidden(bool hidden) { m_bHidden = hidden; }
	/**
	 * Returns the best possible attempt at representing the data in a string
	 * for display. Used by the properties list view.
	 */
	QString displayString() const;
	/**
	 * The list of values that the data is allowed to take (if it is string)
	 * that is displayed to the user.
	 */
	QStringList allowed() const { return m_allowed.values(); }
	/**
	 * @param allowed A list of pairs of (id, i18n-name) of allowed values.
	 */
	void setAllowed(const QStringMap &allowed) { m_allowed = allowed; }
	void setAllowed(QStringMap &&allowed) { m_allowed = allowed; }
	void setAllowed(const QStringList &allowed);
	void appendAllowed(const QString &id, const QString &i18nName);
	void appendAllowed(QString &&id, const QString &i18nName);
	void appendAllowed(const QString &id, QString &&i18nName);
	void appendAllowed(QString &&id, QString &&i18nName);
	void appendAllowed(const QString &allowed);
	void appendAllowed(QString &&allowed);
	/**
	 * @return whether the current value is different to the default value.
	 */
	bool changed() const;
	const QVariant & value() const { return m_value; }
	template <typename T>
	T get() const { return m_value.value<T>(); }
	void setValue(const QVariant &val);
	void setValue(QVariant &&val);

	private:
	template <typename T>
	void _setValue(T val);
	public:

	signals:
	/**
	 * Emitted when the value changes.
	 * NOTE: The order of data given is the new value, and then the old value
	 * This is done so that slots that don't care about the old value don't
	 * have to accept it
	 */
	void valueChanged(const QVariant &newValue, const QVariant &oldValue);
	/**
	 * Emitted for variants of string-like type.
	 */
	void valueChanged(const QString &newValue);
	/**
	 * Emitted for variants of string-like type.
   * This signal is needed for updating values in KComboBox-es, see KComboBox::setCurrentItem(),
	 * second bool parameter, insert.
   */
	void valueChangedStrAndTrue(const QString &newValue, bool trueBool);
	/**
	 * Emitted for variants of int-like type.
	 */
	void valueChanged(int newValue);
	/**
	 * Emitted for variants of double-like type.
	 */
	void valueChanged(double newValue);
	/**
	 * Emitted for variants of color-like type.
	 */
	void valueChanged(const QColor &newValue);
	/**
	 * Emitted for variants of bool-like type.
	 */
	void valueChanged(bool newValue);

	private:
	QStringMap m_allowed;
	QVariant m_value;									// the actual data
	QVariant m_defaultValue;
	QString m_unit;
	const QString m_id;
	QString m_toolbarCaption;					// Short description shown in e.g. properties dialog
	QString m_editorCaption;					// Text displayed before the data entry widget in the toolbar
	QString m_filter;									// If type() == Type::FileName this is the filter used in file dialogs.
	double m_minAbsValue = 1e-6;
	double m_minValue = 1e-6;
	double m_maxValue = 1e9;
	TypeValue m_type;
	int m_colorScheme;
	bool m_bAdvanced = false;					// If advanced, only display data in item editor
	bool m_bHidden = false;						// If hidden, do not allow user to change data
	bool m_bSetDefault = false;				// If false, then the default will be set to the first thing this variant is set to
};
