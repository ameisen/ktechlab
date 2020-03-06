#include "diagnosticstyle.h"

#include <QColor>
#include <QPainter>
#include <QWidget>

namespace Color {
	static const auto Border = QColor(Qt::red);
	static const auto ClassName = QColor(Qt::darkBlue);
	static const auto Translucent = QColor{255, 246, 240, 100};
}

void DiagnosticStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const {
	QProxyStyle::drawControl(element, option, painter, widget);
	if (Ptr::anyNull(widget, painter))
		return;

	// draw a border around the widget
	painter->setPen(Color::Border);
	painter->drawRect(widget->rect());

	// show the classname of the widget
	painter->fillRect(widget->rect(), Color::Translucent);
	painter->setPen(Color::ClassName);

	QString text = widget->metaObject()->className();
	text.append(":");
	text.append(widget->objectName());
	painter->drawText(
		widget->rect(),
		Qt::AlignLeft | Qt::AlignTop,
		text
	);
}

#include "moc_diagnosticstyle.cpp"
