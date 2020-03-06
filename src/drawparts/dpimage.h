#pragma once

#include "drawpart.h"

#include <QImage>
#include <QPixmap>
#include <QThread>

#include <atomic>

/**
@short Thread to perform quick and then good image scaling.
@author David Saxton
*/
class ImageScaleThread final : public QThread {
	Q_OBJECT

	public:
		enum class BestScaling { Unscaled, NormalScaled, SmoothScaled };

		ImageScaleThread();
		/**
		 * Use the given settings.
		 * @return if any of the settings changed.
		 */
		bool updateSettings( const QString &imageURL, const Point2<int> &size );
		/**
		 * @param scaling is set to the type of scaling that this image has had.
		 * @return the best image done so far.
		 */
		const QImage & bestScaling( BestScaling &scaling ) const;

	protected:
		/**
		 * Start scaling.
		 */
		void run() override;

		QImage m_image;
		QImage m_normalScaled;
		QImage m_smoothScaled;

		QString m_imageURL;

		Point2<int> m_size = {-1, -1};

		bool m_bDoneNormalScale = false;
		bool m_bDoneSmoothScale = false;
		std::atomic<bool> m_bSettingsChanged = { false };
};


/**
@short Represents editable text on the canvas
@author David Saxton
 */
class DPImage final : public DrawPart
{
	Q_OBJECT

	public:
		DPImage( ItemDocument *itemDocument, bool newItem, const char *id = nullptr );
		~DPImage() override;

		static Item* construct( ItemDocument *itemDocument, bool newItem, const char *id );
		static LibraryItem *libraryItem();

		void setSelected( bool selected ) override;

	protected:
		void postResize() override;

	protected slots:
		/**
		 * Called from a timeout event after resizing to see if the image
		 * resizing thread has done anything useful yet.
		 */
		void checkImageScaling();

	private:
		void drawShape( QPainter &p ) override;
		void dataChanged() override;

		QPixmap m_image;
		ImageScaleThread m_imageScaleThread;
		QString m_imageURL;
		ImageScaleThread::BestScaling m_imageScaling = ImageScaleThread::BestScaling::Unscaled;
		RectangularOverlay * const m_pRectangularOverlay = nullptr;
		QTimer * const m_pCheckImageScalingTimer = nullptr;
		Property &m_imageProperty;
		bool m_bSettingsChanged = false;

		/**
		 * If we have been loaded from a file, etc, then we want to keep the
		 * previous size instead of resizing ourselves to the new image size
		 * like we would do normally if the user loads an image.
		 */
		bool m_bResizeToImage = false;
};
