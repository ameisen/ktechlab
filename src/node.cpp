/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "cnitem.h"
#include "icndocument.h"
#include "connector.h"
#include "itemdocumentdata.h"
#include "node.h"

#include <qdebug.h>

#include <qpainter.h>

const QColor Node::m_selectedColor = QColor( 101, 134, 192 );

Node::Node( ICNDocument *icnDocument, Node::node_type type, int dir, const QPoint &pos, QString *id ) :
	KtlQCanvasPolygon( icnDocument ? icnDocument->canvas() : nullptr ),
	m_type(type),
	m_dir(dir),
	p_icnDocument(icnDocument)
{
	QString name("Node");
	if (id) {
			name.append(QString("-%1").arg(*id));
	}
	else {
			name.append("-Unknown");
	}
	setObjectName(name.toLatin1().data());
	qDebug() << Q_FUNC_INFO << " this=" << this;

	if ( p_icnDocument ) {
		if (id) {
			m_id = *id;
			if ( !p_icnDocument->registerUID(*id) )
				qCritical() << Q_FUNC_INFO << "Could not register id " << *id << endl;
		}
		else {
			m_id = p_icnDocument->generateUID("node"+QString::number(type));
		}
	}

	initPoints();
	move( pos.x(), pos.y() );
	setBrush( Qt::black );
	setPen( QPen( Qt::black ) );
	show();

	emit (moved(this));
}

Node::~Node()
{
	if ( p_icnDocument )
		p_icnDocument->unregisterUID( id() );
}

void Node::setLevel( const int level )
{
	m_level = level;
}

void Node::setLength( int length )
{
	if ( m_length == length )
		return;
	m_length = length;
	initPoints();
}

void Node::setOrientation( int dir )
{
	if ( m_dir == dir )
		return;
	m_dir = dir;
	initPoints();
}

void Node::initPoints()
{
	// Bounding rectangle, facing right
	QPolygon pa( QRect( 0, -8, m_length, 16 ) );

	QMatrix m;
	m.rotate( m_dir );
	pa = m.map(pa);
	setPoints(pa);
}

void Node::setVisible( bool visible )
{
	if (isVisible() == visible) return;

	KtlQCanvasPolygon::setVisible(visible);
}

void Node::setParentItem( CNItem *parentItem )
{
	if (!parentItem) {
		qCritical() << Q_FUNC_INFO << "no parent item" << endl;
		setLevel(0);
		return;
	}

	p_parentItem = parentItem;

	setLevel(p_parentItem->level());

	connect( p_parentItem, SIGNAL(movedBy(double, double )), this, SLOT(moveBy(double, double)) );
	connect( p_parentItem, SIGNAL(removed(Item*)), this, SLOT(removeNode(Item*)) );
}

void Node::removeNode()
{
	if (b_deleted) return;
	b_deleted = true;

	emit removed(this);
	p_icnDocument->appendDeleteList(this);
}

void Node::setICNDocument(ICNDocument *documentPtr) {
    p_icnDocument = documentPtr;
}

void Node::moveBy( double dx, double dy )
{
	if ( dx == 0 && dy == 0 ) return;
	KtlQCanvasPolygon::moveBy( dx, dy );
	emit moved(this);
}

NodeData Node::nodeData() const
{
	return {
		x(),
		y()
	};
}

void Node::setNodeSelected( bool selected )
{
	if ( isSelected() == selected )
		return;

	KtlQCanvasItem::setSelected(selected);

	const auto color = selected ? m_selectedColor : Qt::black;
	setPen(color);
	setBrush(color);
}

void Node::initPainter( QPainter & p )
{
	p.translate( int(x()), int(y()) );
	p.rotate( m_dir );
}


void Node::deinitPainter( QPainter & p )
{
	p.rotate( -m_dir );
	p.translate( -int(x()), -int(y()) );
}

#include "moc_node.cpp"
