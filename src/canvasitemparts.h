/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef CANVASITEMPARTS_H
#define CANVASITEMPARTS_H

//#include <canvas.h> // 2018.10.16 - not needed
#include "canvasitems.h"
#include <qpointer.h>
#include <qslider.h>
#include <qtoolbutton.h>
#include <QIcon>

class Cells;
class CIWidgetMgr;
class CNItem;
class SliderWidget;
class ToolButton;
class QString;

class GuiPart : public KtlQCanvasRectangle
{
	Q_OBJECT
	public:
		/**
		 * Create a GuiPart. Control the position using setGuiPartSize, instead
		 * of calling KtlQCanvasRectangle::setSize. This allows GuiPart to know
		 * when its size has been changed
		 */
		GuiPart( CNItem *parent, const QRect &r, KtlQCanvas *canvas );
		~GuiPart() override;

		virtual QRect recommendedRect() const { return m_originalRect; }
		void setOriginalRect( const QRect & r ) { m_originalRect = r; }

		virtual void updateConnectorPoints( bool add );

		/**
		 * Set the angle that the GuiPart draws itself (if the GuiPart chooses
		 * to use it by calling initPainter and deinitPainter from drawShape).
		 * Note that this doesn't affect the rectangle position that the
		 * GuiPart is in. The rotation is taken to be about the center of the
		 * rectangle.
		 */
		void setAngleDegrees( int angleDegrees );
		/**
		 * Control the size. Call this instead of KtlQCanvasRectangle::setSize. In
		 * turn, this function will notify subclasses via posChanged();
		 */
		void setGuiPartSize( int width, int height );
		/**
		 * Returns the rectangle to draw in to compensate for rotation of
		 * the QPainter
		 */
		QRect drawRect();

		int angleDegrees() const { return m_angleDegrees; }
		CNItem *parent() const { return p_parent; }

	protected:
		/**
		 * Called when the size or angle changes
		 */
		virtual void posChanged() {;}
		/**
		 * Rotate / etc the painter. You must call deinitPainter after
		 * calling this function.
		 */
		void initPainter( QPainter &p );
		/**
		 * Complement function to initPainter - restores painter to normal
		 * transform
		 */
		void deinitPainter( QPainter &p );
		QRect m_originalRect;
		CNItem *p_parent;
		int m_angleDegrees = 0;
		bool b_pointsAdded = false;

	private slots:
		void slotMoveBy( double dx, double dy );
};


/**
@short Stores internal information about text associated with CNItem
@author David Saxton
*/
class Text : public GuiPart
{
	Q_OBJECT
	public:
		Text( const QString &text, CNItem *parent, const QRect &r, KtlQCanvas *canvas, int flags = Qt::AlignHCenter | Qt::AlignVCenter );
		~Text() override;

		/**
		 * Set the text, returning true if the size of this Text on the canvas
		 * has changed.
		 */
		bool setText( const QString &text );
		QRect recommendedRect() const override;
		void drawShape ( QPainter &p ) override;
		/**
		 * The text flags (see QPainter::drawText) - Qt::AlignmentFlags and
		 * Qt::TextFlags OR'd together.
		 */
		int flags() const { return m_flags; }
		/**
		 * @see flags
		 */
		void setFlags( int flags );

	protected:
		QString m_text;
		int m_flags;
};
using TextMap = QMap<QString, QPointer<Text>>;


/**
@short Base class for embedding Qt Widgets into the canvas
@author David Saxton
*/
class Widget : public GuiPart
{
	public:
		Widget( const QString &id, CNItem *parent, const QRect &r, KtlQCanvas *canvas );
		~Widget() override;


		virtual QWidget * widget() const = 0;
		const QString & id() const { return m_id; }

		/**
		 * Set the widget enabled/disabled
		 */
		void setEnabled( bool enabled );

		virtual void enterEvent(QEvent *) {};
		virtual void leaveEvent(QEvent *) {};

		/**
		 * Mouse was pressed. pos is given relative to CNItem position.
		 */
		virtual void mousePressEvent( [[maybe_unused]] QMouseEvent *e ) { }
		/**
		 * Mouse was released. pos is given relative to CNItem position.
		 */
		virtual void mouseReleaseEvent( [[maybe_unused]] QMouseEvent *e ) { }
		/**
		 * Mouse was double clicked. pos is given relative to CNItem position.
		 */
		virtual void mouseDoubleClickEvent( [[maybe_unused]] QMouseEvent *e ) { }
		/**
		 * Mouse was moved. pos is given relative to CNItem position.
		 */
		virtual void mouseMoveEvent( [[maybe_unused]] QMouseEvent *e ) { }
		/**
		 * Mouse was scrolled. pos is given relative to CNItem position.
		 */
		virtual void wheelEvent( [[maybe_unused]] QWheelEvent *e ) { }

		void drawShape( QPainter &p ) override;

	protected:
		void posChanged() override;
		QString m_id;
};


class ToolButton : public QToolButton
{
	public:
		ToolButton( QWidget *parent );

		void mousePressEvent( QMouseEvent *e ) override { QToolButton::mousePressEvent(e); }
		void mouseReleaseEvent( QMouseEvent *e ) override { QToolButton::mouseReleaseEvent(e); }
		void mouseDoubleClickEvent ( QMouseEvent *e ) override { QToolButton::mouseDoubleClickEvent(e); }
		void mouseMoveEvent( QMouseEvent *e ) override { QToolButton::mouseMoveEvent(e); }
		void wheelEvent( QWheelEvent *e ) override { QToolButton::wheelEvent(e); }
		void enterEvent(QEvent *) override { QToolButton::enterEvent(0l); }
		void leaveEvent(QEvent *) override { QToolButton::leaveEvent(0l); }

		void setAngleDegrees( int angleDegrees ) { m_angleDegrees = angleDegrees; }

	protected:
		virtual void drawButtonLabel( QPainter *p );

		QFont m_font;
		int m_angleDegrees = 0;
};


/**
@short Stores internal information about button associated with CNItem
@author David Saxton
*/
class Button : public Widget
{
	Q_OBJECT
	public:
		Button( const QString &id, CNItem *parent, bool isToggle, const QRect &r, KtlQCanvas *canvas );
		~Button() override;

		void mousePressEvent( QMouseEvent *e ) override;
		void mouseReleaseEvent( QMouseEvent *e ) override;
		void enterEvent(QEvent *) override;
		void leaveEvent(QEvent *) override;

		/**
		 * Set the text displayed inside the button
		 */
		void setText( const QString &text );
		void setToggle( bool toggle );
		bool isToggle() const { return b_isToggle; }
		QWidget *widget() const override;
		bool state() const;
		void setIcon( const QIcon& );
		void setState( bool state );
		QRect recommendedRect() const override;

	protected:
		void posChanged() override;

	private slots:
		void slotStateChanged();

	private:
		ToolButton *m_button = nullptr;
		bool b_isToggle; // i.e. whether it should be depressed when the mouse is released
};


class SliderWidget : public QSlider
{
	public:
		SliderWidget( QWidget *parent );

		void mousePressEvent( QMouseEvent *e ) override { QSlider::mousePressEvent(e); }
		void mouseReleaseEvent( QMouseEvent *e ) override { QSlider::mouseReleaseEvent(e); }
		void mouseDoubleClickEvent ( QMouseEvent *e ) override { QSlider::mouseDoubleClickEvent(e); }
		void mouseMoveEvent( QMouseEvent *e ) override { QSlider::mouseMoveEvent(e); }
		void wheelEvent( QWheelEvent *e ) override { QSlider::wheelEvent(e); }
		void enterEvent(QEvent *) override { QSlider::enterEvent(0l); }
		void leaveEvent(QEvent *) override { QSlider::leaveEvent(0l); }
};


/**
@short Stores internal information about a QSlider associated with CNItem
@author David Saxton
*/
class Slider : public Widget
{
	Q_OBJECT
	public:
		Slider( const QString &id, CNItem *parent, const QRect &r, KtlQCanvas *canvas );
		~Slider() override;

		void mousePressEvent( QMouseEvent *e ) override;
		void mouseReleaseEvent( QMouseEvent *e ) override;
		void mouseDoubleClickEvent ( QMouseEvent *e ) override;
		void mouseMoveEvent( QMouseEvent *e ) override;
		void wheelEvent( QWheelEvent *e ) override;
		void enterEvent(QEvent *) override;
		void leaveEvent(QEvent *) override;

		QWidget * widget() const override;
		int value() const;
		void setValue( int value );
		void setOrientation( Qt::Orientation o );

	protected:
		void posChanged() override;

	private slots:
		void slotValueChanged( int value );

	private:
		SliderWidget *m_slider;
		Qt::Orientation m_orientation = Qt::Vertical;
		bool m_bSliderInverted = false; ///< In some orientations, the slider is reflected
};

#endif
