/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "canvasmanipulator.h"
#include "component.h"
#include "connector.h"
#include "conrouter.h"
#include "cnitemgroup.h"
#include "ecnode.h"
#include "junctionnode.h"
#include "flowcontainer.h"
#include "fpnode.h"
#include "junctionflownode.h"
#include "icndocument.h"
#include "icnview.h"
#include "itemdocumentdata.h"
#include "itemlibrary.h"
#include "ktechlab.h"
#include "nodegroup.h"
#include "outputflownode.h"
#include "utils.h"

#include <qdebug.h>
#include <qclipboard.h>
#include <qtimer.h>
#include <QApplication>


//BEGIN class ICNDocument
ICNDocument::ICNDocument( const QString &caption, const char *name )
	: ItemDocument( caption, name ),
	m_cells(0l)
{
	m_canvas->retune(48);
	m_selectList = new CNItemGroup(this);

	createCellMap();

	m_cmManager->addManipulatorInfo( CMItemMove::manipulatorInfo() );
	m_cmManager->addManipulatorInfo( CMAutoConnector::manipulatorInfo() );
	m_cmManager->addManipulatorInfo( CMManualConnector::manipulatorInfo() );
}


ICNDocument::~ICNDocument()
{
	m_bDeleted = true;

	GuardedNodeGroupList ngToDelete = m_nodeGroupList;
	m_nodeGroupList.clear();
	const GuardedNodeGroupList::iterator nglEnd = ngToDelete.end();
	for ( GuardedNodeGroupList::iterator it = ngToDelete.begin(); it != nglEnd; ++it )
		delete (NodeGroup *)(*it);

	delete m_cells;
	delete m_selectList;
}


View *ICNDocument::createView( ViewContainer *viewContainer, uint viewAreaId, const char *name )
{
	ICNView *icnView = new ICNView( this, viewContainer, viewAreaId, name );
	handleNewView(icnView);
	return icnView;
}


ItemGroup* ICNDocument::selectList() const
{
	return m_selectList;
}


void ICNDocument::fillContextMenu( const QPoint &pos )
{
	ItemDocument::fillContextMenu(pos);
	slotInitItemActions();
}


CNItem* ICNDocument::cnItemWithID( const QString &id )
{
	return dynamic_cast<CNItem*>(itemWithID(id));
}



Connector *ICNDocument::connectorWithID( const QString &id )
{
	const QPtrList<Connector>::iterator end = m_connectorList.end();
	for( QPtrList<Connector>::iterator it = m_connectorList.begin(); it != end; ++it )
	{
		if( (*it)->id() == id ) return *it;
	}
	return 0;
}


FlowContainer *ICNDocument::flowContainer( const QPoint &pos )
{
	auto collisions = m_canvas->collisions(pos);
	FlowContainer *flowContainer = 0l;
	int currentLevel = -1;
	const auto end = collisions.end();
	for ( auto it = collisions.begin(); it != end; ++it )
	{
		if ( FlowContainer *container = dynamic_cast<FlowContainer*>(*it) )
		{
			if ( container->level() > currentLevel && !m_selectList->contains(container) )
			{
				currentLevel = container->level();
				flowContainer = container;
			}
		}
	}

	return flowContainer;
}


bool ICNDocument::canConnect( KtlQCanvasItem *qcanvasItem1, KtlQCanvasItem *qcanvasItem2 ) const
{
	// Rough outline of what can and can't connect:
	// * At most three connectors to a node
	// * Can't have connectors going between different levels (e.g. can't have
	//   a connector coming outside a FlowContainer from inside).
	// * Can't have more than one route between any two nodes
	// * In all connections between nodes, must have at least one input and one
	//   output node at the ends.

	Node *startNode = dynamic_cast<Node*>(qcanvasItem1);
	Node *endNode   = dynamic_cast<Node*>(qcanvasItem2);

	if ( (startNode && startNode->numCon( true, false ) > 2)
	  || (endNode && endNode->numCon(     true, false ) > 2) )
		return false;


	Connector *startConnector = dynamic_cast<Connector*>(qcanvasItem1);
	Connector *endConnector   = dynamic_cast<Connector*>(qcanvasItem2);

// FIXME: overload this instead of calling type().
	// Can't have T- or I- junction in PinMapEditor document
	if ( type() == Document::dt_pinMapEditor && (startConnector || endConnector) )
		return false;

	// Can't have I-junction in flowcode document
	if ( type() == Document::dt_flowcode && startConnector && endConnector )
		return false;


	//BEGIN Change connectors to nodes
	Node * startNode1 = 0l;
	Node * startNode2 = 0l;
	if (startConnector) {
		startNode1 = startConnector->startNode();
		startNode2 = startConnector->endNode();

		if ( !startNode1 || !startNode2 ) return false;
	} else if (!startNode) return false;

	Node * endNode1 = 0l;
	Node * endNode2 = 0l;
	if (endConnector) {
		endNode1 = endConnector->startNode();
		endNode2 = endConnector->endNode();

		if ( !endNode1 || !endNode2 ) return false;
	} else if ( !endNode ) return false;

	Node * start[3];
	start[0] = startNode;
	start[1] = startNode1;
	start[2] = startNode2;

	Node * end[3];
	end[0] = endNode;
	end[1] = endNode1;
	end[2] = endNode2;
	//END Change connectors to nodes


	//BEGIN Check nodes aren't already connected
	for ( unsigned i = 0; i < 3; i++ ) {
		for ( unsigned j = 0; j < 3; j++ )
		{
			if ( start[i] && end[j] && start[i]->isConnected(end[j]) )
				return false;
		}
	}
	//END Check nodes aren't already connected together


	//BEGIN Simple level check
	for ( unsigned i = 0; i < 3; i++ ) {
		for ( unsigned j = 0; j < 3; j++ )
		{
			if ( start[i] && end[j] && start[i]->level() != end[j]->level() )
				return false;
		}
	}
	//END Simple level check


	//BEGIN Advanced level check
	CNItem *startParentItem[3];
	for(unsigned i = 0; i < 3; i++ )
		startParentItem[i] = start[i] ? start[i]->parentItem() : 0l;

	CNItem * endParentItem[3];
	for(unsigned i = 0; i < 3; i++ )
		endParentItem[i]   = end[i]   ? end[i]->parentItem()   : 0l;

	Item *container[6] = {0l};

	for ( unsigned i = 0; i < 3; i++ ) {
		if (startParentItem[i]) {
			int dl = start[i]->level() - startParentItem[i]->level();
			if ( dl == 0 )
				container[i] = startParentItem[i]->parentItem();
			else if ( dl == 1 )
				container[i] = startParentItem[i];
			else	qCritical() << Q_FUNC_INFO << " start, i="<<i<<" dl="<<dl<<endl;
		}

		if (endParentItem[i]) {
			int dl = end[i]->level() - endParentItem[i]->level();
			if ( dl == 0 )
				container[i+3] = endParentItem[i]->parentItem();
			else if ( dl == 1 )
				container[i+3] = endParentItem[i];
			else	qCritical() << Q_FUNC_INFO << " end, i="<<i<<" dl="<<dl<<endl;
		}
	}

	// Everything better well have the same container...
	for(unsigned i = 0; i < 6; ++i ) {
		for(unsigned j = 0; j < i; ++j ) {
			Node * n1 = i < 3 ? start[i] : end[i-3];
			Node * n2 = j < 3 ? start[j] : end[j-3];
			if ( n1 && n2 && (container[i] != container[j]) )
				return false;
		}
	}
	//END Advanced level check

	// Well, it looks like we can, afterall, connect them...
	return true;
}


Connector *ICNDocument::createConnector( Node *startNode, Node *endNode, QList<QPoint> *pointList )
{
	if(!canConnect(startNode, endNode) ) return 0;

	QList<QPoint> autoPoints;
	if (!pointList) {
		addAllItemConnectorPoints();
		ConRouter cr(this);
		cr.mapRoute( int(startNode->x()), int(startNode->y()), int(endNode->x()), int(endNode->y()) );
		autoPoints = cr.pointList(false);
		pointList = &autoPoints;
	}

	Connector *con = 0;

	// Check if we need to swap the ends around, and create the connector
		// FIXME: dynamic_cast used
	if( dynamic_cast<OutputFlowNode*>(endNode) != 0l )
		con = createConnector( endNode->id(), startNode->id(), pointList );
	else    con = createConnector( startNode->id(), endNode->id(), pointList );

	bool startInGroup = deleteNodeGroup(startNode);
	bool endInGroup = deleteNodeGroup(endNode);
	if ( startInGroup || endInGroup ) {
		NodeGroup *ng = createNodeGroup(startNode);
		ng->addNode( endNode, true );
		ng->init();
	}

	flushDeleteList();
	return con;
}


NodeGroup* ICNDocument::createNodeGroup( Node *node )
{
	if ( !node || node->isChildNode() ) return 0;

	const GuardedNodeGroupList::iterator end = m_nodeGroupList.end();
	for ( GuardedNodeGroupList::iterator it = m_nodeGroupList.begin(); it != end; ++it )
	{
		if ( *it && (*it)->contains(node) ) {
			return *it;
		}
	}

	NodeGroup *group = new NodeGroup(this);
	m_nodeGroupList += group;
	group->addNode( node, true );

	return group;
}


bool ICNDocument::deleteNodeGroup( Node *node )
{
	if ( !node || node->isChildNode() ) return false;

	const GuardedNodeGroupList::iterator end = m_nodeGroupList.end();
	for ( GuardedNodeGroupList::iterator it = m_nodeGroupList.begin(); it != end; ++it )
	{
		if ( *it && (*it)->contains(node) ) {
			delete *it;
			m_nodeGroupList.erase(it);
			return true;
		}
	}

	return false;
}


void ICNDocument::slotRequestAssignNG()
{
	requestEvent( ItemDocumentEvent::UpdateNodeGroups );
}


void ICNDocument::slotAssignNodeGroups()
{
	const GuardedNodeGroupList::iterator nglEnd = m_nodeGroupList.end();
	for ( GuardedNodeGroupList::iterator it = m_nodeGroupList.begin(); it != nglEnd; ++it )
		delete *it;
	m_nodeGroupList.clear();


}


void ICNDocument::getTranslatable(const QPtrList<Item> &itemList, QPtrList<Connector> *fixedConnectors, QPtrList<Connector> *translatableConnectors, 					  QPtrList<NodeGroup> *translatableNodeGroups )
{
	QPtrList<Connector> tempCL1;
	if ( !fixedConnectors )
		fixedConnectors = &tempCL1;

	QPtrList<Connector> tempCL2;
	if ( !translatableConnectors )
		translatableConnectors = &tempCL2;

	QPtrList<NodeGroup> tempNGL;
	if ( !translatableNodeGroups )
		translatableNodeGroups = &tempNGL;

	// We record the connectors attached to the items, and
	// the number of times an item in the list is connected to
	// it - i.e. 1 or 2. For those with 2, it is safe to update their
	// route as it simply involves shifting the route
	typedef QMap< Connector*, int > ConnectorMap;
	ConnectorMap fixedConnectorMap;

	// This list of nodes is built up, used for later in determining fixed NodeGroups
	QPtrList<Node> itemNodeList;

	{
		const QPtrList<Item>::const_iterator itemListEnd = itemList.end();
		for ( QPtrList<Item>::const_iterator it = itemList.begin(); it != itemListEnd; ++it )
		{
			CNItem *cnItem = dynamic_cast<CNItem*>((Item*)*it);
			if(!cnItem || !cnItem->canvas() ) continue;

			NodeInfoMap nodeMap = cnItem->nodeMap();

			const NodeInfoMap::iterator nlEnd = nodeMap.end();
			for ( NodeInfoMap::iterator nlIt = nodeMap.begin(); nlIt != nlEnd; ++nlIt )
			{
				itemNodeList.append(nlIt.value().node);
			}

			QPtrList<Connector> conList = cnItem->connectorList();
			conList.removeAll((Connector*)0);

			const QPtrList<Connector>::iterator clEnd = conList.end();
			for ( QPtrList<Connector>::iterator clit = conList.begin(); clit != clEnd; ++clit )
			{
				ConnectorMap::iterator cit = fixedConnectorMap.find(*clit);
				if ( cit != fixedConnectorMap.end() ) {
					cit.value()++;
				} else	fixedConnectorMap[*clit] = 1;
			}
		}
	}

	// We now look through the NodeGroups to see if we have all the external
	// nodes for a given nodeGroup - if so, then the connectors in the fixed
	// connectors are ok to be moved
	QPtrList<Connector> fixedNGConnectors;
	{
		translatableNodeGroups->clear();

		const GuardedNodeGroupList::const_iterator end = m_nodeGroupList.end();
		for ( GuardedNodeGroupList::const_iterator it = m_nodeGroupList.begin(); it != end; ++it )
		{
			NodeGroup *ng = *it;
			if (!ng) continue;

			QPtrList<Node> externalNodeList = ng->externalNodeList();
			const QPtrList<Node>::iterator itemNodeListEnd = itemNodeList.end();
			for ( QPtrList<Node>::iterator inlIt = itemNodeList.begin(); inlIt != itemNodeListEnd; ++inlIt )
				externalNodeList.removeAll(*inlIt);

			if ( externalNodeList.isEmpty() )
			{
				translatableNodeGroups->append(ng);

				const QPtrList<Connector> ngConnectorList = ng->connectorList();
				const QPtrList<Connector>::const_iterator ngConnectorListEnd = ngConnectorList.end();
				for ( QPtrList<Connector>::const_iterator ngclIt = ngConnectorList.begin(); ngclIt != ngConnectorListEnd; ++ngclIt )
				{
					if (*ngclIt)
						fixedNGConnectors += *ngclIt;
				}
			}
		}
	}

	translatableConnectors->clear();

	const ConnectorMap::iterator fcEnd = fixedConnectorMap.end();
	for ( ConnectorMap::iterator it = fixedConnectorMap.begin(); it != fcEnd; ++it )
	{
		// We allow it to be fixed if it is connected to two of the CNItems in the
		// select list, or is connected to itself (hence only appears to be connected to one,
		// but is fixed anyway
		Node *startNode = it.key()->endNode();
		Node *endNode = it.key()->startNode();

		if( (it.value() > 1)
		 || (startNode && endNode && startNode->parentItem() == endNode->parentItem()) )
		{
			translatableConnectors->append( const_cast<Connector*>(it.key()) );
		} else if ( !fixedNGConnectors.contains(it.key()) && !fixedConnectors->contains(it.key()) ) {
			fixedConnectors->append(it.key());
		}
	}
}


void ICNDocument::addCPenalty( int x, int y, int score )
{
	if ( m_cells->haveCell( x, y ) )
		m_cells->cell( x, y ).Cpenalty += score;
}


void ICNDocument::createCellMap()
{
	const ItemMap::iterator ciEnd = m_itemList.end();
	for ( ItemMap::iterator it = m_itemList.begin(); it != ciEnd; ++it ) {
		if ( CNItem *cnItem = dynamic_cast<CNItem*>(*it) )
			cnItem->updateConnectorPoints(false);
	}

	const QPtrList<Connector>::iterator conEnd = m_connectorList.end();
	for ( QPtrList<Connector>::iterator it = m_connectorList.begin(); it != conEnd; ++it ) {
		(*it)->updateConnectorPoints(false);
	}

	delete m_cells;

	m_cells = new Cells( canvas()->rect() );

	for ( QPtrList<Connector>::iterator it = m_connectorList.begin(); it != conEnd; ++it )
		(*it)->updateConnectorPoints(true);
}


int ICNDocument::gridSnap( int pos )
{
	return snapToCanvas( pos );
}


QPoint ICNDocument::gridSnap( const QPoint &pos )
{
	return QPoint( snapToCanvas( pos.x() ), snapToCanvas( pos.y() ) );
}


void ICNDocument::appendDeleteList( KtlQCanvasItem *qcanvasItem )
{
	if ( !qcanvasItem || m_itemDeleteList.indexOf(qcanvasItem) != -1 )
		return;

	m_itemDeleteList.append(qcanvasItem);

/* the issue here is that we don't seem to have a generic call for all of these so we have to
spend time figuring out which method to call...
*/

	if ( Node *node = dynamic_cast<Node*>(qcanvasItem) )
		node->removeNode();
	else if ( Item *item = dynamic_cast<Item*>(qcanvasItem) )
		item->removeItem();
	else {
		Connector * connector = dynamic_cast<Connector*>(qcanvasItem);
		if (!connector) {
			if ( ConnectorLine * cl = dynamic_cast<ConnectorLine*>(qcanvasItem) )
				connector = cl->parent();
		}

		if(connector) connector->removeConnector();
		else qWarning() << Q_FUNC_INFO << "unrecognised KtlQCanvasItem " << qcanvasItem << endl;
	}
}


bool ICNDocument::registerItem( KtlQCanvasItem *qcanvasItem )
{
	if (!qcanvasItem) return false;

	if ( !ItemDocument::registerItem(qcanvasItem) ) {
		if ( dynamic_cast<Node*>(qcanvasItem) ) {
			/*
			m_nodeList[ node->id() ] = node;
			emit nodeAdded(node);
			*/
			qCritical() << Q_FUNC_INFO << "BUG: this member should have been overridden!" << endl;

		} else if ( Connector *connector = dynamic_cast<Connector*>(qcanvasItem) ) {
			m_connectorList.append(connector);
			emit connectorAdded(connector);
		} else {
			qCritical() << Q_FUNC_INFO << "Unrecognised item"<<endl;
			return false;
		}
	}

	requestRerouteInvalidatedConnectors();

	return true;
}


void ICNDocument::copy()
{
	if(m_selectList->isEmpty()) return;

	ItemDocumentData data( type() );

	// We only want to copy the connectors who have all ends attached to something in the selection
	QPtrList<Connector> connectorList = m_selectList->connectors(false);

	typedef QMap< Node*, QPtrList<Connector> > NCLMap;
	NCLMap nclMap;

	QPtrList<Connector>::iterator end = connectorList.end();
	for ( QPtrList<Connector>::iterator it = connectorList.begin(); it != end; ++it ) {
		Node *startNode = (*it)->startNode();
		if ( startNode && !startNode->isChildNode() )
			nclMap[startNode].append(*it);

		Node *endNode = (*it)->endNode();
		if ( endNode && !endNode->isChildNode() )
			nclMap[endNode].append(*it);
	}

	QPtrList<Node> nodeList;
	// Remove those connectors (and nodes) which are dangling on an orphan node
	NCLMap::iterator nclEnd = nclMap.end();
	for ( NCLMap::iterator it = nclMap.begin(); it != nclEnd; ++it ) {
		if ( it.value().size() > 1 )
			nodeList.append(it.key());
		else if ( it.value().size() > 0 )
			connectorList.removeAll( it.value().at(0) );
	}

	data.addItems( m_selectList->items(false) );
	data.addNodes( nodeList );
	data.addConnectors( connectorList );

	QApplication::clipboard()->setText( data.toXML(), QClipboard::Clipboard );
}


void ICNDocument::selectAll()
{
	selectAllNodes();

	const ItemMap::iterator itemEnd = m_itemList.end();
	for ( ItemMap::iterator itemIt = m_itemList.begin(); itemIt != itemEnd; ++itemIt ) {

		if (*itemIt) select(*itemIt);
	}

	const QPtrList<Connector>::iterator conEnd = m_connectorList.end();
	for ( QPtrList<Connector>::iterator connectorIt = m_connectorList.begin(); connectorIt != conEnd; ++connectorIt ) {

		if (*connectorIt) select(*connectorIt);
	}
}


Item* ICNDocument::addItem( const QString &id, const QPoint &p, bool newItem )
{
	if ( !isValidItem(id) ) return 0;

	// First, we need to tell all containers to go to full bounding so that
	// we can detect a "collision" with them
	const ItemMap::iterator end = m_itemList.end();
	for ( ItemMap::iterator it = m_itemList.begin(); it != end; ++it )
	{
		if ( FlowContainer *flowContainer = dynamic_cast<FlowContainer*>(*it) )
			flowContainer->setFullBounds(true);
	}

	auto preCollisions = canvas()->collisions(p);
	for ( ItemMap::iterator it = m_itemList.begin(); it != end; ++it )
	{
		if ( FlowContainer *flowContainer = dynamic_cast<FlowContainer*>(*it) )
			flowContainer->setFullBounds(false);
	}

	Item *item = itemLibrary()->createItem( id, this, newItem );
	if (!item) return 0L;

	// Look through the CNItems at the given point (sorted by z-coordinate) for
	// a container item.
	FlowContainer *container = 0l;
	const auto pcEnd = preCollisions.end();
	for ( auto it = preCollisions.begin(); it != pcEnd && !container; ++it )
	{
		if ( FlowContainer *flowContainer = dynamic_cast<FlowContainer*>(*it) )
			container = flowContainer;
	}

	// We want to check it is not a special item first as
	// isValidItem may prompt the user about his bad choice
	if ( !isValidItem(item) ) {
		item->removeItem();
		delete item;
		flushDeleteList();
		return 0L;
	}

	int x = int(p.x());
	int y = int(p.y());

	if( x < 16 || x > (canvas()->width( )) )
		x = 16;
	if( y < 16 || y > (canvas()->height()) )
		y = 16;

	if ( CNItem *cnItem = dynamic_cast<CNItem*>(item) ) {
		cnItem->move( snapToCanvas(p.x()), snapToCanvas(p.y()) );

		if (container) container->addChild(cnItem);

	} else item->move( x, y );

	registerItem(item);

	item->show();
	requestStateSave();
	return item;
}


void ICNDocument::addAllItemConnectorPoints()
{
	// FIXME The next line crashes sometimes??!
	const ItemMap::iterator ciEnd = m_itemList.end();
	for ( ItemMap::iterator it = m_itemList.begin(); it != ciEnd; ++it )
	{
		if ( CNItem *cnItem = dynamic_cast<CNItem*>(*it) )
			cnItem->updateConnectorPoints(true);
	}
}


void ICNDocument::requestRerouteInvalidatedConnectors()
{
	requestEvent( ItemDocumentEvent::RerouteInvalidatedConnectors );
}


void ICNDocument::rerouteInvalidatedConnectors()
{
	//qApp->processEvents(QEventLoop::AllEvents, 300); // 2015.07.07 - do not process events, if it is not urgently needed; might generate crashes?

	// We only ever need to add the connector points for CNItem's when we're about to reroute...
	addAllItemConnectorPoints();

	// List of connectors which are to be determined to need rerouting (and whose routes aren't controlled by NodeGroups)
	QPtrList<Connector> connectorRerouteList;

	// For those connectors that are controlled by node groups
	QPtrList<NodeGroup> nodeGroupRerouteList;

	const QPtrList<Connector>::iterator connectorListEnd = m_connectorList.end();
	for ( QPtrList<Connector>::iterator it = m_connectorList.begin(); it != connectorListEnd; ++it )
	{
		Connector *connector = *it;
		if ( connector && connector->isVisible() && connector->startNode() && connector->endNode() )
		{
			// Perform a series of tests to see if the connector needs rerouting
			bool needsRerouting = false;

			// Test to see if we actually have any points
			const QList<QPoint> pointList = connector->connectorPoints();

			if( pointList.isEmpty() ) needsRerouting = true;

			// Test to see if the route doesn't match up with the node positions at either end
			if (!needsRerouting) {
				const QPoint listStart = pointList.first();
				const QPoint listEnd = pointList.last();
				const QPoint nodeStart = QPoint( int(connector->startNode()->x()), int(connector->startNode()->y()) );
				const QPoint nodeEnd = QPoint( int(connector->endNode()->x()), int(connector->endNode()->y()) );

				if ( ((listStart != nodeStart) || (listEnd != nodeEnd  )) &&
				     ((listStart != nodeEnd  ) || (listEnd != nodeStart)) )
				{
					needsRerouting = true;
				}
			}

			// Test to see if the route intersects any Items (we ignore if it is a manual route)
			if ( !needsRerouting && !connector->usesManualPoints() ) {

				const auto collisions = connector->collisions(true);
				const auto collisionsEnd = collisions.end();
				for ( auto collisionsIt = collisions.begin(); (collisionsIt != collisionsEnd) && !needsRerouting; ++collisionsIt )
				{
					if ( dynamic_cast<Item*>(*collisionsIt) )
						needsRerouting = true;
				}
			}

			if (needsRerouting) {
				NodeGroup *nodeGroup = connector->nodeGroup();

				if ( !nodeGroup && !connectorRerouteList.contains(connector) )
					connectorRerouteList.append(connector);
				else if ( nodeGroup && !nodeGroupRerouteList.contains(nodeGroup) )
					nodeGroupRerouteList.append(nodeGroup);
			}
		}
	}

	// To allow proper rerouting, we want to start with clean routes for all of the invalidated connectors
	const QPtrList<NodeGroup>::iterator nodeGroupRerouteEnd = nodeGroupRerouteList.end();
	for ( QPtrList<NodeGroup>::iterator it = nodeGroupRerouteList.begin(); it != nodeGroupRerouteEnd; ++it )
	{
		const QPtrList<Connector> contained = (*it)->connectorList();
		const QPtrList<Connector>::const_iterator end = contained.end();
		for ( QPtrList<Connector>::const_iterator it = contained.begin(); it != end; ++it )
			(*it)->updateConnectorPoints(false);
	}

	const QPtrList<Connector>::iterator connectorRerouteEnd = connectorRerouteList.end();
	for ( QPtrList<Connector>::iterator it = connectorRerouteList.begin(); it != connectorRerouteEnd; ++it )
		(*it)->updateConnectorPoints(false);

	// And finally, reroute the connectors
	for ( QPtrList<NodeGroup>::iterator it = nodeGroupRerouteList.begin(); it != nodeGroupRerouteEnd; ++it )
		(*it)->updateRoutes();

	for ( QPtrList<Connector>::iterator it = connectorRerouteList.begin(); it != connectorRerouteEnd; ++it )
		(*it)->rerouteConnector();

	for ( QPtrList<Connector>::iterator it = m_connectorList.begin(); it != connectorListEnd; ++it )
	{
		if (*it) (*it)->updateDrawList();
	}
}


void ICNDocument::deleteSelection()
{
	// End whatever editing mode we are in, as we don't want to start editing
	// something that is about to no longer exist...
	m_cmManager->cancelCurrentManipulation();

	if ( m_selectList->isEmpty() ) return;

	m_selectList->deleteAllItems();
	flushDeleteList();
	setModified(true);

	// We need to emit this so that property widgets etc...
	// can clear themselves.
	emit selectionChanged();

	requestRerouteInvalidatedConnectors();
	requestStateSave();
}


QPtrList<Connector> ICNDocument::getCommonConnectors( const QPtrList<Item> &list )
{
	QPtrList<Node> nodeList = getCommonNodes(list);

	// Now, get all the connectors, and remove the ones that don't have both end
	// nodes in the above generated list
	QPtrList<Connector> connectorList = m_connectorList;
	const QPtrList<Connector>::iterator connectorListEnd = connectorList.end();
	for ( QPtrList<Connector>::iterator it = connectorList.begin(); it != connectorListEnd; ++it )
	{
		Connector *con = *it;
		if ( !con || !nodeList.contains(con->startNode()) || !nodeList.contains(con->endNode()) ) {
			*it = 0;
		}
	}
	connectorList.removeAll((Connector*)0);
	return connectorList;
}


QPtrList<Node> ICNDocument::getCommonNodes( const QPtrList<Item> &list )
{
	QPtrList<Node> nodeList;

	const QPtrList<Item>::const_iterator listEnd = list.end();
	for(QPtrList<Item>::const_iterator it = list.begin(); it != listEnd; ++it )
	{
		NodeInfoMap nodeMap;
		CNItem *cnItem = dynamic_cast<CNItem*>((Item*)*it);

		if(cnItem) nodeMap = cnItem->nodeMap();

		const NodeInfoMap::iterator nodeMapEnd = nodeMap.end();
		for(NodeInfoMap::iterator it = nodeMap.begin(); it != nodeMapEnd; ++it )
		{
			Node *node = it.value().node;

			if(!nodeList.contains(node) ) nodeList += node;

			NodeGroup *ng = node->nodeGroup();
			if(ng) {
				QPtrList<Node> intNodeList = ng->internalNodeList();
				const QPtrList<Node>::iterator intNodeListEnd = intNodeList.end();
				for ( QPtrList<Node>::iterator it = intNodeList.begin(); it != intNodeListEnd; ++it )
				{
					Node *intNode = *it;
					if(!nodeList.contains(intNode) )
						nodeList += intNode;
				}
			}
		}
	}

	return nodeList;
}


void ICNDocument::unregisterUID( const QString & uid )
{
	ItemDocument::unregisterUID( uid );
}

//END class ICNDocument


DirCursor *DirCursor::m_self = 0;

DirCursor::DirCursor()
{
	initCursors();
}

DirCursor::~DirCursor()
{
}

DirCursor *DirCursor::self()
{
	 if (!m_self) m_self = new DirCursor;
	return m_self;
}

void DirCursor::initCursors()
{
// 	QCursor c(Qt::ArrowCursor);
// 	QBitmap bitmap = *c.bitmap();
// 	QBitmap mask = *c.mask();
// 	QPixmap pm( bitmap->width(), bitmap->height() );
// 	pm.setMask(mask);
// 	pm = c.pi
}

#include "moc_icndocument.cpp"
