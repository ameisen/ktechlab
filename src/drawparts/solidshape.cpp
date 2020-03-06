#include "solidshape.h"
#include "libraryitem.h"
#include "resizeoverlay.h"

#include <kiconloader.h>
#include <klocalizedstring.h>
#include <QPainter>

#include <cmath>
#include <algorithm>

//BEGIN class DPRectangle
Item * DPRectangle::construct( ItemDocument *itemDocument, bool newItem, const char *id ) {
	return new DPRectangle( itemDocument, newItem, id );
}

LibraryItem* DPRectangle::libraryItem() {
	return new LibraryItem(
		QStringList(QString("dp/rectangle")),
		i18n("Rectangle"),
		i18n("Other"),
		KIconLoader::global()->loadIcon( "text", KIconLoader::Small ),
		LibraryItem::lit_drawpart,
		&DPRectangle::construct
	);
}

DPRectangle::DPRectangle( ItemDocument *itemDocument, bool newItem, const char *id ) :
	DrawPart(itemDocument, newItem, id ? id : "rectangle"),
	m_pRectangularOverlay(new RectangularOverlay(this)),
	m_Background(createPropertyRef("background", Variant::Type::Bool)),
	m_BackgroundColor(createPropertyRef("background-color", Variant::Type::Color)),
	m_LineColor(createPropertyRef("line-color", Variant::Type::Color)),
	m_LineWidth(createPropertyRef("line-width", Variant::Type::Int)),
	m_LineStyle(createPropertyRef("line-style", Variant::Type::PenStyle))
{
	m_name = i18n("Rectangle");

	m_Background.setValue(false);
	m_Background.setCaption( i18n("Display Background") );
	m_Background.setAdvanced(true);

	m_BackgroundColor.setValue(QColor(Qt::white));
	m_BackgroundColor.setCaption( i18n("Background Color") );
	m_BackgroundColor.setAdvanced(true);

	m_LineColor.setValue(QColor(Qt::black));
	m_LineColor.setCaption( i18n("Line Color") );
	m_LineColor.setAdvanced(true);

	m_LineWidth.setCaption( i18n("Line Width") );
	m_LineWidth.setMinValue(1);
	m_LineWidth.setMaxValue(1000);
	m_LineWidth.setValue(1);
	m_LineWidth.setAdvanced(true);

	m_LineStyle.setAdvanced(true);
	m_LineStyle.setCaption( i18n("Line Style") );
	setDataPenStyle(m_LineStyle, Qt::SolidLine);
}

DPRectangle::~DPRectangle() {
	delete m_pRectangularOverlay;
}

void DPRectangle::setSelected(bool selected) {
	Super::setSelected(selected);
	if (selected == isSelected())
		return;

	m_pRectangularOverlay->showResizeHandles(selected);
}

void DPRectangle::dataChanged() {
	auto displayBackground = m_Background.get<bool>();
	auto line_color = m_LineColor.get<QColor>();
	auto width = qreal(m_LineWidth.get<int>());
	auto style = m_LineStyle.get<Qt::PenStyle>();

	setPen( QPen( line_color, width, style ) );

	setBrush(displayBackground ? m_BackgroundColor.get<QColor>() : Qt::NoBrush);

	postResize();
	update();
}

QSize DPRectangle::minimumSize() const {
	auto side = std::max(16, pen().width() + 2);
	return QSize{side, side};
}

void DPRectangle::postResize() {
	setItemPoints( m_sizeRect, false );
}

QRect DPRectangle::drawRect() const {
	const auto size = getSize();

	const auto lw = std::min({
		pen().width(),
		size.width,
		size.height
	});

	const auto offset = getOffset();

	return QRect(
		round_int(x() + offset.x + lw / 2),
		round_int(y() + offset.y + lw / 2),
		size.width - lw,
		size.height - lw
	);
}

void DPRectangle::drawShape(QPainter &p){
	p.drawRect(drawRect());
}
//END class DPRectangle

//BEGIN class DPEllipse
Item * DPEllipse::construct( ItemDocument *itemDocument, bool newItem, const char *id ) {
	return new DPEllipse( itemDocument, newItem, id );
}

LibraryItem* DPEllipse::libraryItem() {
	return new LibraryItem(
		QStringList(QString("dp/ellipse")),
		i18n("Ellipse"),
		i18n("Other"),
		KIconLoader::global()->loadIcon( "text", KIconLoader::Small ),
		LibraryItem::lit_drawpart,
		DPEllipse::construct
	);
}

DPEllipse::DPEllipse( ItemDocument *itemDocument, bool newItem, const char *id ) :
	Super(itemDocument, newItem, id ? id : "ellipse")
{
	m_name = i18n("Ellipse");
}

DPEllipse::~DPEllipse() {}

void DPEllipse::postResize() {
	QRect br = m_sizeRect;

	// Make octagon that roughly covers ellipse
	QPolygon pa(8);
	pa[0] = QPoint( br.x() + br.width()/4,		br.y() );
	pa[1] = QPoint( br.x() + 3*br.width()/4,	br.y() );
	pa[2] = QPoint( br.x() + br.width(),			br.y() + br.height()/4 );
	pa[3] = QPoint( br.x() + br.width(),			br.y() + 3*br.height()/4 );
	pa[4] = QPoint( br.x() + 3*br.width()/4,	br.y() + br.height() );
	pa[5] = QPoint( br.x() + br.width()/4,		br.y() + br.height() );
	pa[6] = QPoint( br.x(),										br.y() + 3*br.height()/4 );
	pa[7] = QPoint( br.x(),										br.y() + br.height()/4 );

	setItemPoints( pa, false );
}

void DPEllipse::drawShape(QPainter &p) {
	p.drawEllipse(drawRect());
}
//END class SolidShape
