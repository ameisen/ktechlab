#pragma once

#include "pch.hpp"

#include <QProxyStyle>

/**
 * see approach from here:
 * http://stackoverflow.com/questions/5909907/drawing-an-overlay-on-top-of-an-applications-window
 */
class DiagnosticStyle final : public QProxyStyle {
	using Super = QProxyStyle;
	Q_OBJECT

public:
	void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
	virtual ~DiagnosticStyle() override = default;
};
