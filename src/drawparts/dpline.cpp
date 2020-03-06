#include "dpline.h"
#include "libraryitem.h"
#include "resizeoverlay.h"
#include "variant.h"

#include <cmath>
#include <cstdlib>
#include <kiconloader.h>
#include <klocalizedstring.h>
#include <QPainter>

//BEGIN class DPLine
Item* DPLine::construct( ItemDocument *itemDocument, bool newItem, const char *id ) {
	return new DPLine( itemDocument, newItem, id );
}

LibraryItem* DPLine::libraryItem() {
	return new LibraryItem(
		QStringList(QString("dp/line")),
		i18n("Line"),
		i18n("Other"),
		KIconLoader::global()->loadIcon( "text", KIconLoader::Small ),
		LibraryItem::lit_drawpart,
		&DPLine::construct
	);
}

DPLine::DPLine( ItemDocument *itemDocument, bool newItem, const char *id ) :
	Super( itemDocument, newItem, id ? id : "line" ),
	m_pLineOverlay(new LineOverlay(this)),
	m_LineColor(createPropertyRef("line-color", Variant::Type::Color)),
	m_LineWidth(createPropertyRef("line-width", Variant::Type::Int)),
	m_LineStyle(createPropertyRef("line-style", Variant::Type::PenStyle)),
	m_CapStyle(createPropertyRef("cap-style", Variant::Type::PenCapStyle))
{
	m_name = i18n("Line");

	m_LineColor.setCaption( i18n("Line Color") );
	m_LineColor.setValue(QColor(Qt::black));

	m_LineWidth.setCaption( i18n("Line Width") );
	m_LineWidth.setMinValue(1);
	m_LineWidth.setMaxValue(1000);
	m_LineWidth.setValue(1);

	m_LineStyle.setCaption( i18n("Line Style") );
	m_LineStyle.setAdvanced(true);
	setDataPenStyle(m_LineStyle, Qt::SolidLine);

	m_CapStyle.setCaption( i18n("Cap Style") );
	m_CapStyle.setAdvanced(true);
	setDataPenCapStyle(m_CapStyle, Qt::FlatCap);
}

DPLine::~DPLine() {
	delete m_pLineOverlay;
}

void DPLine::setSelected( bool selected ) {
	Super::setSelected(selected);
	if ( selected == isSelected() )
		return;

	m_pLineOverlay->showResizeHandles(selected);
}

void DPLine::dataChanged() {
	setPen(
		{
			m_LineColor.get<QColor>(),
			qreal(m_LineWidth.get<int>()),
			m_LineStyle.get<Qt::PenStyle>(),
			m_CapStyle.get<Qt::PenCapStyle>(),
			Qt::MiterJoin
		}
	);

	postResize(); // in case the pen width has changed
	update();
}

void DPLine::postResize() {
	int x1 = offsetX();
	int y1 = offsetY();
	int x2 = x1 + width();
	int y2 = y1 + height();

	QPolygon p(4);
	int dx = std::abs(x1 - x2);
	int dy = std::abs(y1 - y2);
	int pw = pen().width() * 4 / 3 + 2; // approx pw*sqrt(2)
	int px = (x1 < x2) ? -pw : pw;
	int py = (y1 < y2) ? -pw : pw;
	if ( dx && dy && (dx > dy ? (dx * 2 /dy <= 2) : (dy * 2 / dx <= 2)) ) {
		// steep
		if ( px == py ) {
			p[0] = QPoint(x1   ,y1+py);
			p[1] = QPoint(x2-px,y2   );
			p[2] = QPoint(x2   ,y2-py);
			p[3] = QPoint(x1+px,y1   );
		}
		else {
			p[0] = QPoint(x1+px,y1   );
			p[1] = QPoint(x2   ,y2-py);
			p[2] = QPoint(x2-px,y2   );
			p[3] = QPoint(x1   ,y1+py);
		}
	}
	else if ( dx > dy ) {
		// horizontal
		p[0] = QPoint(x1+px,y1+py);
		p[1] = QPoint(x2-px,y2+py);
		p[2] = QPoint(x2-px,y2-py);
		p[3] = QPoint(x1+px,y1-py);
	}
	else {
		// vertical
		p[0] = QPoint(x1+px,y1+py);
		p[1] = QPoint(x2+px,y2-py);
		p[2] = QPoint(x2-px,y2-py);
		p[3] = QPoint(x1-px,y1+py);
	}
	setItemPoints( p, false );
}

void DPLine::drawShape( QPainter &p ) {
	int x1 = int(x() + offsetX());
	int y1 = int(y() + offsetY());
	int x2 = x1 + width();
	int y2 = y1 + height();

	p.drawLine( x1, y1, x2, y2 );
}
//END class DPLine

//BEGIN class DPArrow
Item* DPArrow::construct( ItemDocument *itemDocument, bool newItem, const char *id ) {
	return new DPArrow( itemDocument, newItem, id );
}

LibraryItem* DPArrow::libraryItem() {
	return new LibraryItem(
		QStringList(QString("dp/arrow")),
		i18n("Arrow"),
		i18n("Other"),
		KIconLoader::global()->loadIcon("text", KIconLoader::Small),
		LibraryItem::lit_drawpart,
		DPArrow::construct
	);
}

DPArrow::DPArrow( ItemDocument *itemDocument, bool newItem, const char *id ) :
	Super( itemDocument, newItem, id ? id : "arrow" ),
	m_HeadAngle(createPropertyRef("HeadAngle", Variant::TypeValue::Double))
{
	m_name = i18n("Arrow");

	// We don't want to use the square cap style as it screws up drawing our arrow head
	QStringList allowed = m_CapStyle.allowed();
	allowed.removeAll( DrawPart::penCapStyleToName( Qt::SquareCap ) );
	m_CapStyle.setAllowed(allowed);

	m_HeadAngle.setAdvanced( true );
	m_HeadAngle.setCaption( i18n("Head angle") );
	m_HeadAngle.setMinValue( 10.0 );
	m_HeadAngle.setMaxValue( 60.0 );
	m_HeadAngle.setUnit( QChar(0xb0) ); // degree
	m_HeadAngle.setValue( m_headAngle );
}

DPArrow::~DPArrow() {}

void DPArrow::dataChanged() {
	Super::dataChanged();
	m_headAngle = m_HeadAngle.get<double>();
	setChanged();
}

void DPArrow::drawShape( QPainter & p ) {
	double x1 = x() + offsetX();
	double y1 = y() + offsetY();
	double x2 = x1 + width();
	double y2 = y1 + height();

	p.drawLine( round_int(x1), round_int(y1), round_int(x2), round_int(y2) );

	double dx = x2 - x1;
	double dy = y2 - y1;

	if ( dx == 0.0 && dy == 0.0 )
		return;

	double arrow_angle = (dx == 0) ?
		((dy > 0) ? M_PI_2 : -M_PI_2) :
		std::atan(dy / dx);
	if ( dx < 0 )
		arrow_angle += M_PI;

	double head_angle = M_PI * m_headAngle / 180.0;
	double head_length = 10.0;

	// Position of arrowhead
	double x3 = x2 + head_length * std::cos( M_PI + arrow_angle - head_angle );
	double y3 = y2 + head_length * std::sin( M_PI + arrow_angle - head_angle );
	double x4 = x2 + head_length * std::cos( M_PI + arrow_angle + head_angle );
	double y4 = y2 + head_length * std::sin( M_PI + arrow_angle + head_angle );

	// Draw arrowhead
	QPen pen = p.pen();
	pen.setCapStyle( Qt::RoundCap );
	p.setPen(pen);
	p.setBrush(pen.color());
	QPolygon pa(3);
	pa[0] = QPoint( round_int(x2), round_int(y2) );
	pa[1] = QPoint( round_int(x3), round_int(y3) );
	pa[2] = QPoint( round_int(x4), round_int(y4) );
	p.drawPolygon(pa);
	p.drawPolyline(pa);
}
//END class DPLine
