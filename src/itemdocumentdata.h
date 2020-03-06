/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#pragma once

#include "item.h"
#include "microsettings.h"

#include <qdom.h>

class Connector;
class ECSubcircuit;
class KUrl;
class Node;
class PinMapping;

using PinMappingMap = QMap<QString, PinMapping>;

using BoolMap = QMap<QString, bool>;
using DoubleMap = QMap<QString, double>;
using IntMap = QMap<QString, int>;
using QColorMap = QMap<QString, QColor>;
using QStringMap = QMap<QString, QString>;
using QBitArrayMap = QMap<QString, QBitArray>;


class ItemData
{
	public:
		ItemData();

		QString type;
		double x;
		double y;
		int z;
		QRect size;
		bool setSize;
		int orientation; // used for flowparts, should be set to -1 if not used.
		double angleDegrees;
		bool flipped;
		BoolMap buttonMap;
		IntMap sliderMap;
		QString parentId;
		BoolMap dataBool;
		DoubleMap dataNumber;
		QColorMap dataColor;
		QStringMap dataString;
		QBitArrayMap dataRaw;
};
using ItemDataMap = QMap<QString, ItemData>;


class ConnectorData
{
	public:
		ConnectorData();

		QList<QPoint> route;
		bool manualRoute;

		bool startNodeIsChild;
		bool endNodeIsChild;

		QString startNodeCId;
		QString endNodeCId;

		QString startNodeParent;
		QString endNodeParent;

		QString startNodeId;
		QString endNodeId;
};
using ConnectorDataMap = QMap<QString, ConnectorData>;


struct NodeData
{
		double x = 0;
		double y = 0;
};
using NodeDataMap = QMap<QString, NodeData>;

class PinData
{
	public:
		PinData();

		PinSettings::pin_type type;
		PinSettings::pin_state state;
};
using PinDataMap = QMap<QString, PinData>;

class MicroData
{
	public:
		MicroData();
		void reset();

		QString id;
		PinDataMap pinMap;
		QStringMap variableMap;
		PinMappingMap pinMappings;
};

/**
This class encapsulates all or part of an ItemDocument. It is used for writing
the document to file / reading from file, as well as for the clipboard and
undo/redo system.
@author David Saxton
*/
class ItemDocumentData
{
	public:
		ItemDocumentData( uint documentType );
		~ItemDocumentData();
		/**
		 * Erases / resets all data to defaults
		 */
		void reset();
		/**
		 * Read in data from a saved file. Any existing data in this class will
		 * be deleted first.
		 * @returns true iff successful
		 */
		bool loadData( const KUrl &url );
		/**
		 * Write the data to the given file.
		 * @returns true iff successful
		 */
		bool saveData( const KUrl &url );
		/**
		 * Returns the xml used for describing the data
		 */
		QString toXML();
		/**
		 * Restore the document from the given xml
		 * @return true if successful
		 */
		bool fromXML( const QString &xml );
		/**
		 * Saves the document to the data
		 */
		void saveDocumentState( ItemDocument *itemDocument );
		/**
		 * Restores a document to the state stored in this class
		 */
		void restoreDocument( ItemDocument *itemDocument );
		/**
		 * Merges the stuff stored here with the given document. If this is
		 * being used for e.g. pasting, you should call generateUniqueIDs()
		 * @param selectNew if true then the newly created items & connectors will be selected
		 */
		void mergeWithDocument( ItemDocument *itemDocument, bool selectNew );
		/**
		 * Replaces the IDs of everything with unique ones for the document.
		 * Used in pasting.
		 */
		void generateUniqueIDs( ItemDocument *itemDocument );
		/**
		 * Move all the items, connectors, nodes, etc by the given amount
		 */
		void translateContents( int dx, int dy );
		/**
		 * Returns the document type.
		 * @see Document::DocumentType
		 */
		uint documentType() const { return m_documentType; }

		//BEGIN functions for adding data
		void setMicroData( const MicroData &data );
		void addItems( const QPtrList<Item> &itemList );
		void addConnectors( const QPtrList<Connector> &connectorList );
		void addNodes( const QPtrList<Node> &nodeList );

		/**
		 * Add the given ItemData to the stored data
		 */
		void addItemData( ItemData itemData, QString id );
		/**
		 * Add the given ConnectorData to the stored data
		 */
		void addConnectorData( ConnectorData connectorData, QString id );
		/**
		 * Add the given NodeData to the stored data
		 */
		void addNodeData( NodeData nodeData, QString id );
		//END functions for adding data

		//BEGIN functions for returning strings for saving to xml
		QString documentTypeString() const;
		QString revisionString() const;
		//END functions for returning strings for saving to xml

	protected:
		//BEGIN functions for generating QDomElements
		QDomElement microDataToElement( QDomDocument &doc );
		QDomElement itemDataToElement( QDomDocument &doc, const ItemData &itemData );
		QDomElement nodeDataToElement( QDomDocument &doc, const NodeData &nodeData );
		QDomElement connectorDataToElement( QDomDocument &doc, const ConnectorData &connectorData );
		//END functions for generating QDomElements

		//BEGIN functions for reading QDomElements to stored data
		void elementToMicroData( QDomElement element );
		void elementToItemData( QDomElement element );
		void elementToNodeData( QDomElement element );
		void elementToConnectorData( QDomElement element );
		//END functions for reading QDomElements to stored data

		ItemDataMap m_itemDataMap;
		ConnectorDataMap m_connectorDataMap;
		NodeDataMap m_nodeDataMap;
		MicroData m_microData;
		uint m_documentType; // See Document::DocumentType
};

class SubcircuitData : public ItemDocumentData
{
	public:
		SubcircuitData();
		void initECSubcircuit( ECSubcircuit * ecSubcircuit );
};
