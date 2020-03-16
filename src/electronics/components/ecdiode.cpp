#include "diode.h"
#include "ecdiode.h"
#include "ecnode.h"
#include "libraryitem.h"

#include <KLocalizedString>
#include <QPainter>

namespace {
	static const QPolygon ThisPolygon = {{
		QPoint{ 8,  0},
		QPoint{-8, -8},
		QPoint{-8,  8}
	}};

	static constexpr const QLine ThisLine = {8, 8, 8, 8};
}

Item* ECDiode::construct(ItemDocument *itemDocument, bool newItem, const char *id) {
	return new ECDiode((ICNDocument*)itemDocument, newItem, id);
}

LibraryItem* ECDiode::libraryItem() {
	static const auto libraryIDList = QStringList(QString(GetCategoryID(Category, ID).data()));
	return new LibraryItem(
		libraryIDList,
		i18n(Name),
		i18n("Discrete"),
		"diode.png",
		LibraryItem::lit_component,
		&ECDiode::construct
	);
}

ECDiode::ECDiode(ICNDocument *icnDocument, bool newItem, const char *id) :
	Super(icnDocument, newItem, id ?: ID),
	saturationCurrent(createPropertyRef( "I_S", Variant::Type::Double )),
	emissionCoefficient(createPropertyRef( "N", Variant::Type::Double )),
	breakdownVoltage(createPropertyRef( "V_B", Variant::Type::Double ))
{
	m_name = i18n(Name);

	setSize( -8, -8, 16, 16 );

	init1PinLeft();
	init1PinRight();

	m_diode = createDiode( m_pNNode[0], m_pPNode[0] );

	DiodeSettings ds; // it will have the default properties that we use

	saturationCurrent.setCaption("Saturation Current");
	saturationCurrent.setUnit("A");
	saturationCurrent.setMinValue(1e-20);
	saturationCurrent.setMaxValue(1e-0);
	saturationCurrent.setValue( ds.I_S );
	saturationCurrent.setAdvanced(true);

	emissionCoefficient.setCaption( i18n("Emission Coefficient") );
	emissionCoefficient.setMinValue(1e0);
	emissionCoefficient.setMaxValue(1e1);
	emissionCoefficient.setValue( ds.N );
	emissionCoefficient.setAdvanced(true);

	breakdownVoltage.setCaption( i18n("Breakdown Voltage") );
	breakdownVoltage.setUnit("V");
	breakdownVoltage.setMinAbsValue(1e-5);
	breakdownVoltage.setMaxValue(1e10);
	breakdownVoltage.setValue( ds.V_B );
	breakdownVoltage.setAdvanced(true);
}

void ECDiode::dataChanged() {
	const DiodeSettings ds = {
		saturationCurrent.get<double>(),
		emissionCoefficient.get<double>(),
		breakdownVoltage.get<double>()
	};

	m_diode->setDiodeSettings(ds);
}

void ECDiode::drawShape(QPainter &p) {
	initPainter(p);

	const Point2<int> position = {
		int(x()),
		int(y())
	};

	p.save();
	p.translate(QPoint(position));

	p.drawPolygon(ThisPolygon);
	p.drawPolyline(ThisPolygon);

	p.drawLine(ThisLine);

	p.restore();

	deinitPainter(p);
}
