/***************************************************************************
 *   Copyright (C) 2003-2004 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "cnitemgroup.h"
#include "component.h"
#include "connector.h"
#include "flowpart.h"
#include "icndocument.h"
#include "node.h"
#include "nodegroup.h"

CNItemGroup::CNItemGroup( ICNDocument *icnDocument, const char *name)
	: ItemGroup( icnDocument, name )
{
	p_icnDocument = icnDocument;
	m_connectorCount = 0;
	m_nodeCount = 0;
	m_currentLevel = -1;
}


CNItemGroup::~CNItemGroup()
{
}


bool CNItemGroup::addItem( Item *item )
{
	// Note, we must prepend the item to the list so that
	// activeCNItem() can return the item at the start
	// of the list as the most recently added item if some
	// the previous activeCNItem is removed

	if ( !item || !item->canvas() || m_itemList.contains(item) || !item->isMovable() )
		return false;

	if ( m_currentLevel != -1 && item->level() > m_currentLevel )
		return false;

	if ( item && m_currentLevel > item->level() )
		removeAllItems();

	registerItem(item);
	m_currentLevel = item->level();
	setActiveItem(item);
	item->setSelected(true);
	updateInfo();
	emit itemAdded(item);
	return true;
}


bool CNItemGroup::addNode( Node *node )
{
	if ( !node || m_nodeList.contains(node) || node->isChildNode() )
		return false;
	m_nodeList.prepend(node);
	node->setSelected(true);
	updateInfo();
	emit nodeAdded(node);
	return true;
}


bool CNItemGroup::addConnector( Connector *con )
{
	if ( !con || m_connectorList.contains(con) )
		return false;
	m_connectorList.prepend(con);
	con->setSelected(true);
	updateInfo();
	emit connectorAdded(con);
	return true;
}


bool CNItemGroup::addQCanvasItem( KtlQCanvasItem *qcanvasItem )
{
	if (!qcanvasItem) return false;

	Item *item = dynamic_cast<Item*>(qcanvasItem);
	if (item)
		return addItem(item);

	Node *node = dynamic_cast<Node*>(qcanvasItem);
	if (node)
		return addNode(node);

	Connector *connector = dynamic_cast<Connector*>(qcanvasItem);
	if (!connector)
	{
		ConnectorLine *connectorLine = dynamic_cast<ConnectorLine*>(qcanvasItem);
		if (connectorLine)
			connector = connectorLine->parent();
	}
	if (connector)
		return addConnector(connector);

	return false;
}

void CNItemGroup::setItems( KtlQCanvasItemList list )
{
	QPtrList<Item> itemRemoveList = m_itemList;
	QPtrList<Connector> connectorRemoveList = m_connectorList;
	QPtrList<Node> nodeRemoveList = m_nodeList;

	const KtlQCanvasItemList::const_iterator end = list.end();
	for ( KtlQCanvasItemList::const_iterator it = list.begin(); it != end; ++it )
	{
		if ( Item * item = dynamic_cast<Item*>(*it) )
			itemRemoveList.removeAll( item );

		else if ( Node * node = dynamic_cast<Node*>(*it) )
			nodeRemoveList.removeAll( node );

		else if ( Connector * con = dynamic_cast<Connector*>(*it) )
			connectorRemoveList.removeAll( con );

		else if ( ConnectorLine * conLine = dynamic_cast<ConnectorLine*>(*it) )
			connectorRemoveList.removeAll( conLine->parent() );
	}

	{
		const QPtrList<Item>::const_iterator end = itemRemoveList.end();
		for ( QPtrList<Item>::const_iterator it = itemRemoveList.begin(); it != end; ++it )
		{
			removeItem(*it);
			(*it)->setSelected(false);
		}
	}

	{
		const QPtrList<Node>::const_iterator end = nodeRemoveList.end();
		for ( QPtrList<Node>::const_iterator it = nodeRemoveList.begin(); it != end; ++it )
		{
			removeNode(*it);
			(*it)->setSelected(false);
		}
	}

	{
		const QPtrList<Connector>::const_iterator end = connectorRemoveList.end();
		for ( QPtrList<Connector>::const_iterator it = connectorRemoveList.begin(); it != end; ++it )
		{
			removeConnector(*it);
			(*it)->setSelected(false);
		}
	}

	{
		const KtlQCanvasItemList::const_iterator end = list.end();
		for ( KtlQCanvasItemList::const_iterator it = list.begin(); it != end; ++it )
		{
			// We don't need to check that we've already got the item as it will
			// be checked in the function call
			addQCanvasItem(*it);
		}
	}
}


void CNItemGroup::removeItem( Item *item )
{
	if ( !item || !m_itemList.contains(item) )
		return;
	unregisterItem(item);
	if ( m_activeItem == item )
		getActiveItem();

	item->setSelected(false);
	updateInfo();
	emit itemRemoved(item);
}


void CNItemGroup::removeNode( Node *node )
{
	if ( !node || !m_nodeList.contains(node) )
		return;
	m_nodeList.removeAll(node);
	node->setSelected(false);
	updateInfo();
	emit nodeRemoved(node);
}


void CNItemGroup::removeConnector( Connector *con )
{
	if ( !con || !m_connectorList.contains(con) ) return;
	m_connectorList.removeAll(con);
	con->setSelected(false);
	updateInfo();
	emit connectorRemoved(con);
}


void CNItemGroup::removeQCanvasItem( KtlQCanvasItem *qcanvasItem )
{
	if (!qcanvasItem) return;

	Item *item = dynamic_cast<Item*>(qcanvasItem);
	if (item)
		return removeItem(item);

	Node *node = dynamic_cast<Node*>(qcanvasItem);
	if (node)
		return removeNode(node);

	Connector *connector = dynamic_cast<Connector*>(qcanvasItem);
	if (!connector)
	{
		ConnectorLine *connectorLine = dynamic_cast<ConnectorLine*>(qcanvasItem);
		if (connectorLine)
			connector = connectorLine->parent();
	}
	if (connector)
		return removeConnector(connector);
}


QPtrList<Node> CNItemGroup::nodes( bool excludeParented ) const
{
	QPtrList<Node> nodeList = m_nodeList;
	if (excludeParented)
		return nodeList;

	QPtrList<NodeGroup> translatableNodeGroups;
	p_icnDocument->getTranslatable( items(false), 0l, 0l, &translatableNodeGroups );

	QPtrList<NodeGroup>::iterator end = translatableNodeGroups.end();
	for ( QPtrList<NodeGroup>::iterator it = translatableNodeGroups.begin(); it != end; ++it )
	{
		const QPtrList<Node> internal = (*it)->internalNodeList();
		QPtrList<Node>::const_iterator internalEnd = internal.end();
		for ( QPtrList<Node>::const_iterator intIt = internal.begin(); intIt != internalEnd; ++intIt )
		{
			if ( *intIt && !nodeList.contains(*intIt) )
				nodeList << *intIt;
		}
	}

	return nodeList;
}


QPtrList<Connector> CNItemGroup::connectors( bool excludeParented ) const
{
	QPtrList<Connector> connectorList = m_connectorList;
	if (excludeParented)
		return connectorList;

	QPtrList<Connector> translatableConnectors;
	QPtrList<NodeGroup> translatableNodeGroups;
	p_icnDocument->getTranslatable( items(false), 0l, &translatableConnectors, &translatableNodeGroups );

	QPtrList<Connector>::iterator tcEnd = translatableConnectors.end();
	for ( QPtrList<Connector>::iterator it = translatableConnectors.begin(); it != tcEnd; ++it )
	{
		if ( *it && !connectorList.contains(*it) )
			connectorList << *it;
	}

	QPtrList<NodeGroup>::iterator end = translatableNodeGroups.end();
	for ( QPtrList<NodeGroup>::iterator it = translatableNodeGroups.begin(); it != end; ++it )
	{
		const QPtrList<Node> internal = (*it)->internalNodeList();
		QPtrList<Node>::const_iterator internalEnd = internal.end();
		for ( QPtrList<Node>::const_iterator intIt = internal.begin(); intIt != internalEnd; ++intIt )
		{
			const QPtrList<Connector> connected = (*intIt)->getAllConnectors();
			QPtrList<Connector>::const_iterator connectedEnd = connected.end();
			for ( QPtrList<Connector>::const_iterator conIt = connected.begin(); conIt != connectedEnd; ++conIt )
			{
				if ( *conIt && !connectorList.contains(*conIt) )
					connectorList << *conIt;
			}
		}
	}

	return connectorList;
}


bool CNItemGroup::contains( KtlQCanvasItem *qcanvasItem ) const
{
	if (!qcanvasItem)
		return false;

	const QPtrList<Item>::const_iterator ciEnd = m_itemList.end();
	for ( QPtrList<Item>::const_iterator it = m_itemList.begin(); it != ciEnd; ++it )
	{
		if ( *it == qcanvasItem )
			return true;
	}
	const QPtrList<Connector>::const_iterator conEnd = m_connectorList.end();
	for ( QPtrList<Connector>::const_iterator it = m_connectorList.begin(); it != conEnd; ++it )
	{
		if ( *it == qcanvasItem )
			return true;
	}
	const QPtrList<Node>::const_iterator nodeEnd = m_nodeList.end();
	for ( QPtrList<Node>::const_iterator it = m_nodeList.begin(); it != nodeEnd; ++it )
	{
		if ( *it == qcanvasItem )
			return true;
	}

	return false;
}


void CNItemGroup::setSelected( bool sel )
{
	const QPtrList<Item>::iterator ciEnd = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != ciEnd; ++it )
	{
		if (*it && (*it)->isSelected() != sel )
			(*it)->setSelected(sel);
	}
	const QPtrList<Connector>::iterator conEnd = m_connectorList.end();
	for ( QPtrList<Connector>::iterator it = m_connectorList.begin(); it != conEnd; ++it )
	{
		if ( *it && (*it)->isSelected() != sel )
			(*it)->setSelected(sel);
	}
	const QPtrList<Node>::iterator nodeEnd = m_nodeList.end();
	for ( QPtrList<Node>::iterator it = m_nodeList.begin(); it != nodeEnd; ++it )
	{
		if ( *it && (*it)->isSelected() != sel )
			(*it)->setSelected(sel);
	}
}


bool CNItemGroup::canRotate() const
{
	const QPtrList<Item>::const_iterator end = m_itemList.end();
	for ( QPtrList<Item>::const_iterator it = m_itemList.begin(); it != end; ++it )
	{
		// Components can rotate
		if ( dynamic_cast<Component*>((Item*)*it) )
			return true;
	}
	return false;
}


bool CNItemGroup::canFlip() const
{
	const QPtrList<Item>::const_iterator end = m_itemList.end();
	for ( QPtrList<Item>::const_iterator it = m_itemList.begin(); it != end; ++it )
	{
		// Components can flip
		if ( dynamic_cast<Component*>((Item*)*it) )
			return true;
	}
	return false;
}


void CNItemGroup::slotRotateCW()
{
	const QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		Component *component = dynamic_cast<Component*>((Item*)*it);
		if ( component && component->isMovable() )
		{
			int oldAngle = component->angleDegrees();
			component->setAngleDegrees( oldAngle + 90 );
		}
	}
	p_icnDocument->requestStateSave();
}

void CNItemGroup::slotRotateCCW()
{
	const QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		Component *component = dynamic_cast<Component*>((Item*)*it);
		if ( component && component->isMovable() )
		{
			int oldAngle = component->angleDegrees();
			component->setAngleDegrees( oldAngle - 90 );
		}
	}
	p_icnDocument->requestStateSave();
}


void CNItemGroup::flipHorizontally()
{
	const QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		Component *component = dynamic_cast<Component*>((Item*)*it);
		if ( component && component->isMovable() )
		{
			bool oldFlipped = component->flipped();
			component->setFlipped(!oldFlipped);
		}
	}
	p_icnDocument->requestStateSave();
}


void CNItemGroup::flipVertically()
{
	const QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		Component *component = dynamic_cast<Component*>((Item*)*it);
		if ( component && component->isMovable() )
		{
			bool oldFlipped = component->flipped();

			int oldAngle = component->angleDegrees();
			component->setAngleDegrees( oldAngle + 180 );
			component->setFlipped( ! oldFlipped );
			component->setAngleDegrees( oldAngle + 180 );
		}
	}
	p_icnDocument->requestStateSave();
}


bool CNItemGroup::haveSameOrientation() const
{
	// set true once determined what is in this itemgroup
	bool areFlowparts = false;
	bool areComponents = false;

	// for components
	int angleDegrees = 0;
	bool flipped = false;

	// for flowparts
	unsigned orientation = 0;

	const QPtrList<Item>::const_iterator end = m_itemList.end();
	for ( QPtrList<Item>::const_iterator it = m_itemList.begin(); it != end; ++it )
	{
		Component * component = dynamic_cast<Component*>((Item*)*it);
		FlowPart * flowpart = dynamic_cast<FlowPart*>((Item*)*it);

		if ( component && flowpart )
			return false;

		if ( !component && !flowpart )
			return false;

		if ( component )
		{
			if ( areFlowparts )
				return false;

			if ( !areComponents )
			{
				// It's the first component we've come across
				angleDegrees = component->angleDegrees();
				flipped = component->flipped();
				areComponents = true;
			}
			else
			{
				if ( angleDegrees != component->angleDegrees() )
					return false;

				if ( flipped != component->flipped() )
					return false;
			}
		}
		else
		{
			if ( areComponents )
				return false;

			if ( !areFlowparts )
			{
				// It's the first flowpart we've come across
				orientation = flowpart->orientation();
				areFlowparts = true;
			}
			else
			{
				if ( orientation != flowpart->orientation() )
					return false;
			}
		}
	}

	return true;
}


void CNItemGroup::setOrientationAngle( int _angle )
{
	const QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		Component *component = dynamic_cast<Component*>((Item*)*it);
		if ( component && component->isMovable() )
		{
			int oldAngle = component->angleDegrees();
			if ( oldAngle != _angle )
			{
				component->setAngleDegrees(_angle);
			}
		}
	}
	p_icnDocument->requestStateSave();
}


void CNItemGroup::setComponentOrientation( int angleDegrees, bool flipped )
{
	const QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		Component *component = dynamic_cast<Component*>((Item*)*it);
		if ( component && component->isMovable() )
		{
			int oldAngle = component->angleDegrees();
			int oldFlipped = component->flipped();
			if ( (oldAngle != angleDegrees) || (oldFlipped != flipped) )
			{
				component->setFlipped(flipped);
				component->setAngleDegrees(angleDegrees);
			}
		}
	}
	p_icnDocument->requestStateSave();
}


void CNItemGroup::setFlowPartOrientation( unsigned orientation )
{
	const QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		FlowPart * flowPart = dynamic_cast<FlowPart*>((Item*)*it);
		if ( flowPart && flowPart->isMovable() )
			flowPart->setOrientation(orientation);
	}
	p_icnDocument->requestStateSave();
}


void CNItemGroup::mergeGroup( ItemGroup *itemGroup )
{
	CNItemGroup *group = dynamic_cast<CNItemGroup*>(itemGroup);
	if (!group) return;

	const QPtrList<Item> items = group->items();
	const QPtrList<Connector> connectors = group->connectors();
	const QPtrList<Node> nodes = group->nodes();

	const QPtrList<Item>::const_iterator ciEnd = items.end();
	for ( QPtrList<Item>::const_iterator it = items.begin(); it != ciEnd; ++it )
	{
		addItem(*it);
	}
	const QPtrList<Connector>::const_iterator conEnd = connectors.end();
	for ( QPtrList<Connector>::const_iterator it = connectors.begin(); it != conEnd; ++it )
	{
		addConnector(*it);
	}
	const QPtrList<Node>::const_iterator nodeEnd = nodes.end();
	for ( QPtrList<Node>::const_iterator it = nodes.begin(); it != nodeEnd; ++it )
	{
		addNode(*it);
	}
}


void CNItemGroup::removeAllItems()
{
	while ( !m_itemList.isEmpty() )
		removeItem(*m_itemList.begin());

	while ( !m_connectorList.isEmpty() )
		removeConnector(*m_connectorList.begin());

	while ( !m_nodeList.isEmpty() )
		removeNode(*m_nodeList.begin());
}


void CNItemGroup::deleteAllItems()
{
	const QPtrList<Item>::iterator ciEnd = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != ciEnd; ++it )
	{
		if (*it)
			(*it)->removeItem();
	}
	const QPtrList<Node>::iterator nodeEnd = m_nodeList.end();
	for ( QPtrList<Node>::iterator it = m_nodeList.begin(); it != nodeEnd; ++it )
	{
		if ( *it && !(*it)->isChildNode() )
		{
			(*it)->removeNode();
		}
	}
	const QPtrList<Connector>::iterator conEnd = m_connectorList.end();
	for ( QPtrList<Connector>::iterator it = m_connectorList.begin(); it != conEnd; ++it )
	{
		if (*it)
		{
			(*it)->removeConnector();
		}
	}

	// Clear the lists
	removeAllItems();
}


void CNItemGroup::updateInfo()
{
	m_connectorCount = m_connectorList.count();
	m_nodeCount = m_nodeList.count();

	if ( m_itemList.isEmpty() )
		m_currentLevel = -1;
}


void CNItemGroup::getActiveItem()
{
	if ( m_itemList.isEmpty() )
		setActiveItem(0l);
	else
		setActiveItem( *m_itemList.begin() );
}


void CNItemGroup::setActiveItem( Item *item )
{
	if ( item == m_activeItem )
		return;
	m_activeItem = item;
}


QStringList CNItemGroup::itemIDs()
{
	QStringList list;
	QPtrList<Item>::iterator end = m_itemList.end();
	for ( QPtrList<Item>::iterator it = m_itemList.begin(); it != end; ++it )
	{
		if (*it) {
			list += (*it)->id();
		}
	}
	return list;
}

#include "moc_cnitemgroup.cpp"
