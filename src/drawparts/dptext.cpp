#include "dptext.h"
#include "itemdocument.h"
#include "libraryitem.h"
#include "resizeoverlay.h"

#include <kiconloader.h>
#include <klocalizedstring.h>

#include <QPainter>
#include <QTextEdit>

Item* DPText::construct( ItemDocument *itemDocument, bool newItem, const char *id ) {
	return new DPText( itemDocument, newItem, id );
}

LibraryItem* DPText::libraryItem() {
	QStringList idList;
	idList << "dp/text" << "dp/canvas_text" << "canvas_text";

	return new LibraryItem(
		idList,
		i18n("Canvas Text"),
		i18n("Other"),
		KIconLoader::global()->loadIcon( "text", KIconLoader::Small ),
		LibraryItem::lit_drawpart,
		&DPText::construct
	);
}

DPText::DPText( ItemDocument *itemDocument, bool newItem, const char *id ) :
	DrawPart(itemDocument, newItem, id ? id : "canvas_text"),
	m_rectangularOverlay(new RectangularOverlay(this)),
	m_Text(createPropertyRef("text", Variant::Type::RichText)),
	m_Background(createPropertyRef("background", Variant::Type::Bool)),
	m_BackgroundColor(createPropertyRef("background-color", Variant::Type::Color)),
	m_FrameColor(createPropertyRef("frame-color", Variant::Type::Color))
{
	m_name = i18n("Text");

	m_Text.setValue( i18n("Text") );

	m_Background.setValue(false);
	m_Background.setCaption( i18n("Display Background") );
	m_Background.setAdvanced(true);

	m_BackgroundColor.setValue(QColor(Qt::white));
	m_BackgroundColor.setCaption( i18n("Background Color") );
	m_BackgroundColor.setAdvanced(true);

	m_FrameColor.setValue(QColor(Qt::black));
	m_FrameColor.setCaption( i18n("Frame Color") );
	m_FrameColor.setAdvanced(true);
}

DPText::~DPText() {
	delete m_rectangularOverlay;
}

void DPText::setSelected( bool selected ) {
	if ( selected == isSelected() )
		return;

	DrawPart::setSelected(selected);
	m_rectangularOverlay->showResizeHandles(selected);
}

void DPText::dataChanged() {
	b_displayBackground = m_Background.get<bool>();
	m_backgroundColor = m_BackgroundColor.get<QColor>();
	m_frameColor = m_FrameColor.get<QColor>();

	m_text = m_Text.get<QString>();

	if ( !Qt::mightBeRichText( m_text ) ) {
		// Format the text to be HTML
		m_text.replace( '\n', "<br>" );
	}

	update();
}

void DPText::postResize() {
	setItemPoints(QPolygon(m_sizeRect), false);
}

QSize DPText::minimumSize() const {
	return QSize(48, 24);
}

void DPText::drawShape( QPainter &p ) {
	auto bound = m_sizeRect;
	bound.setWidth( bound.width() - 2 );
	bound.setHeight( bound.height() - 2 );
	bound.translate(
		round_int(x()) + 1,
		round_int(y()) + 1
	);

	if (b_displayBackground) {
		p.save();
		p.setPen( QPen( m_frameColor, 1, Qt::DotLine) );
		p.setBrush(m_backgroundColor);
		p.drawRect(bound);
		p.restore();
	}

	constexpr const int pad = 6;

	bound.setLeft( bound.left() + pad );
	bound.setTop( bound.top() );
	bound.setRight( bound.right() - pad );
	bound.setBottom( bound.bottom() - pad );

	// TODO : Should we really be instantiating a new instance of this _every_ time we draw?
	auto t = QTextEdit(m_text);
  t.resize( bound.width(), bound.height() );
	t.viewport()->setAutoFillBackground( false );
  t.setFrameStyle(QFrame::NoFrame);

  t.render( &p, bound.topLeft(), QRegion(), QWidget::DrawChildren );
}
