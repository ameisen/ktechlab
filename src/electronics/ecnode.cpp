/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <algorithm>
#include <memory>

#include "circuitdocument.h"
#include "component.h"
#include "connector.h"
#include "ecnode.h"
#include "electronicconnector.h"
#include "pin.h"

#include <qdebug.h>
#include <qpainter.h>

#include <ktlconfig.h>

ECNode::ECNode( ICNDocument *icnDocument, Node::node_type _type, int dir, const QPoint &pos, QString *_id ) :
	Node( icnDocument, _type, dir, pos, _id ),
	m_bShowVoltageBars(KTLConfig::showVoltageBars()),
	m_bShowVoltageColor(KTLConfig::showVoltageColor())
{
	if ( icnDocument )
		icnDocument->registerItem(this);

	m_pins.push_back(new Pin(this));
}

ECNode::~ECNode()
{
	if (m_pinPoint) {
		m_pinPoint->setCanvas(nullptr);
		delete m_pinPoint;
	}

	for (auto &pin : m_pins) {
			delete pin.data();
	}
}


void ECNode::setNumPins( unsigned num )
{
	auto oldNum = unsigned(m_pins.size());

	if ( num == oldNum ) return;

	if ( num > oldNum ) {
		m_pins.resize(num);
		for ( unsigned i = oldNum; i < num; i++ )
			m_pins[i] = new Pin(this);
	}
	else {
		for ( unsigned i = num; i < oldNum; i++ )
			delete m_pins[i];
		m_pins.resize(num);
	}

	emit numPinsChanged(num);
}

Pin *ECNode::pin( unsigned num ) const
{
	return (llong(num) < m_pins.size()) ?
		m_pins[num] :
		nullptr;
}

void ECNode::setNodeChanged()
{
	if ( !canvas() || numPins() != 1 ) return;

	auto &pin = m_pins[0];

	double v = pin->voltage();
	double i = pin->current();

	if ( v != m_prevV || i != m_prevI ) {
		auto r = boundingRect();
		canvas()->setChanged(r);
		m_prevV = v;
		m_prevI = i;
	}
}


void ECNode::setParentItem( CNItem * parentItem )
{
	Node::setParentItem(parentItem);

	if ( auto *component = dynamic_cast<Component*>(parentItem) ) {
		connect( component, SIGNAL(elementDestroyed(Element* )), this, SLOT(removeElement(Element* )) );
		connect( component, SIGNAL(switchDestroyed( Switch* )), this, SLOT(removeSwitch( Switch* )) );
	}
}


void ECNode::removeElement( Element * e )
{
	for (auto &pin : m_pins) {
		pin->removeElement(e);
	}
}


void ECNode::removeSwitch( Switch * sw )
{
	for (auto &pin : m_pins) {
		pin->removeSwitch(sw);
	}
}


// -- functionality from node.cpp --

bool ECNode::isConnected( Node *node, QPtrList<Node> *checkedNodes )
{
	if ( this == node )
		return true;

	// This way, it will automatically delete when this function exits.
	std::unique_ptr<QPtrList<Node>> localCheckedNodes;

	if (!checkedNodes) {
		checkedNodes = new QPtrList<Node>();
		localCheckedNodes.reset(checkedNodes);
	}
	else if ( checkedNodes->contains(this) )
		return false;

	checkedNodes->append(this);

	for (auto &connector : m_connectorList)
	{
		if (!connector) continue;

		auto *startNode = connector->startNode();
		if ( startNode && startNode->isConnected( node, checkedNodes ) ) {
			return true;
		}
	}

	return false;
}

void ECNode::checkForRemoval( Connector *connector )
{
	removeConnector(connector);
	setNodeSelected(false);

	removeNullConnectors();

	if ( !p_parentItem && m_connectorList.empty() ) {
		removeNode();
	}
}

void ECNode::setVisible( bool visible )
{
	if ( isVisible() == visible ) return;

	KtlQCanvasPolygon::setVisible(visible);

	for (auto &connector : m_connectorList) {
		if (!connector) continue;

		if ( isVisible() ) {
			connector->setVisible(true);
		}
		else {
			auto *node = connector->startNode();
			connector->setVisible( node && node->isVisible() );
		}
	}
}

QPoint ECNode::findConnectorDivergePoint( bool * found ) // TODO : Really? A pointer to a boolean?
{
	// FIXME someone should check that this function is OK ... I just don't understand what it does
	if (found)
		*found = false;

	if ( numCon( false, false ) != 2 )
		return QPoint{};

	QList<QPoint> p1;
	QList<QPoint> p2;

	const auto inSize = m_connectorList.count();

	bool gotP1 = false;
	bool gotP2 = false;

	int at = -1;
	for (auto &connector : m_connectorList)
	{
		at++;

		if ( !connector || !connector->canvas() )
			continue;

		if (gotP1) {
			p2 = connector->connectorPoints( at < inSize );
			gotP2 = true;
			break;
		}
		else {
			p1 = connector->connectorPoints( at < inSize );
			gotP1 = true;
		}
	}

	if ( !gotP1 || !gotP2 )
		return QPoint{};

	int maxLength = std::max(p1.size(), p2.size());

	for ( int i = 1; i < maxLength; ++i )
	{
		if ( p1[i] != p2[i] ) {
			if (found)
				*found = true;
			return p1[i - 1];
		}
	}
	return QPoint{};
}

void ECNode::addConnector( Connector * const connector )
{
	if ( !handleNewConnector(connector) )
		return;

	m_connectorList.append(connector);
}

bool ECNode::handleNewConnector( Connector * connector )
{
	if (!connector)
		return false;

	if ( m_connectorList.contains(connector) )
	{
		qWarning() << Q_FUNC_INFO << " Already have connector = " << connector << endl;
		return false;
	}

	connect( this, SIGNAL(removed(Node*)), connector, SLOT(removeConnector(Node*)) );
	connect( connector, SIGNAL(removed(Connector*)), this, SLOT(checkForRemoval(Connector*)) );
	connect( connector, SIGNAL(selected(bool)), this, SLOT(setNodeSelected(bool)) );

	if ( !isChildNode() )
		p_icnDocument->slotRequestAssignNG();

	return true;
}

Connector* ECNode::createConnector( Node * node )
{
	// FIXME dynamic_cast used
	auto *connector = new ElectronicConnector(
		dynamic_cast<ECNode*>(node),
		dynamic_cast<ECNode*>(this),
		p_icnDocument
	);
	addConnector(connector);

	return connector;
}

void ECNode::removeNullConnectors()
{
	m_connectorList.removeAll(nullptr);
}

int ECNode::numCon( bool includeParentItem, bool includeHiddenConnectors ) const
{
	int count = 0;

	for (auto &connector : m_connectorList)
	{
		if ( connector && (includeHiddenConnectors || connector->canvas()) )
			++count;
	}


	if ( isChildNode() && includeParentItem )
		++count;

	return count;
}

void ECNode::removeConnector( Connector *connector )
{
	if (!connector) return;

  const int i = m_connectorList.indexOf(connector);
	if ( i != -1 )
	{
		connector->removeConnector();
		m_connectorList[i] = nullptr;
	}
}

Connector* ECNode::getAConnector() const
{
	return !m_connectorList.isEmpty() ?
		m_connectorList.first() :
		nullptr;
}

#include "moc_ecnode.cpp"
