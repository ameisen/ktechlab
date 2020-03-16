#include "colorcombo.h"
#include "diode.h"
#include "led.h"
#include "ecnode.h"
#include "libraryitem.h"
#include "simulator.h"

#include <KLocalizedString>
#include <QPainter>
#include <QLine>

#include <algorithm>

namespace {
	static const QPolygon ThisPolygon = {{
		QPoint{ 8,  0},
		QPoint{-8, -8},
		QPoint{-8,  8}
	}};

	static const QVector<QLine> ThisLines = {
		//BEGIN draw "Diode" part
		QLine{8,  -8,  8,  8},
		//END draw "Diode" part

		//BEGIN draw "Arrows" part
		QLine{7,  10, 10, 13}, // Tail of left arrow
		QLine{10, 13, 8,  13}, // Left edge of left arrow tip
		QLine{10, 13, 10, 11}, // Right edge of left arrow tip
		QLine{10, 7,  13, 10}, // Tail of right arrow
		QLine{13, 10, 11, 10}, // Left edge of right arrow tip
		QLine{13, 10, 13, 8 }, // Right edge of right arrow tip
		QLine{8,  13, 13, 8 } // Diagonal line that forms base of both arrow tips
		//END draw "Arrows" part1
	};
}

Item* LED::construct(ItemDocument *itemDocument, bool newItem, const char *id) {
	return new LED(dynamic_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem* LED::libraryItem() {
	static const auto libraryIDList = QStringList(QString(GetCategoryID(Category, ID).data()));
	return new LibraryItem(
		libraryIDList,
		i18n(Name),
		i18n("Outputs"),
		"led.png",
		LibraryItem::lit_component,
		&LED::construct
	);
}

LED::LED(ICNDocument *icnDocument, bool newItem, const char *id) :
	Super(icnDocument, newItem, id ?: ID),
	zeroColor(createPropertyRef("0-color", Variant::Type::Color)),
	minCurrent(createPropertyRef("minCurrent", Variant::Type::Real)),
	maxCurrent(createPropertyRef("maxCurrent", Variant::Type::Real))
{
	m_bDynamicContent = true;
	m_name = i18n(Name);
	setSize(
		{-8, -16, 24, 24},
		true
	);

	zeroColor.setCaption(i18n("Color"));
	zeroColor.setColorScheme(ColorCombo::LED);

	minCurrent.set<real>(MinCurrent);
	maxCurrent.set<real>(MaxCurrent);
}

void LED::dataChanged() {
	const auto zero_color = zeroColor.get<QColor>();
	constexpr const real maxRecip = 1.0 / 255.0;
	color.r = saturate(real(zero_color.red()) * maxRecip);
	color.g = saturate(real(zero_color.green()) * maxRecip);
	color.b = saturate(real(zero_color.blue()) * maxRecip);
}

void LED::stepNonLogic() {
	const auto actualCurrent = m_diode->current();

	brightness = getBrightnessReal(actualCurrent, minCurrent.get<real>(), maxCurrent.get<real>());
}

real LED::getBrightnessReal(current_t current, current_t minCurrentV, current_t maxCurrentV) {
	return clamped_rlerp(current, minCurrentV, maxCurrentV);
}

int LED::getBrightness(current_t current, current_t minCurrentV, current_t maxCurrentV) {
	return iround<int>(getBrightnessReal(current, minCurrentV, maxCurrentV) * 255.0);
}

void LED::drawShape(QPainter &p) {
	const Point2<int> position = {
		iround<int>(x()),
		iround<int>(y())
	};

	initPainter(p);

	//BEGIN draw "Diode" part
	const auto getColor = [&](real channel) -> int {
		return iround<int>(saturate(brightness * channel) * 255.0);
	};

	p.setBrush(QColor(
		getColor(color.r),
		getColor(color.g),
		getColor(color.b)
	));

	p.save();
	p.translate(QPoint(position));

	p.drawPolygon(ThisPolygon);
	p.drawPolyline(ThisPolygon);
	//END draw "Diode" part

	p.drawLines(ThisLines);

	p.restore();

	deinitPainter(p);
}
