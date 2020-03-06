/***************************************************************************
 *   Copyright (C) 2004-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "icndocument.h"
#include "connector.h"
#include "conrouter.h"
#include "node.h"
#include "nodegroup.h"

#include <qdebug.h>
#include <cassert>
#include <cstdlib>

NodeGroup::NodeGroup( ICNDocument *icnDocument, const char *name ) :
	QObject(icnDocument),
	p_icnDocument(icnDocument)
{
  setObjectName(name);
}


NodeGroup::~NodeGroup()
{
	clearConList();

	for (auto &node : m_extNodeList) {
		if (node)
			node->setNodeGroup(nullptr);
	}

	for (auto &node : m_nodeList) {
		if (node)
			node->setNodeGroup(nullptr);
	}
}


void NodeGroup::setVisible( bool visible )
{
	if ( b_visible == visible )
		return;

	b_visible = visible;

	m_nodeList.removeAll(nullptr);
	for (auto &node : m_nodeList) {
		node->setVisible(visible);
	}
}


void NodeGroup::addNode( Node *node, bool checkSurrouding )
{
	if ( !node || node->isChildNode() || m_nodeList.contains(node) ) {
		return;
	}

	m_nodeList.append(node);
	node->setNodeGroup(this);
	node->setVisible(b_visible);

	if (checkSurrouding)
	{
		const auto connectors = node->getAllConnectors();
		for (auto &connector : connectors) {
			if (!connector)
				continue;
			// maybe we can put here a check, because only 1 of there checks should pass
			if ( connector->startNode() != node )
				addNode( connector->startNode(), true );
			if ( connector->endNode() != node )
				addNode( connector->endNode(), true );
		}

	}
}


void NodeGroup::translate( int dx, int dy )
{
	if (dx == 0 && dy == 0)
		return;

	m_conList.removeAll(nullptr);
	for (auto &connector : m_conList) {
		connector->updateConnectorPoints(false);
		connector->translateRoute(dx, dy);
	}

	m_nodeList.removeAll(nullptr);
	for (auto &node : m_nodeList) {
		node->moveBy(dx, dy);
	}
}


void NodeGroup::updateRoutes()
{
	resetRoutedMap();

	// Basic algorithm used here: starting with the external nodes, find the
	// pair with the shortest distance between them. Route the connectors
	// between the two nodes appropriately. Remove that pair of nodes from the
	// list, and add the nodes along the connector route (which have been spaced
	// equally along the route). Repeat until all the nodes are connected.

	for (auto &connector : m_conList) {
		if (connector) {
			connector->updateConnectorPoints(false);
		}
	}

	QPtrList<Node> currentList = m_extNodeList;
	while ( !currentList.isEmpty() )
	{
		Node *n1, *n2;
		findBestPair(currentList, &n1, &n2);
		if (!n1 || !n2) {
			return;
		}
		auto route = findRoute(n1, n2);
		currentList += route;

		ConRouter cr{p_icnDocument};
		cr.mapRoute( (int)n1->x(), (int)n1->y(), (int)n2->x(), (int)n2->y() );
    if (cr.pointList(false).isEmpty()) {
      qDebug() << Q_FUNC_INFO << "no ConRouter points, giving up";
      return; // continue might get to an infinite loop
    }
		auto pl = cr.dividePoints( route.size() + 1 );

		const QPtrList<Node>::iterator routeEnd = route.end();
		Node *prev = n1;
		QPtrList<Node>::iterator routeIt = route.begin();
		for (auto &pointList : pl)
		{
			Node *next = (routeIt == routeEnd) ? n2 : (Node *)*(routeIt++);
			removeRoutedNodes( &currentList, prev, next );
			if ( prev != n1 && !pointList.isEmpty() )
			{
				auto &first = pointList.first();
				prev->moveBy( first.x() - prev->x(), first.y() - prev->y() );
			}
			auto con = findCommonConnector(prev, next);
			if (con)
			{
				con->updateConnectorPoints(false);
				con->setRoutePoints(pointList, false, false);
				con->updateConnectorPoints(true);
			}
			prev = next;
		}
	}
}


QPtrList<Node> NodeGroup::findRoute( Node *startNode, Node *endNode )
{
	if ( !startNode || !endNode || startNode == endNode ) {
		return QPtrList<Node>{};
	}

	IntList temp;
	IntList il = findRoute( temp, getNodePos(startNode), getNodePos(endNode) );

	QPtrList<Node> nl;
	for (auto value : il)
	{
		Node *node = getNodePtr(value);
		if (node) {
			nl += node;
		}
	}

	nl.removeAll(startNode);
	nl.removeAll(endNode);

	return nl;
}


IntList NodeGroup::findRoute(IntList used, int currentNode, int endNode, bool *success)
{
	bool temp;
	if (!success) {
		success = &temp;
	}
	*success = false;

	if (!used.contains(currentNode)) {
		used.append(currentNode);
	}

	if (currentNode == endNode) {
		*success = true;
		return used;
	}

	const int n = m_nodeList.size() + m_extNodeList.size();
	for ( int i = 0; i<n; ++i )
	{
		if (b_routedMap[i * n + currentNode] && !used.contains(i))
		{
			IntList il = findRoute(used, i, endNode, success);
			if (*success) {
				return il;
			}
		}
	}

	return IntList{};
}


Connector* NodeGroup::findCommonConnector( Node *n1, Node *n2 )
{
	if ( !n1 || !n2 || n1 == n2 ) {
		return nullptr;
	}

	const auto n1Con = n1->getAllConnectors();
	const auto n2Con = n2->getAllConnectors();

	for (auto &connector : n1Con)
	{
		if (!connector) continue;
		if (n2Con.contains(connector)) {
			return connector;
		}
	}
	return nullptr;
}


void NodeGroup::findBestPair( const QPtrList<Node> &list, Node **n1, Node **n2 )
{
	*n1 = nullptr;
	*n2 = nullptr;

	if (list.size() < 2) {
		return;
	}

	const auto end = list.end();
	int shortest = 1 << 30;

	// Try and find any that are aligned horizontally
	for (auto it1 = list.begin(); it1 != end; ++it1)
	{
		auto it2 = it1;
		for (++it2; it2 != end; ++it2)
		{
			if ( *it1 != *it2 && (*it1)->y() == (*it2)->y() && canRoute( *it1, *it2 ) )
			{
				const int distance = std::abs(int( (*it1)->x()-(*it2)->x() ));
				if ( distance < shortest )
				{
					shortest = distance;
					*n1 = *it1;
					*n2 = *it2;
				}
			}
		}
	}
	if (*n1) {
		return;
	}

	// Try and find any that are aligned vertically
	for (auto it1 = list.begin(); it1 != end; ++it1 )
	{
		auto it2 = it1;
		for (++it2; it2 != end; ++it2)
		{
			if ( *it1 != *it2 && (*it1)->x() == (*it2)->x() && canRoute( *it1, *it2 ) )
			{
				const int distance = std::abs(int( (*it1)->y()-(*it2)->y() ));
				if ( distance < shortest )
				{
					shortest = distance;
					*n1 = *it1;
					*n2 = *it2;
				}
			}
		}
	}
	if (*n1) {
		return;
	}

	// Now, lets just find the two closest nodes
	for (auto it1 = list.begin(); it1 != end; ++it1 )
	{
		auto it2 = it1;
		for (++it2; it2 != end; ++it2)
		{
			if (*it1 != *it2 && canRoute( *it1, *it2 ))
			{
				const int dx = (int)((*it1)->x()-(*it2)->x());
				const int dy = (int)((*it1)->y()-(*it2)->y());
				const int distance = std::abs(dx) + std::abs(dy);
				if ( distance < shortest )
				{
					shortest = distance;
					*n1 = *it1;
					*n2 = *it2;
				}
			}
		}
	}

	if (!*n1) {
		qCritical() << "NodeGroup::findBestPair: Could not find a routable pair of nodes!"<<endl;
	}
}


bool NodeGroup::canRoute(Node *n1, Node *n2)
{
	if ( !n1 || !n2 ) {
		return false;
	}

	IntList reachable;
	getReachable( &reachable, getNodePos(n1) );
	return reachable.contains(getNodePos(n2));
}


void NodeGroup::getReachable(IntList *reachable, int node)
{
	if ( !reachable || reachable->contains(node) ) {
		return;
	}
	reachable->append(node);

	const int n = m_nodeList.size() + m_extNodeList.size();
	assert( node < n );
	for ( int i = 0; i < n; ++i )
	{
		if ( b_routedMap[i*n + node] ) {
			getReachable(reachable,i);
		}
	}
}


void NodeGroup::resetRoutedMap()
{
	const int n = m_nodeList.size() + m_extNodeList.size();

	b_routedMap.resize( n * n );

	for (auto &connector : m_conList)
	{
		if (!connector) continue;

		int n1 = getNodePos(connector->startNode());
		int n2 = getNodePos(connector->endNode());
		if (n1 != -1 && n2 != -1)
		{
			b_routedMap[n1*n + n2] = b_routedMap[n2*n + n1] = true;
		}
	}
}


void NodeGroup::removeRoutedNodes( QPtrList<Node> *nodes, Node *n1, Node *n2 )
{
	if (!nodes) {
		return;
	}

	// Lets get rid of any duplicate nodes in nodes (as a general cleaning operation)
	for (auto &&node : *nodes)
	{
		if (!node) continue;
  	if (nodes->count(node) > 1) {
			node = nullptr;
		}
	}
	nodes->removeAll(nullptr);

	const int n1pos = getNodePos(n1);
	const int n2pos = getNodePos(n2);

	if ( n1pos == -1 || n2pos == -1 ) {
		return;
	}

	const int n = m_nodeList.size() + m_extNodeList.size();

	b_routedMap[n1pos*n + n2pos] = b_routedMap[n2pos*n + n1pos] = false;

	bool n1HasCon = false;
	bool n2HasCon = false;

	for ( int i = 0; i < n; ++i )
	{
		n1HasCon |= b_routedMap[n1pos*n + i];
		n2HasCon |= b_routedMap[n2pos*n + i];
	}

	if (!n1HasCon) {
		nodes->removeAll(n1);
	}
	if (!n2HasCon) {
		nodes->removeAll(n2);
	}
}


int NodeGroup::getNodePos( Node *n )
{
	if (!n) {
		return -1;
	}
	int pos = m_nodeList.indexOf(n);
	if ( pos != -1 ) {
		return pos;
	}
	pos = m_extNodeList.indexOf(n);
	if ( pos != -1 ) {
		return pos+m_nodeList.size();
	}
	return -1;
}


Node* NodeGroup::getNodePtr( int n )
{
	if ( n<0 ) {
		return nullptr;
	}
	const int a = m_nodeList.size();
	if (n<a) {
		return m_nodeList[n];
	}
	const int b = m_extNodeList.size();
	if (n<a+b) {
		return m_extNodeList[n-a];
	}
	return nullptr;
}


void NodeGroup::clearConList()
{
	for (auto &connector : m_conList)
	{
		if (!connector) continue;
		connector->setNodeGroup(nullptr);
		disconnect( connector, SIGNAL(removed(Connector*)), this, SLOT(connectorRemoved(Connector*)) );
	}
	m_conList.clear();
}


void NodeGroup::init()
{
	for (auto &node : m_extNodeList)
	{
		if (!node) continue;
		node->setNodeGroup(nullptr);
	}
	m_extNodeList.clear();

	clearConList();

	// First, lets get all of our external nodes and internal connectors
	for (auto &node : m_nodeList)
	{
		if (!node) continue;
		// 2. rewrite
		const auto conList = node->getAllConnectors();

		for (auto &connector : conList)
		{
			if (!connector) continue;

			// possible check: only 1 of these ifs should be true
			if ( connector->startNode() != node )
			{
				addExtNode ( connector->startNode() );
				if ( !m_conList.contains ( connector ) )
				{
					m_conList += connector;
					connector->setNodeGroup ( this );
				}
			}
			if ( connector->endNode() != node )
			{
				addExtNode ( connector->endNode() );
				if ( !m_conList.contains ( connector ) )
				{
					m_conList += connector;
					connector->setNodeGroup ( this );
				}
			}
			connect ( connector, SIGNAL ( removed ( Connector* ) ), this, SLOT ( connectorRemoved ( Connector* ) ) );
		}

		// Connect the node up to us
		connect ( node, SIGNAL ( removed ( Node* ) ), this, SLOT ( nodeRemoved ( Node* ) ) );
	}

	// And connect up our external nodes
	for (auto &node : m_extNodeList)
	{
		if (!node) continue;
		connect( node, SIGNAL(removed(Node*)), this, SLOT(nodeRemoved(Node*)) );
	}
}


void NodeGroup::nodeRemoved( Node *node )
{
	// We are probably about to get deleted by ICNDocument anyway...so no point in doing anything
	m_nodeList.removeAll(node);
	m_extNodeList.removeAll(node);
	if (node) {
		node->setNodeGroup(nullptr);
		node->setVisible(true);
	}
}


void NodeGroup::connectorRemoved( Connector *connector )
{
	m_conList.removeAll(connector);
}


void NodeGroup::addExtNode( Node *node )
{
	if (!node) return;

	if (!m_extNodeList.contains(node) && !m_nodeList.contains(node))
	{
		m_extNodeList.append(node);
		node->setNodeGroup(this);
	}
}

#include "moc_nodegroup.cpp"
