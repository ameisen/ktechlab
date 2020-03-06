#include "dpimage.h"
#include "itemdocument.h"
#include "libraryitem.h"
#include "resizeoverlay.h"

#include <QDebug>
#include <kiconloader.h>
#include <klocalizedstring.h>

#include <QPainter>
#include <QTimer>

namespace {
	// Start with a blank (grey) image
	static QImage getDefaultImage() {
		QPixmap pm{1, 1};
		pm.fill(Qt::gray);
		return pm.toImage();
	}
}

//BEGIN class ImageScaleThread
ImageScaleThread::ImageScaleThread() :
	m_image(getDefaultImage())
{}

bool ImageScaleThread::updateSettings( const QString &imageURL, const Point2<int> &size ) {
	if (isRunning()) {
		qWarning() << Q_FUNC_INFO << "Cannot update settings while running.\n";
		return false;
	}

	bool changed = false;

	if (m_size != size) {
		m_size = size;
		changed = true;
	}

	if (m_imageURL != imageURL) {
		m_imageURL = imageURL;
		m_image.load( m_imageURL );
		if (m_image.isNull()) {
			m_image = getDefaultImage();
		}
		changed = true;
	}

	if (changed) {
		m_bSettingsChanged = true;
		m_bDoneNormalScale = false;
		m_bDoneSmoothScale = false;
	}

	return changed;
}

const QImage & ImageScaleThread::bestScaling( BestScaling &scaling ) const {
	if (m_bDoneSmoothScale) {
		scaling = BestScaling::SmoothScaled;
		return m_smoothScaled;
	}
	else if (m_bDoneNormalScale) {
		scaling = BestScaling::NormalScaled;
		return m_normalScaled;
	}
	else {
		scaling = BestScaling::Unscaled;
		return m_image;
	}
}

// TODO : Nothing about this is threadsafe
void ImageScaleThread::run() {
	do {
		m_bSettingsChanged = false;
		if (!m_bDoneNormalScale ){
			m_normalScaled = m_image.scaled(m_size);
			m_bDoneNormalScale = true;
		}
	}
	while (m_bSettingsChanged);

	// If m_bSettingsChanged is true, then another thread called updateSettings
	// while we were doing normal scaling, so don't bother doing smooth scaling
	// just yet.

	if (!m_bDoneSmoothScale) {
    m_smoothScaled = m_image.scaled(
			m_size,
			Qt::IgnoreAspectRatio,
			Qt::SmoothTransformation
		);
		m_bDoneSmoothScale = true;
	}
}
//END class ImageScaleThread

//BEGIN class DPImage
Item* DPImage::construct( ItemDocument *itemDocument, bool newItem, const char *id ) {
	return new DPImage( itemDocument, newItem, id );
}

LibraryItem* DPImage::libraryItem() {
	return new LibraryItem(
		QStringList(QString("dp/image")),
		i18n("Image"),
		i18n("Other"),
		KIconLoader::global()->loadIcon( "text", KIconLoader::Small ),
		LibraryItem::lit_drawpart,
		&DPImage::construct
	);
}

DPImage::DPImage( ItemDocument *itemDocument, bool newItem, const char *id ) :
	DrawPart( itemDocument, newItem, id ? id : "image" ),
	m_pRectangularOverlay(new RectangularOverlay(this)),
	m_pCheckImageScalingTimer(new QTimer(this)),
	m_imageProperty(createPropertyRef("image", Variant::Type::FileName)),
	m_bResizeToImage(newItem)
{
	connect( m_pCheckImageScalingTimer, SIGNAL(timeout()), SLOT(checkImageScaling()) );
	m_pCheckImageScalingTimer->start( 100 );

	m_name = i18n("Image");

	m_imageProperty.setCaption(i18n("Image File"));
	dataChanged();
}

DPImage::~DPImage() {
	m_imageScaleThread.wait();
	delete m_pRectangularOverlay;
}

void DPImage::setSelected( bool selected ) {
	if ( selected == isSelected() )
		return;

	DrawPart::setSelected(selected);
	m_pRectangularOverlay->showResizeHandles(selected);
}

void DPImage::postResize() {
	setItemPoints( QPolygon(m_sizeRect), false );
	m_bSettingsChanged = true;
}

void DPImage::dataChanged() {
	m_imageURL = m_imageProperty.get<QString>();
	m_image.load(m_imageURL);

	if (m_image.isNull()) {
		// Make a grey image
    m_image = m_image.copy(0, 0, width(), height());
		m_image.fill(Qt::gray);

		m_imageScaling = ImageScaleThread::BestScaling::SmoothScaled;
	}
	else {
		if (m_bResizeToImage) {
			setSize(0, 0, m_image.width(), m_image.height());
			m_imageScaling = ImageScaleThread::BestScaling::SmoothScaled;
		}
		else {
			m_bResizeToImage = true;
			m_bSettingsChanged = true;
		}
	}
}

void DPImage::checkImageScaling() {
	if (!m_bSettingsChanged && (m_imageScaling == ImageScaleThread::BestScaling::SmoothScaled)) {
		// Image scaling is already at its best, so return
		return;
	}

	ImageScaleThread::BestScaling bs;
	const auto &im = m_imageScaleThread.bestScaling(bs);
	if (bs > m_imageScaling) {
		m_imageScaling = bs;
		m_image = QPixmap::fromImage(im);
		setChanged();
	}

	if (!m_imageScaleThread.isRunning()) {
		if (m_imageScaleThread.updateSettings(m_imageURL, getSize())) {
			m_bSettingsChanged = false;
			m_imageScaling = ImageScaleThread::BestScaling::Unscaled;
			m_imageScaleThread.start();
		}
	}
}

void DPImage::drawShape(QPainter &p) {
	p.drawPixmap(
		round_int(x() + offsetX()),
		round_int(y() + offsetY()),
		m_image,
		0,
		0,
		width(),
		height()
	);
}
//END class DPImage

#include "moc_dpimage.cpp"
