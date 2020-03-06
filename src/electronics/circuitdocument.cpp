/***************************************************************************
 *   Copyright (C) 2003-2005 by David Saxton                               *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "canvasmanipulator.h"
#include "circuitdocument.h"
#include "circuiticndocument.h"
#include "circuitview.h"
#include "component.h"
#include "connector.h"
#include "cnitemgroup.h"
#include "documentiface.h"
#include "drawparts/drawpart.h"
#include "ecnode.h"
#include "itemdocumentdata.h"
#include "ktechlab.h"
#include "pin.h"
#include "simulator.h"
#include "subcircuits.h"
#include "switch.h"

#include <qdebug.h>
#include <kinputdialog.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kactionmenu.h>
#include <kicon.h>

#include <qregexp.h>
#include <qtimer.h>

#include <ktlconfig.h>


CircuitDocument::CircuitDocument( const QString & caption, const char *name )
	: CircuitICNDocument( caption, name )
{
	m_pOrientationAction = new KActionMenu( QIcon::fromTheme("transform-rotate"), i18n("Orientation"), this );

	m_type = Document::dt_circuit;
	m_pDocumentIface = new CircuitDocumentIface(this);
	m_fileExtensionInfo = QString("*.circuit|%1(*.circuit)\n*|%2").arg( i18n("Circuit") ).arg( i18n("All Files") );

	m_cmManager->addManipulatorInfo( CMSelect::manipulatorInfo() );
	m_cmManager->addManipulatorInfo( CMDraw::manipulatorInfo() );
	m_cmManager->addManipulatorInfo( CMRightClick::manipulatorInfo() );
	m_cmManager->addManipulatorInfo( CMRepeatedItemAdd::manipulatorInfo() );
	m_cmManager->addManipulatorInfo( CMItemResize::manipulatorInfo() );
	m_cmManager->addManipulatorInfo( CMItemDrag::manipulatorInfo() );

	connect( this, SIGNAL(connectorAdded(Connector*)), this, SLOT(requestAssignCircuits()) );
	connect( this, SIGNAL(connectorAdded(Connector*)), this, SLOT(connectorAdded(Connector*)) );

	m_updateCircuitsTmr = new QTimer();
	connect( m_updateCircuitsTmr, SIGNAL(timeout()), this, SLOT(assignCircuits()) );

	requestStateSave();
}

CircuitDocument::~CircuitDocument()
{
	m_bDeleted = true;

    disconnect( m_updateCircuitsTmr, SIGNAL(timeout()), this, SLOT(assignCircuits()) );
    disconnect( this, SIGNAL(connectorAdded(Connector*)), this, SLOT(connectorAdded(Connector*)) );
    disconnect( this, SIGNAL(connectorAdded(Connector*)), this, SLOT(requestAssignCircuits()) );

    for (QPtrList<Connector>::Iterator itConn = m_connectorList.begin(); itConn != m_connectorList.end(); ++itConn) {
        Connector *connector = itConn->data();
        disconnect( connector, SIGNAL(removed(Connector*)), this, SLOT(requestAssignCircuits()) );
    }
    for (ItemMap::Iterator itItem = m_itemList.begin(); itItem != m_itemList.end(); ++itItem) {
        Item *item = itItem.value();
        disconnect( item, SIGNAL(removed(Item*)), this, SLOT(componentRemoved(Item*)) );
        Component *comp = dynamic_cast<Component*>( item );
        if ( comp ) {
            disconnect( comp, SIGNAL(elementDestroyed(Element*)), this, SLOT(requestAssignCircuits()) );
        }
    }

    deleteCircuits();

	delete m_updateCircuitsTmr;
	delete m_pDocumentIface;
}


void CircuitDocument::slotInitItemActions( )
{
	CircuitICNDocument::slotInitItemActions();

	CircuitView *activeCircuitView = dynamic_cast<CircuitView*>(activeView());

	if ( !KTechlab::self() || !activeCircuitView ) return;

	Component *item = dynamic_cast<Component*>( m_selectList->activeItem() );

	if ( (!item && m_selectList->count() > 0) || !m_selectList->itemsAreSameType() )
		return;

	QAction * orientation_actions[] = {
		activeCircuitView->actionByName("edit_orientation_0"),
		activeCircuitView->actionByName("edit_orientation_90"),
		activeCircuitView->actionByName("edit_orientation_180"),
		activeCircuitView->actionByName("edit_orientation_270") };

	if ( !item )
	{
		for ( unsigned i = 0; i < 4; ++i )
			orientation_actions[i]->setEnabled(false);
		return;
	}

	for ( unsigned i = 0; i < 4; ++ i)
	{
		orientation_actions[i]->setEnabled(true);
		m_pOrientationAction->removeAction( orientation_actions[i] );
		m_pOrientationAction->addAction( orientation_actions[i] );
	}

	KToggleAction *action = nullptr;
	switch (item->angleDegrees()) {
		case 0:
			action = static_cast<KToggleAction *>(orientation_actions[0]); break;
		case 90:
			action = static_cast<KToggleAction *>(orientation_actions[1]); break;
		case 180:
			action = static_cast<KToggleAction *>(orientation_actions[2]); break;
		case 270:
			action = static_cast<KToggleAction *>(orientation_actions[3]); break;
		default: break;
	}
	if (action) {
		action->setChecked(true);
	}
}


void CircuitDocument::rotateCounterClockwise()
{
	m_selectList->slotRotateCCW();
	requestRerouteInvalidatedConnectors();
}

void CircuitDocument::rotateClockwise()
{
	m_selectList->slotRotateCW();
	requestRerouteInvalidatedConnectors();
}

void CircuitDocument::flipHorizontally()
{
	m_selectList->flipHorizontally();
	requestRerouteInvalidatedConnectors();
}

void CircuitDocument::flipVertically()
{
	m_selectList->flipVertically();
	requestRerouteInvalidatedConnectors();
}

void CircuitDocument::setOrientation0()
{
	m_selectList->slotSetOrientation0();
	requestRerouteInvalidatedConnectors();
}

void CircuitDocument::setOrientation90()
{
	m_selectList->slotSetOrientation90();
	requestRerouteInvalidatedConnectors();
}

void CircuitDocument::setOrientation180()
{
	m_selectList->slotSetOrientation180();
	requestRerouteInvalidatedConnectors();
}

void CircuitDocument::setOrientation270()
{
	m_selectList->slotSetOrientation270();
	requestRerouteInvalidatedConnectors();
}


View *CircuitDocument::createView( ViewContainer *viewContainer, uint viewAreaId, const char *name )
{
	View *view = new CircuitView( this, viewContainer, viewAreaId, name );
	handleNewView(view);
	return view;
}


void CircuitDocument::slotUpdateConfiguration()
{
	CircuitICNDocument::slotUpdateConfiguration();

	for (auto &node : m_ecNodeList) {
		if (!node) continue;

		node->setShowVoltageBars( KTLConfig::showVoltageBars() );
		node->setShowVoltageColor( KTLConfig::showVoltageColor() );
	}

	for (auto &connector : m_connectorList) {
		if (connector.isNull() || !connector) continue;

		connector->updateConnectorLines();
	}

	for (auto &component : m_componentList) {
		if (!component) continue;

		component->slotUpdateConfiguration();
	}
}


void CircuitDocument::update()
{
	CircuitICNDocument::update();

	bool animWires = KTLConfig::animateWires();

	if ( KTLConfig::showVoltageColor() || animWires )
	{
		if ( animWires )
		{
			// Wire animation is for showing currents, so we need to recalculate the currents
			// in the wires.
			calculateConnectorCurrents();
		}

		for (auto &connector : m_connectorList) {
			if (connector.isNull() || !connector) continue;

			connector->incrementCurrentAnimation( 1.0 / double(KTLConfig::refreshRate()) );
			connector->updateConnectorLines( animWires );
		}
	}

	if ( KTLConfig::showVoltageColor() || KTLConfig::showVoltageBars() )
	{
		for (auto &node : m_ecNodeList) {
			if (!node) continue;

			node->setNodeChanged();
		}
	}
}


void CircuitDocument::fillContextMenu( const QPoint &pos )
{
	CircuitICNDocument::fillContextMenu(pos);

	CircuitView *activeCircuitView = dynamic_cast<CircuitView*>(activeView());

	if (!activeCircuitView) return;

	bool canCreateSubcircuit = (m_selectList->count() > 1 && countExtCon(m_selectList->items()) > 0);
	QAction *subcircuitAction = activeCircuitView->actionByName("circuit_create_subcircuit");
	subcircuitAction->setEnabled( canCreateSubcircuit );

	if (m_selectList->isEmpty()) return;

	Component *item = dynamic_cast<Component*>(selectList()->activeItem());

	// NOTE: I negated this whole condition because I couldn't make out quite what the
	//logic was --electronerd
	if (!( (!item && m_selectList->count() > 0) || !m_selectList->itemsAreSameType() ))
	{
		QAction * orientation_actions[] = {
			activeCircuitView->actionByName("edit_orientation_0"),
			activeCircuitView->actionByName("edit_orientation_90"),
			activeCircuitView->actionByName("edit_orientation_180"),
			activeCircuitView->actionByName("edit_orientation_270") };

		if ( !item ) return;

		for ( unsigned i = 0; i < 4; ++ i)
		{
			m_pOrientationAction->removeAction( orientation_actions[i] );
			m_pOrientationAction->addAction( orientation_actions[i] );
		}

		QList<QAction*> orientation_actionlist;
	// 	orientation_actionlist.prepend( new KActionSeparator() );
		orientation_actionlist.append( m_pOrientationAction );
		KTechlab::self()->plugActionList( "orientation_actionlist", orientation_actionlist );
	}
}


void CircuitDocument::deleteCircuits()
{
	for (auto &circuit : m_circuitList)
	{
		if (!circuit) continue;
		if (!Simulator::isDestroyedSim()) {
				Simulator::self()->detachCircuit(circuit);
		}
		delete circuit;
	}
	m_circuitList.clear();
	m_pinList.clear();
	m_wireList.clear();
}


void CircuitDocument::requestAssignCircuits()
{
	if (m_bDeleted) {
			return;
	}
	deleteCircuits();
	m_updateCircuitsTmr->stop();
  m_updateCircuitsTmr->setSingleShot( true );
	m_updateCircuitsTmr->start( 0 /*, true */ );
}


void CircuitDocument::connectorAdded( Connector * connector )
{
	if (connector) {
		connect( connector, SIGNAL(numWiresChanged(unsigned )), this, SLOT(requestAssignCircuits()) );
		connect( connector, SIGNAL(removed(Connector*)), this, SLOT(requestAssignCircuits()) );
	}
}


void CircuitDocument::itemAdded( Item * item)
{
	CircuitICNDocument::itemAdded( item );
	componentAdded( item );
}


void CircuitDocument::componentAdded( Item * item )
{
	Component *component = dynamic_cast<Component*>(item);

	if (!component) return;

	requestAssignCircuits();

	connect( component, SIGNAL(elementCreated(Element*)), this, SLOT(requestAssignCircuits()) );
	connect( component, SIGNAL(elementDestroyed(Element*)), this, SLOT(requestAssignCircuits()) );
	connect( component, SIGNAL(removed(Item*)), this, SLOT(componentRemoved(Item*)) );

	// We don't attach the component to the Simulator just yet, as the
	// Component's vtable is not yet fully constructed, and so Simulator can't
	// tell whether or not it is a logic component
	if ( !m_toSimulateList.contains(component) )
		m_toSimulateList << component;
}


void CircuitDocument::componentRemoved( Item * item )
{
	Component *component = dynamic_cast<Component*>(item);

	if (!component) return;

	m_componentList.removeAll( component );
	m_toSimulateList.removeAll( component );

	// Check all of the lists and see if it's the component of anything.
	for (auto it = m_switchList.begin(); it < m_switchList.end();) {
		auto &comp = (*it);
		if (!!comp && ((Switch *)comp == (Switch *)component || comp->getComponent() == component)) {
			it = m_switchList.erase(it);
		}
		else {
			++it;
		}
	}

	requestAssignCircuits();

	if (!Simulator::isDestroyedSim()) {
			Simulator::self()->detachComponent(component);
	}
}

// I think this is where the inf0z from cnodes/branches is moved into the midle-layer
// pins/wires.

void CircuitDocument::calculateConnectorCurrents()
{
	for (auto &circuit : m_circuitList) {
		if (!circuit) continue;
		circuit->updateCurrents();
	}

	QPtrList<Pin> groundPins;

	// Tell the Pins to reset their calculated currents to zero
	m_pinList.removeAll(nullptr);

	for (auto &pin : m_pinList) {
		if (pin.isNull() || !pin) continue;

			pin->resetCurrent();
			pin->setSwitchCurrentsUnknown();

			if ( !pin->parentECNode()->isChildNode() ) {
				pin->setCurrentKnown( true );
				// (and it has a current of 0 amps)
			}
			else if ( pin->groundType() == Pin::gt_always ) {
				groundPins << pin;
				pin->setCurrentKnown( false );
			} else {
				// Child node that is non ground
				pin->setCurrentKnown( pin->parentECNode()->numPins() < 2 );
			}
	}

	// Tell the components to update their ECNode's currents' from the elements
	// currents are merged into PINS.
	for (auto &component : m_componentList) {
		if (!component) continue;
		component->setNodalCurrents();
	}

	// And now for the wires and switches...
	m_wireList.removeAll(nullptr);
	for (auto &wire : m_wireList) {
		if (wire.isNull() || !wire) continue;
		wire->setCurrentKnown(false);
	}

	m_switchList.removeAll(nullptr);

	QPtrList<Switch> switches = m_switchList;
	QPtrList<Wire> wires = m_wireList;
	bool found = true;
	while ( (!wires.isEmpty() || !switches.isEmpty() || !groundPins.isEmpty()) && found )
	{
		found = false;

		for (auto it = wires.begin(); it != wires.end();)
		{
			auto &wire = *it;
			if (wire.isNull() || !wire) continue;
			if (wire->calculateCurrent())
			{
				found = true;
				it = wires.erase(it);
			}
			else {
      	++it;
      }
		}

		for (auto it = switches.begin(); it != switches.end();)
		{
			auto &sw = *it;
			if (!sw) continue;
			if (sw->calculateCurrent())
			{
				found = true;
				it = switches.erase(it);
			}
			else {
				++it;
			}
		}

		//make the ground pins work. Current engine doesn't treat ground explicitly.
		for (auto it = groundPins.begin(); it != groundPins.end();) {
			auto &pin = *it;
			if (pin.isNull() || !pin) continue;
			if (pin->calculateCurrentFromWires()) {
				found = true;
				it = groundPins.erase(it);
			}
			else {
				++it;
			}
		}
	}
}


void CircuitDocument::assignCircuits()
{
	// Now we can finally add the unadded components to the Simulator
	for (auto &component : m_toSimulateList) {
		if (!component) continue;
		Simulator::self()->attachComponent(component);
	}
	m_toSimulateList.clear();

	// Stage 0: Build up pin and wire lists
	m_pinList.clear();
	for (auto &node : m_ecNodeList) {
		if (!node) continue;

		for (unsigned i = 0; i < node->numPins(); i++) {
			m_pinList << node->pin(i);
		}
	}

	m_wireList.clear();
	for (auto &connector : m_connectorList) {
		if (connector.isNull() || !connector) continue;

		for (int i = 0; i < connector->numWires(); i++) {
			m_wireList << connector->wire(i);
		}
	}

	using PinListList = QList<QPtrList<Pin>>;

	// Stage 1: Partition the circuit up into dependent areas (bar splitting
	// at ground pins)
	QPtrList<Pin> unassignedPins = m_pinList;
	PinListList pinListList;

	while(!unassignedPins.isEmpty()) {
		QPtrList<Pin> pinList;
		getPartition(*unassignedPins.begin(), &pinList, &unassignedPins);
		pinListList.append(pinList);
	}

	// Stage 2: Split up each partition into circuits by ground pins
	for (auto &pinList : pinListList) {
		splitIntoCircuits(&pinList);
	}

	// Stage 3: Initialize the circuits
	m_circuitList.removeAll(nullptr);
	for (auto &circuit : m_circuitList) {
		if (!circuit) continue;

		circuit->init();
	}

	m_switchList.clear();
	m_componentList.clear();
	for (auto &item : m_itemList) {
		if (!item) continue;
		auto *component = dynamic_cast<Component*>(item);
		if (!component) continue;

		m_componentList << component;
		component->initElements(0);
		m_switchList += component->switchList();
	}

	for (auto &circuit : m_circuitList) {
		if (!circuit) continue;

		circuit->createMatrixMap();
	}

	for (auto &item : m_itemList) {
		if (!item) continue;
		auto *component = dynamic_cast<Component*>(item);
		if (!component) continue;

		component->initElements(1);
	}

	for (auto &circuit : m_circuitList) {
		if (!circuit) continue;

		circuit->initCache();
		Simulator::self()->attachCircuit(circuit);
	}
}


void CircuitDocument::getPartition( Pin *pin, QPtrList<Pin> *pinList, QPtrList<Pin> *unassignedPins, bool onlyGroundDependent )
{
	if (!pin || !unassignedPins || !pinList) return;

	unassignedPins->removeAll(pin);

	if (pinList->contains(pin)) return;

	pinList->append(pin);

	const auto localConnectedPins = pin->localConnectedPins();
	for (auto &pin : localConnectedPins) {
		if (pin.isNull() || !pin) continue;
		getPartition(pin, pinList, unassignedPins, onlyGroundDependent);
	}

	const auto groundDependentPins = pin->groundDependentPins();
	for (auto &pin : groundDependentPins) {
		if (pin.isNull() || !pin) continue;
		getPartition(pin, pinList, unassignedPins, onlyGroundDependent);
	}

	if (!onlyGroundDependent) {
		const auto circuitDependentPins = pin->circuitDependentPins();
		for (auto &pin : circuitDependentPins) {
			if (pin.isNull() || !pin) continue;
			getPartition(pin, pinList, unassignedPins, onlyGroundDependent);
		}
	}
}


void CircuitDocument::splitIntoCircuits( QPtrList<Pin> *pinList )
{
	if (!pinList) return;

	// First: identify ground
	QPtrList<Pin> unassignedPins = *pinList;
	using PinListList = QList<QPtrList<Pin>>;
	PinListList pinListList;

	while (!unassignedPins.isEmpty()) {
		QPtrList<Pin> tempPinList;
		getPartition(*unassignedPins.begin(), &tempPinList, &unassignedPins, true);
		pinListList.append(tempPinList);
	}

	for (auto &list : pinListList) {
		Circuit::identifyGround(list);
	}

	while (!pinList->isEmpty()) {
		auto end = pinList->end();
		auto it = pinList->begin();

		while (
			it != end &&
			!(*it).isNull() &&
			!!(*it) &&
			(*it)->eqId() == -1
		) {
			++it;
		}

		if (it == end) break;

		auto &pin = *it;
		if (pin.isNull() || !pin) continue;

		Circuitoid circuitoid;
		recursivePinAdd(pin, &circuitoid, pinList);

		if (!tryAsLogicCircuit(&circuitoid)) {
			m_circuitList += createCircuit(&circuitoid);
		}
	}

	// Remaining pins are ground; tell them about it
	// TODO This is a bit hacky....
	for (auto &pin : *pinList) {
		if (pin.isNull() || !pin) continue;

		pin->setVoltage(0.0);
		auto elements = pin->elements();
		for (auto &element : elements) {
			if (!element) continue;

			LogicIn *logicIn = nullptr;
			if ((logicIn = dynamic_cast<LogicIn *>(element)))
			{
				logicIn->setLastState(false);
				logicIn->callCallback();
			}
		}
	}
}


void CircuitDocument::recursivePinAdd( Pin *pin, Circuitoid *circuitoid, QPtrList<Pin> *unassignedPins )
{
	if (!pin || !circuitoid || !unassignedPins) return;

	if (pin->eqId() != -1 )
		unassignedPins->removeAll(pin);

	if (circuitoid->contains(pin)) return;

	circuitoid->addPin(pin);

	if (pin->eqId() == -1) return;

	for (auto &pin : pin->localConnectedPins()) {
		if (pin.isNull() || !pin) continue;
		recursivePinAdd(pin, circuitoid, unassignedPins);
	}

	for (auto &pin : pin->groundDependentPins()) {
		if (pin.isNull() || !pin) continue;
		recursivePinAdd(pin, circuitoid, unassignedPins);
	}

	for (auto &pin : pin->circuitDependentPins()) {
		if (pin.isNull() || !pin) continue;
		recursivePinAdd(pin, circuitoid, unassignedPins);
	}

	for (auto &element : pin->elements()) {
		if (!element) continue;
		circuitoid->addElement(element);
	}
}


bool CircuitDocument::tryAsLogicCircuit( Circuitoid *circuitoid )
{
	if (!circuitoid) return false;

	if (circuitoid->elementList.isEmpty())
	{
		// This doesn't quite belong here...but whatever. Initialize all
		// pins to voltage zero as they won't get set to zero otherwise
		for (auto &pin : circuitoid->pinList) {
			if (pin.isNull() || !pin) continue;
			pin->setVoltage(0.0);
		}

		// A logic circuit only requires there to be no non-logic components,
		// and at most one LogicOut - so this qualifies
		return true;
	}

	QList<LogicIn *> logicInList;
	LogicOut *out = nullptr;

	uint logicInCount = 0;
	for (auto &element : circuitoid->elementList) {
		if (!element) continue;

		switch (element->type()) {
			case Element::Element_LogicOut:
				if (out) {
					return false;
				}
				out = static_cast<LogicOut *>(element);
				break;
			case Element::Element_LogicIn:
				++logicInCount;
				logicInList += static_cast<LogicIn *>(element);
				break;
			default:
				return false;
		}
	}

	if (out) {
		Simulator::self()->createLogicChain(out, logicInList, circuitoid->pinList);
	}
	else {
		// We have ourselves stranded LogicIns...so lets set them all to low
		for (auto &pin : circuitoid->pinList) {
			if (pin.isNull() || !pin) continue;
			pin->setVoltage(0.0);
		}

		for (auto &element : circuitoid->elementList) {
			if (!element) continue;

			auto *logicIn = static_cast<LogicIn *>(element);
			logicIn->setNextLogic(nullptr);
			logicIn->setElementSet(nullptr);
			if (logicIn->isHigh())
			{
				logicIn->setLastState(false);
				logicIn->callCallback();
			}
		}
	}

	return true;
}


Circuit *CircuitDocument::createCircuit( Circuitoid *circuitoid )
{
	if (!circuitoid) return 0l;

	auto *circuit = new Circuit();

	for (auto &pin : circuitoid->pinList) {
		if (pin.isNull() || !pin) continue;
		circuit->addPin(pin);
	}

	for (auto &element : circuitoid->elementList) {
		if (!element) continue;
		circuit->addElement(element);
	}

	return circuit;
}


void CircuitDocument::createSubcircuit()
{
	QPtrList<Item> itemList = m_selectList->items();
	for (auto &item : itemList) {
		if (item.isNull() || !item || !dynamic_cast<Component *>((Item *)item)) {
			item = nullptr;
		}
	}
	itemList.removeAll(nullptr);

	if (itemList.isEmpty()) {
		KMessageBox::sorry( activeView(), i18n("No components were found in the selection.") );
		return;
	}

	// Number of external connections
	const int extConCount = countExtCon(itemList);
	if (extConCount == 0) {
		KMessageBox::sorry( activeView(), i18n("No External Connection components were found in the selection.") );
		return;
	}

	bool ok = false;
	const QString name = KInputDialog::getText( "Subcircuit", "Name", QString::null, &ok, activeView() );
	if (!ok) return;

	SubcircuitData subcircuit;
	subcircuit.addItems(itemList);
	subcircuit.addNodes( getCommonNodes(itemList) );
	subcircuit.addConnectors( getCommonConnectors(itemList) );

	Subcircuits::addSubcircuit( name, subcircuit.toXML() );
}


int CircuitDocument::countExtCon( const QPtrList<Item> &itemList ) const
{
	int count = 0;
	for (auto &item : itemList) {
		if (!item) continue;

		if (item->type() == "ec/external_connection") {
			++count;
		}
	}
	return count;
}


bool CircuitDocument::isValidItem( const QString &itemId )
{
	return itemId.startsWith("ec/") || itemId.startsWith("dp/") || itemId.startsWith("sc/");
}


bool CircuitDocument::isValidItem( Item *item )
{
	return (dynamic_cast<Component*>(item) || dynamic_cast<DrawPart*>(item));
}


void CircuitDocument::displayEquations()
{
	qDebug() << "######################################################" << endl;
	int i = 1;
	for (auto &circuit : m_circuitList) {
		if (!circuit) continue;

		qDebug() << "Equation set "<<i<<":\n";
		circuit->displayEquations();
		i++;
	}
	qDebug() << "######################################################" << endl;
}


#include "moc_circuitdocument.cpp"
