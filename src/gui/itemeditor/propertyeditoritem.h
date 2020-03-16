#pragma once

#include "property.h"

#include <QTableWidget>

/** This class is a subclass of K3ListViewItem which is associated to a property.
    It also takes care of drawing custom contents.
 **/
 //! An item in PropertyEditorItem associated to a property
class PropertyEditorItem final : public QObject, public QTableWidgetItem {
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

	~PropertyEditorItem() override = default;

	/**
	 * \return property's name.
	 */
	QString name() const { return m_property ? m_property->id() : QString{}; }
	/**
	 * \return properties's type.
	 */
	Variant::Type type() { return m_property ? m_property->type() : Variant::Type::None; }
	/**
	 * \return a pointer to the property associated to this item.
	 */
	Property *property() { return m_property; }
	/**
	 * Updates text on of this item, for current property value. If
	 * \a alsoParent is true, parent item (if present) is also updated.
	 */
	virtual void updateValue(bool alsoParent = true);

	protected slots:
	virtual void propertyValueChanged();

	private:
	Property * const m_property = nullptr;
};
