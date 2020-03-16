#include "drawpart.h"
#include "propertyeditor.h"
#include "propertyeditoritem.h"

#include <QDebug>
#include <KIconLoader>
#include <KLocalizedString>

#include <QColor>
#include <QCursor>
#include <QFont>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QSize>

//BEGIN Class PropertyEditorItem
PropertyEditorItem::PropertyEditorItem(
	[[maybe_unused]] PropertyEditorItem *par,
	Property *property
) :
	QTableWidgetItem(property->editorCaption()),
	m_property(property)
{}

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

void PropertyEditorItem::propertyValueChanged() {
	setText(m_property ? m_property->displayString() : QString{});
}

void PropertyEditorItem::updateValue(bool alsoParent) {
	QString text = m_property ? m_property->displayString() : QString{};
	qDebug() << Q_FUNC_INFO << "text= " << text;
	setText(text);
	if (alsoParent && QObject::parent()) {
		static_cast<PropertyEditorItem*>(QObject::parent())->updateValue();
	}
}

//END class PropertyEditorItem

#include "moc_propertyeditoritem.cpp"
