/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ECNODE_H
#define ECNODE_H

#include "pch.hpp"

#include "node.h"

// #include <q3valuevector.h>

class ECNode;
class Element;
class Pin;
class Switch;
class QTimer;

using PinVector = QVector<QPointer<Pin>>;

/**
@short Electrical node with voltage / current / etc properties
@author David Saxton
*/
class ECNode : public Node
{
	Q_OBJECT
	public:
		ECNode( ICNDocument *icnDocument, Node::node_type type, int dir, const QPoint &pos, QString *id = nullptr );
		~ECNode() override;

		void setParentItem( CNItem *parentItem ) override;

		/**
		 *  draws the ECNode; still only a pure virtual function
		 */
		void drawShape( QPainter &p ) override = 0;
		/**
		 * Set the number of pins "contained" in this node.
		 */
		void setNumPins( unsigned num );
		/**
		 * @return the number of pins in this node.
		 * @see setNumPins
		 */
		unsigned numPins() const { return m_pins.size(); }
		/**
		 * @return the pins in the node, as a vector
		 */
		const PinVector & pins() const { return m_pins; }

		/**
		 * @param num number of the
		 * @return pointer to a pin in this node, given by num
		 */
		Pin *pin( unsigned num = 0 ) const ;
			//{ return (num < m_pins.size()) ? m_pins[num] : 0l; }

		bool showVoltageBars() const { return m_bShowVoltageBars; }
		void setShowVoltageBars( bool show ) { m_bShowVoltageBars = show; }
		bool showVoltageColor() const { return m_bShowVoltageColor; }
		void setShowVoltageColor( bool show ) { m_bShowVoltageColor = show; }
		void setNodeChanged();

		/**
		 * Returns true if this node is connected (or is the same as) the node given
		 * by other connectors or nodes (although not through CNItems)
		 * checkedNodes is a list of nodes that have already been checked for
		 * being the connected nodes, and so can simply return if they are in there.
		 * If it is null, it will assume that it is the first ndoe & will create a list
		 */
		bool isConnected( Node *node, QPtrList<Node> *checkedNodes = nullptr ) override;
		/**
		 * Sets the node's visibility, as well as updating the visibility of the
		 * attached connectors as appropriate
	 	*/
		void setVisible( bool visible ) override;
		/**
		 * Registers an input connector (i.e. this is the end node) as connected
		 * to this node.
	 	*/
		void addConnector( Connector * const connector );
		/**
		 * Creates a new connector, sets this as the end node to the connector
		 * and returns a pointer to the connector.
		 */
		Connector* createConnector( Node * node);

		// TODO oups, the following two methods do the same thing. Only one is needed.
		/**
		 * Returns a list of the attached connectors; implemented inline
	 	*/
		const QPtrList<Connector> & connectorList() const { return m_connectorList; }

		/**
		 * @return the list of all the connectors attached to the node
		 */
		QPtrList<Connector> getAllConnectors() const override { return m_connectorList; }

		/**
		 * Removes all the NULL connectors
	 	 */
		void removeNullConnectors() override;

		/**
		 * Returns the total number of connections to the node. This is the number
		 * of connectors and the parent
		 * item connector if it exists and is requested.
		 * @param includeParentItem Count the parent item as a connector if it exists
		 * @param includeHiddenConnectors hidden connectors are those as e.g. part of a subcircuit
	 	*/
		int numCon( bool includeParentItem, bool includeHiddenConnectors ) const override;
		/**
		 * Removes a specific connector
		 */
		void removeConnector( Connector *connector ) override;

		/**
		 * For an electric node: returns the first connector
		 * If the node isn't connected to anyithing, returns null ( 0 )
		 * @return pointer to the desired connector
		 */
		Connector* getAConnector() const override ;

	signals:
		void numPinsChanged( unsigned newNum );

	public slots:
		// -- from node.h --
		void checkForRemoval( Connector *connector );

	protected slots:
		void removeElement( Element * e );
		void removeSwitch( Switch * sw );

	protected:
		/** The attached connectors to this electronic node. No directionality here */
		QPtrList<Connector> m_connectorList;
		PinVector m_pins;
		KtlQCanvasRectangle * m_pinPoint = nullptr;
		double m_prevV = 0;
		double m_prevI = 0;
		bool m_bShowVoltageBars;
		bool m_bShowVoltageColor;

		// -- functionality from node.h --
		/** If this node has precisely two connectors emerging from it, then this
		 * function will trace the two connectors until the point where they
		 * diverge; this point is returned.
		 * TODO: find a meaning for this function, for an electronic node...
		 */
		QPoint findConnectorDivergePoint( bool * found ) override;

		/** (please document this) registers some signals for the node and the new connector (?) */
		bool handleNewConnector( Connector * newConnector );
};

#endif
