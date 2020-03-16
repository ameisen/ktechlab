#include "ecnode.h"
#include "ecsignallamp.h"
#include "element.h"
#include "libraryitem.h"
#include "pin.h"

#include <KLocalizedString>

#include <QPainter>

#include <cmath>

// TODO: resistance and power rating should be user definable properties.
static constexpr const resistance_t Resistance = 100.0;
static constexpr const power_t Wattage = 0.5;
// minimal power to create glow. (looks low...)
static constexpr const power_t LightUp = Wattage / 20.0;

#define inputNode (*(m_pPNode[0]))
#define outputNode (*(m_pNNode[0]))

#define inputPin (*(inputNode.pin()))
#define outputPin (*(outputNode.pin()))

Item* ECSignalLamp::construct(ItemDocument *itemDocument, bool newItem, const char *id) {
	return new ECSignalLamp(dynamic_cast<ICNDocument *>(itemDocument), newItem, id);
}

LibraryItem* ECSignalLamp::libraryItem() {
	static const auto libraryIDList = QStringList(QString(GetCategoryID(Category, ID).data()));
	return new LibraryItem(
		libraryIDList,
		i18n(Name),
		i18n("Outputs"),
		"signal_lamp.png",
		LibraryItem::lit_component,
		&ECSignalLamp::construct
	);
}

ECSignalLamp::ECSignalLamp(ICNDocument *icnDocument, bool newItem, const char *id) :
	Super(icnDocument, newItem, id ?: ID)
{
	m_name = i18n(Name);

	setSize(-8, -8, 16, 16);

	init1PinLeft();
	init1PinRight();

	resistance.reset(createResistance(inputNode, outputNode, Resistance));

	m_bDynamicContent = true;
}

void ECSignalLamp::stepNonLogic() {
	const voltage_t currentVoltage = isCurrentKnown() ? (inputPin.voltage() - outputPin.voltage()) : 0.0;
	currentWattage = currentVoltage * (currentVoltage / Resistance);

	// do not try to divide by 0
	advanceSinceUpdate = std::max(advanceSinceUpdate, 1u);

	avgPower = std::abs(
		avgPower * advanceSinceUpdate +
		(currentVoltage * currentVoltage / Resistance)
	) / real(advanceSinceUpdate);

	// Roll-over to zero shouldn't be a big deal in this situation.
	++advanceSinceUpdate;
}

bool ECSignalLamp::isCurrentKnown() const {
	return inputPin.currentIsKnown() && outputPin.currentIsKnown();
}

void ECSignalLamp::drawShape(QPainter &p) {
	initPainter(p);

	const Point2<int> position = {
		iround<int>(x()),
		iround<int>(y())
	};

	// Calculate the brightness as a linear function of power
	const auto brightness = iround<int>(255.0 * clamped_rlerp(currentWattage, LightUp, Wattage));
	advanceSinceUpdate = 0;

	p.save();

	p.translate(QPoint(position));

	p.setBrush(QColor(255, 255 - brightness, 255 - brightness));
	p.drawEllipse( -8, -8, 16, 16 );

	const int pos = 8 - iround<int>(8.0 * M_SQRT1_2);

	p.drawLine( -8 + pos, -8 + pos,  8 - pos, 8 - pos );
	p.drawLine(  8 - pos, -8 + pos, -8 + pos, 8 - pos );

	p.restore();

	deinitPainter(p);
}
