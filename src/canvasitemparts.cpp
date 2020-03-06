/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "canvasitemparts.h"
#include "cells.h"
#include "cnitem.h"
#include "icndocument.h"

#include <qevent.h>
#include <qpainter.h>
#include <qdebug.h>

#include <algorithm>

//BEGIN Class GuiPart
GuiPart::GuiPart( CNItem *parent, const QRect &r, KtlQCanvas *canvas ) :
	KtlQCanvasRectangle(r, canvas),
	m_originalRect(r),
	p_parent(parent)
{
	connect( parent, SIGNAL(movedBy(double, double )), this, SLOT(slotMoveBy(double, double )) );
	setZ( parent->z() + 0.5 );
}


GuiPart::~GuiPart()
{
	hide();
}


void GuiPart::setAngleDegrees( int angleDegrees )
{
	m_angleDegrees = angleDegrees;
	posChanged();
	if (canvas())
		canvas()->setChanged( boundingRect() );
}


void GuiPart::setGuiPartSize( int width, int height )
{
	updateConnectorPoints(false);
	setSize( width, height );
	posChanged();
}


void GuiPart::initPainter( QPainter &p )
{
	if ((m_angleDegrees % 180) == 0)
		return;

	p.translate( int(x()+(width()/2)), int(y()+(height()/2)) );
	p.rotate(m_angleDegrees);
	p.translate( -int(x()+(width()/2)), -int(y()+(height()/2)) );
}


void GuiPart::deinitPainter( QPainter &p )
{
	if ((m_angleDegrees % 180) == 0 )
		return;

	p.translate( int(x()+(width()/2)), int(y()+(height()/2)) );
	p.rotate(-m_angleDegrees);
	p.translate( -int(x()+(width()/2)), -int(y()+(height()/2)) );
}


void GuiPart::slotMoveBy( double dx, double dy )
{
	if (dx == 0 && dy == 0)
		return;

	moveBy(dx, dy);
	posChanged();
}


void GuiPart::updateConnectorPoints( bool add )
{
	if (!p_parent) return;

	auto *icnd = dynamic_cast<ICNDocument*>(p_parent->itemDocument());
	if (!icnd) return;

	auto *cells = icnd->cells();
	if (!cells) return;

	if (!isVisible())
		add = false;

	if (add == b_pointsAdded)
		return;

	b_pointsAdded = add;

	int mult = add ? 1 : -1;
	int sx = roundDown( x(), 8 );
	int sy = roundDown( y(), 8 );
	int ex = roundDown( x() + width(), 8 );
	int ey = roundDown( y() + height(), 8 );

	for ( int x = sx; x <= ex; ++x )
	{
		for ( int y = sy; y <= ey; ++y )
		{
			if ( cells->haveCell( x, y ) )
				cells->cell( x, y ).CIpenalty += mult * ICNDocument::hs_item / 2;
		}
	}
}


QRect GuiPart::drawRect()
{
	auto dr = rect();
	if ((m_angleDegrees % 180) != 0)
	{
		QMatrix m;
		m.translate( int(x()+(width()/2)), int(y()+(height()/2)) );

		if ((m_angleDegrees % 180) != 0)
			m.rotate(-m_angleDegrees);

		m.translate( -int(x()+(width()/2)), -int(y()+(height()/2)) );

		dr = m.mapRect(dr);
	}
	return dr;
}
//END Class GuiPart



//BEGIN Class Text
Text::Text( const QString &text, CNItem *parent, const QRect &r, KtlQCanvas *canvas, int flags ) :
	GuiPart( parent, r, canvas ),
	m_flags(flags)
{
	setText(text);
}


Text::~Text()
{
}


bool Text::setText( const QString &text )
{
	if ( m_text == text )
		return false;

	updateConnectorPoints(false);

	m_text = text;
	return true;
}


void Text::setFlags( int flags )
{
	updateConnectorPoints( false );
	m_flags = flags;
}


void Text::drawShape( QPainter &p )
{
	initPainter(p);
	if (p_parent) {
		p.setFont( p_parent->font() );
	}
	p.drawText( drawRect(), m_flags, m_text );
	deinitPainter(p);
}


QRect Text::recommendedRect() const
{
	if (!p_parent) return QRect{};

	return QFontMetrics( p_parent->font() ).boundingRect( m_originalRect.x(), m_originalRect.y(), m_originalRect.width(), m_originalRect.height(), m_flags, m_text );
}
//END Class Text



//BEGIN Class Widget
Widget::Widget( const QString &id, CNItem *parent, const QRect &r, KtlQCanvas *canvas ) :
	GuiPart( parent, r, canvas ),
	m_id(id)
{
	show();
}

Widget::~Widget()
{
}


void Widget::setEnabled( bool enabled )
{
	auto *obj = widget();
	if (!obj) return;

	obj->setEnabled(enabled);
}


void Widget::posChanged()
{
	auto *obj = widget();
	if (!obj) return;

	// Swap around the width / height if we are rotated at a non-half way around
	obj->setFixedSize(
		((m_angleDegrees % 90) != 0) ?
			QSize{height(), width()} :
			size()
	);

	obj->move( int(x()), int(y()) );
}


void Widget::drawShape( QPainter &p )
{
// 	initPainter(p);
	//p.drawPixmap( int(x()), int(y()), QPixmap::grabWidget( widget() ) ); // 2019.05.06
    p.drawPixmap( int(x()), int(y()), widget()->grab() );
// 	deinitPainter(p);
}
//END Class Widget


//BEGIN Class ToolButton
ToolButton::ToolButton( QWidget *parent ) :
	QToolButton(parent)
{
	// It has to be > 11, not > 12, as (I think) pixelSize() rounds off the actual size
	if (QFontInfo(m_font).pixelSize() > 11)
		m_font.setPixelSize(12);
}


void ToolButton::drawButtonLabel( QPainter *p )
{
	if ((m_angleDegrees % 180) == 0 || text().isEmpty())
	{
    QToolButton::render(p);
		return;
	}

	// TODO : integer divide cast to double. Loses decimal.
	double dx = size().width() / 2;
	double dy = size().height() / 2;

	p->translate( dx, dy );
	p->rotate( m_angleDegrees );
	p->translate( -dx, -dy );

	p->translate( -dy + dx, 0 );

	const int m = std::max(width(), height());

	p->setPen( Qt::black );
	p->drawText(
		isDown() ? 1 : 0,
		isDown() ? 1 : 0,
		m,
		m,
		Qt::AlignVCenter | Qt::AlignHCenter,
		text()
	);

	p->translate( dy-dx, 0 );

	p->translate( dx, dy );
	p->rotate( -m_angleDegrees );
	p->translate( -dx, -dy );
}
//END Class ToolButton


//BEGIN Class Button
Button::Button( const QString & id, CNItem *parent, bool isToggle, const QRect &r, KtlQCanvas *canvas ) :
	Widget( id, parent, r, canvas ),
	b_isToggle(isToggle)
{
	m_button = new ToolButton(nullptr);
	m_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	m_button->setCheckable(b_isToggle);
	connect( m_button, SIGNAL(pressed()), this, SLOT(slotStateChanged()) );
	connect( m_button, SIGNAL(released()), this, SLOT(slotStateChanged()) );
	posChanged();
}


Button::~Button()
{
	delete m_button;
}


void Button::setToggle( bool toggle )
{
	if ( b_isToggle == toggle )
		return;

	if (b_isToggle)
	{
		// We must first untoggle it, else it'll be forever stuck...
		setState(false);
	}

	b_isToggle = toggle;
	m_button->setCheckable(b_isToggle);
}


void Button::posChanged()
{
	Widget::posChanged();
	m_button->setAngleDegrees(m_angleDegrees);
}

void Button::slotStateChanged()
{
	parent()->buttonStateChanged( id(), m_button->isDown() || m_button->isChecked() );
}
QWidget* Button::widget() const
{
	return m_button;
}
void Button::setIcon( const QIcon &icon )
{
	m_button->setIcon(icon);
}
void Button::setState( bool state )
{
	if ( this->state() == state )
		return;

	if ( isToggle() )
		m_button->setChecked(state);
	else
		m_button->setDown(state);

	slotStateChanged();
}
bool Button::state() const
{
	if ( isToggle() )
		return m_button->isChecked(); //was: state()
	else
		return m_button->isDown();
}


QRect Button::recommendedRect() const
{
	auto sizeHint = m_button->sizeHint();
	if ( sizeHint.width() < m_originalRect.width() )
		sizeHint.setWidth( m_originalRect.width() );

	// Hmm...for now, lets just keep the recomended rect the same height as the original rect
	sizeHint.setHeight( m_originalRect.height() );

	int hdw = (sizeHint.width() - m_originalRect.width())/2;
	int hdh = (sizeHint.height() - m_originalRect.height())/2;

	return QRect( m_originalRect.x()-hdw, m_originalRect.y()-hdh, sizeHint.width(), sizeHint.height() );
}


void Button::setText( const QString &text )
{
	if ( m_button->text() == text )
		return;

	updateConnectorPoints(false);

	m_button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	m_button->setText(text);
	m_button->setToolTip(text);
	canvas()->setChanged( rect() );
	p_parent->updateAttachedPositioning();
}


void Button::mousePressEvent( QMouseEvent *e )
{
	if (!e) return;
	if (!m_button->isEnabled()) return;

	QMouseEvent event{
		QEvent::MouseButtonPress,
		e->pos()-QPoint(int(x()),int(y())),
		e->button(),
    e->buttons(),
		e->modifiers()
	};
	m_button->mousePressEvent(&event);
	if (event.isAccepted())
		e->accept();
	canvas()->setChanged( rect() );
}


void Button::mouseReleaseEvent( QMouseEvent *e )
{
	if (!e) return;
	if (!m_button->isEnabled()) return;

	QMouseEvent event{
		QEvent::MouseButtonRelease,
		e->pos()-QPoint(int(x()),int(y())),
		e->button(),
    e->buttons(),
		e->modifiers()
  };
	m_button->mouseReleaseEvent(&event);
	if (event.isAccepted())
		e->accept();
	canvas()->setChanged( rect() );
}


void Button::enterEvent(QEvent *)
{
	m_button->enterEvent(nullptr);
}

void Button::leaveEvent(QEvent *)
{
	m_button->leaveEvent(nullptr);
}
//END Class Button


//BEGIN Class SliderWidget
SliderWidget::SliderWidget( QWidget *parent )
	: QSlider(parent)
{
	//setWFlags(Qt::WNoAutoErase|Qt::WRepaintNoErase);
    //setWindowFlags(/*Qt::WNoAutoErase | */ Qt::WRepaintNoErase);
}
//END Class SliderWidget


//BEGIN Class Slider
Slider::Slider( const QString &id, CNItem *parent, const QRect &r, KtlQCanvas *canvas ) :
	Widget( id, parent, r, canvas )
{
	m_slider = new SliderWidget(nullptr);
  QPalette p;
  p.setColor(m_slider->foregroundRole(), Qt::white);
	p.setColor(m_slider->backgroundRole(), Qt::transparent);
  m_slider->setPalette(p);
	connect( m_slider, SIGNAL(valueChanged(int)), this, SLOT(slotValueChanged(int)) );
	posChanged();
}


Slider::~Slider()
{
	delete m_slider;
}


QWidget * Slider::widget() const
{
	return m_slider;
}


int Slider::value() const
{
	if (m_bSliderInverted)
	{
		// Return the value as if the slider handle was reflected along through
		// the center of the slide.
		return m_slider->maximum() + m_slider->minimum() - m_slider->value();
	}

	return m_slider->value();
}

void Slider::setValue( int value )
{
	if (m_bSliderInverted)
	{
		value = m_slider->maximum() + m_slider->minimum() - value;
	}

	m_slider->setValue(value);

	auto *c = canvas();
	if (c)
		c->setChanged( rect() );
}


void Slider::mousePressEvent( QMouseEvent *e )
{
	if (!e) return;

  qDebug() << Q_FUNC_INFO << "pos " << e->pos() << " x " << int(x()) << " y " << int(y())
  	<< " b " << e->button() << " bs " << e->buttons() << " m " << e->modifiers();

	QMouseEvent event{
		QEvent::MouseButtonPress,
		e->pos()-QPoint(int(x()),int(y())),
		e->button(),
    e->buttons(),
		e->modifiers()
	};
	m_slider->mousePressEvent(&event);
	if (event.isAccepted()) {
  	qDebug() << Q_FUNC_INFO << "accepted " << e;
		e->accept();
  }
	auto *c = canvas();
	if (c)
		c->setChanged(rect());
}

void Slider::mouseReleaseEvent( QMouseEvent *e )
{
	if (!e) return;

  qDebug() << Q_FUNC_INFO << "pos " << e->pos() << " x " << int(x()) << " y " << int(y())
  	<< " b " << e->button() << " bs " << e->buttons() << " m " << e->modifiers() ;

	QMouseEvent event{
		QEvent::MouseButtonRelease,
		e->pos()-QPoint(int(x()),int(y())),
		e->button(),
		e->buttons(),
		e->modifiers()
	};
	m_slider->mouseReleaseEvent(&event);
	if (event.isAccepted()) {
  	qDebug() << Q_FUNC_INFO << "accepted " << e;
		e->accept();
  }
	auto *c = canvas();
	if (c)
		c->setChanged(rect());
}

void Slider::mouseDoubleClickEvent ( QMouseEvent *e )
{
	if (!e) return;

	QMouseEvent event{
		QEvent::MouseButtonDblClick,
		e->pos()-QPoint(int(x()),int(y())),
		e->button(),
    e->buttons(),
		e->modifiers()
	};
	m_slider->mouseDoubleClickEvent(&event);
	if (event.isAccepted())
		e->accept();

	auto *c = canvas();
	if (c)
		c->setChanged(rect());
}

void Slider::mouseMoveEvent( QMouseEvent *e )
{
	if (!e) return;

	QMouseEvent event{
		QEvent::MouseMove,
		e->pos()-QPoint(int(x()),int(y())),
		e->button(),
		e->buttons(),
		e->modifiers()
	};
	m_slider->mouseMoveEvent(&event);
	if (event.isAccepted())
		e->accept();
}

void Slider::wheelEvent( QWheelEvent *e )
{
	if (!e) return;

	QWheelEvent event{
		e->pos()-QPoint(int(x()),int(y())),
		e->delta(),
		e->buttons(),
		e->modifiers(),
		e->orientation()
	};
	m_slider->wheelEvent(&event);
	if (event.isAccepted())
		e->accept();

	auto *c = canvas();
	if (c)
		c->setChanged(rect());
}

void Slider::enterEvent(QEvent *e)
{
  qDebug() << Q_FUNC_INFO;
	m_slider->enterEvent(e);
}

void Slider::leaveEvent(QEvent *e)
{
	qDebug() << Q_FUNC_INFO;
	m_slider->leaveEvent(e);
}

void Slider::slotValueChanged( [[maybe_unused]] int value )
{
	if (parent() && parent()->itemDocument())
		parent()->itemDocument()->setModified(true);

	// Note that we do not use value as we want to take into account rotation
	parent()->sliderValueChanged(id(), this->value());

	if (canvas())
		canvas()->setChanged( rect() );
}

void Slider::setOrientation( Qt::Orientation o )
{
	m_orientation = o;
	posChanged();
}

void Slider::posChanged()
{
	Widget::posChanged();

	bool nowInverted;

	if ( m_orientation == Qt::Vertical )
	{
		nowInverted = angleDegrees() == 90 || angleDegrees() == 180;
		m_slider->setOrientation( (m_angleDegrees%180 == 0) ? Qt::Vertical : Qt::Horizontal );
	}

	else
	{
		nowInverted = angleDegrees() == 0 || angleDegrees() == 90;
		m_slider->setOrientation( (m_angleDegrees%180 == 0) ? Qt::Horizontal : Qt::Vertical );
	}

	if ( nowInverted != m_bSliderInverted )
	{
		auto prevValue = value();
		m_bSliderInverted = nowInverted;
		setValue(prevValue);
	}
}
//END Class Slider


#include "moc_canvasitemparts.cpp"
