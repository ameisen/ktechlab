#include "circuit.h"
#include "circuitdocument.h"
#include "element.h"
#include "elementset.h"
#include "logic.h"
#include "matrix.h"
#include "nonlinear.h"
#include "pin.h"
#include "reactive.h"
#include "wire.h"

#include <cmath>
#include <map>
#include <algorithm>

using PinVectorMap = std::multimap<int, QPtrVector<Pin>>;

//BEGIN class Circuit
Circuit::Circuit() :
	ElementSet_(std::make_unique<ElementSet>(this, 0, 0)), // why do we do this?
	LogicCacheBase_(std::make_unique<LogicCacheNode>())
{}

void Circuit::addPin( Pin *node ) {
	PinSet_.insert(node);
}

void Circuit::addElement( Element *element ) {
	if (ElementList_.contains(element))
		return;
	ElementList_.append(element);
}

bool Circuit::contains( Pin *pin ) {
	return PinSet_.contains(pin);
}

// static function
int Circuit::identifyGround(const QPtrList<Pin> &nodeList, int &highest) {
	// What this function does:
	// We are given a list of pins. First, we divide them into groups of pins
	// that are directly connected to each other (e.g. through wires or
	// switches). Then, each group of connected pins is looked at to find the
	// pin with the highest "ground priority", and this is taken to be
	// the priority of the group. The highest ground priority from all the
	// groups is recorded. If the highest ground priority found is the maximum,
	// then all the pins in groups with this priority are marked as ground
	// (their eq-id is set to -1). Otherwise, the first group of pins with the
	// highest ground priority found is marked as ground, and all others are
	// marked as non ground (their eq-id is set to 0).

	int curHighest = Pin::GroundType::Never;

	auto nodeSet = QPtrSet<Pin>::fromList(nodeList);
	nodeSet.remove(nullptr);

	// Now to give all the Pins ids
	PinVectorMap eqs;
	while (!nodeSet.isEmpty()) {
		QPtrSet<Pin> associated;
		QPtrVector<Pin> nodes;
		Pin *node = *nodeSet.begin();
		recursivePinAdd(node, nodeSet, associated, nodes);
		if (!nodes.empty()) {
			eqs.insert(std::make_pair(associated.size(), nodes));
		}
	}

	// Now, we want to look through the associated Pins,
	// to find the ones with the highest "Ground Priority". Anything with a lower
	// priority than Pin::GroundType::Never will not be considered
	int numGround = 0; // The number of node groups found with that priority
	for (auto &eqsPair : eqs) {
		int highPri = Pin::GroundType::Never; // The highest priority found in these group of nodes
		for (auto &pin : eqsPair.second) {
			highPri = std::min(highPri, pin->getGroundType());
		}

		if (highPri == curHighest) {
			++numGround;
		}
		else if (highPri < curHighest) {
			numGround = 1;
			curHighest = highPri;
		}
	}

	if (curHighest == Pin::GroundType::Never) {
		--curHighest;
		numGround = 0;
	}
	// If there are no Always Ground nodes, then we only want to set one of the nodes as ground
	else if (curHighest > Pin::GroundType::Always) {
		numGround = 1;
	}

	// Now, we can give the nodes their cnode ids, or tell them they are ground
	bool foundGround = false; // This is only used when we don't have a Always ground node
	for (auto &eqsPair : eqs) {
		bool ground = false;
		for (auto &pin : eqsPair.second) {
			ground |= (pin->getGroundType() <= curHighest);
		}

		if (ground && (!foundGround || curHighest == Pin::GroundType::Always)) {
			for (auto &pin : eqsPair.second) {
				pin->setEqId(-1);
			}
			foundGround = true;
		}
		else {
			for (auto &pin : eqsPair.second) {
				pin->setEqId(0);
			}
		}
	}

	highest = curHighest;
	return numGround;
}

void Circuit::init() {
	int countCBranches = 0;
	for (auto *element : ElementList_) {
		if (!element) continue;
		countCBranches += element->numCBranches();
	}

	// Now to give all the Pins ids
	int groundCount = 0;
	PinVectorMap eqs;
	QPtrSet<Pin> unassignedNodes = PinSet_;
	unassignedNodes.remove(nullptr);

	while (!unassignedNodes.isEmpty()) {
		QPtrSet<Pin> associated;
		QPtrVector<Pin> nodes;
		Pin *node = *unassignedNodes.begin();
		if (recursivePinAdd(node, unassignedNodes, associated, nodes)) {
			groundCount++;
		}
		if (!nodes.isEmpty()) {
			eqs.insert(std::make_pair(associated.size(), nodes));
		}
	}

	const auto countCNodes = eqs.size() - groundCount;

	LogicCacheBase_ = nullptr;

	ElementSet_ = std::make_unique<ElementSet>(
		this,
		countCNodes,
		countCBranches
	);

	NonLogicCount_ = countCNodes + countCBranches;

	// Now, we can give the nodes their cnode ids, or tell them they are ground
	QuickVector &x = ElementSet_->x();
	int i = 0;

	for (auto &eqsPair : eqs) {
		bool foundGround = false;

		for (auto &pin : eqsPair.second) {
			if (!pin) continue;
			foundGround |= (pin->eqId() == Pin::EquationID::Ground);
		}

		if (foundGround)
			continue;

		bool foundEnergyStoragePin = false;

		for (auto &pin : eqsPair.second) {
			if (!pin) continue;

			pin->setEqId(i);

			bool energyStorage = false;
			const auto &elements = pin->elements();
			for (auto &element : elements) {
				if (!element) continue;

				switch (element->type()) {
					case Element::Element_Capacitance:
					case Element::Element_Inductance:
						energyStorage = true;
						break;
					default:
						break;
				}

				if (energyStorage) {
					break;
				}
			}

			// A pin attached to an energy storage pin overrides one that doesn't.
			// If the two pins have equal status with in this regard, we pick the
			// one with the highest absolute voltage on it.
			if (foundEnergyStoragePin && !energyStorage)
				continue;

			voltage_t v = pin->voltage();

			if (energyStorage && !foundEnergyStoragePin) {
				foundEnergyStoragePin = true;
				x[i] = v;
				continue;
			}

			if (std::abs(v) > std::abs(x[i])) {
				x[i] = v;
			}
		}
		i++;
	}

	// And add the elements to the elementSet
	for (auto &element : ElementList_) {
		if (!element) continue;
		// We don't want the element to prematurely try to do anything,
		// as it doesn't know its actual cnode ids yet
		element->setCNodes();
		element->setCBranches();
		ElementSet_->addElement(element);
	}

	// And give the branch ids to the elements
	i = 0;
	for (auto &element : ElementList_) {
		if (!element) continue;

		const auto numCBranches = element->numCBranches();
		assert(numCBranches >= 0 && numCBranches <= 3);
		element->setCBranchesRange(numCBranches, i);
		i += numCBranches;
	}
}

void Circuit::initCache() {
	ElementSet_->updateInfo();

	bool canCache = true;

	LogicOutList_.reserve(ElementList_.size());

	for (auto &element : ElementList_) {
		if (!element) {
			continue;
		}

		switch (element->type()) {
			case Element::Element_LogicOut: {
				LogicOutList_ << static_cast<LogicOut *>(element);
				break;
			}

			case Element::Element_CurrentSignal:
			case Element::Element_VoltageSignal:
			case Element::Element_Capacitance:
			case Element::Element_Inductance: {
				canCache = false;
				break;
			}

			default:
				break;
		}
	}

	if (canCache) {
		LogicCacheBase_ = std::make_unique<LogicCacheNode>();
	}
	else {
		LogicCacheBase_.reset(nullptr);
	}
}

void Circuit::setCacheInvalidated() {
	if (!LogicCacheBase_)	return;

	LogicCacheBase_->data = QuickVector{0};

	LogicCacheBase_->high = nullptr;
	LogicCacheBase_->low = nullptr;

	LogicCacheBase_->hasData = false;
}

void Circuit::cacheAndUpdate() {
	auto *node = LogicCacheBase_.get();
	if (!node) return;

	for (auto *logicOut : LogicOutList_) {
		if (!logicOut) continue;

		if (logicOut->outputState()) {
			if (!node->high) {
				node->high = std::make_unique<LogicCacheNode>();
			}
			node = node->high.get();
		}
		else {
			if (!node->low) {
				node->low = std::make_unique<LogicCacheNode>();
			}
			node = node->low.get();
		}
	}

	if (node->hasData) {
		ElementSet_->x() = node->data;
		ElementSet_->updateInfo();
		return;
	}

	if (ElementSet_->containsNonLinear()) {
		ElementSet_->doNonLinear(
			150,
			1.0e-10,
			1.0e-13
		);
	}
	else {
		ElementSet_->doLinear(true);
	}

	node->data = ElementSet_->x();
	node->hasData = true;
}

void Circuit::createMatrixMap() {
	ElementSet_->createMatrixMap();
}

bool Circuit::recursivePinAdd(Pin *node, QPtrSet<Pin> &unassignedNodes, QPtrSet<Pin> &associated, QPtrVector<Pin> &nodes) {
	if (!node || !unassignedNodes.remove(node))
		return false;

	bool foundGround = (node->eqId() == Pin::EquationID::Ground);

	const auto &circuitDependentPins = node->circuitDependentPins();
	associated.reserve(associated.size() + circuitDependentPins.size());
	for (auto &pin : circuitDependentPins) {
		associated.insert(pin);
	}

	nodes.append(node);

	const auto localConnectedPins = node->localConnectedPins();
	nodes.reserve(nodes.size() + localConnectedPins.size());

	// TODO : Rewrite this into a loop to avoid stack recursion
	for (auto &pin : localConnectedPins) {
		foundGround |= recursivePinAdd(pin, unassignedNodes, associated, nodes);
	}

	return foundGround;
}

void Circuit::doNonLogic() {
	if (NonLogicCount_ <= 0)
		return;

	if (isCacheable()) {
		if (!ElementSet_->b().isChanged && !ElementSet_->matrix().isChanged()) {
			return;
		}
		cacheAndUpdate();
		updateNodalVoltages();
		ElementSet_->b().isChanged = false;
		return;
	}

	stepReactive();

	if (ElementSet_->containsNonLinear()) {
		ElementSet_->doNonLinear(
			10,
			1.0e-9,
			1.0e-12
		);
		updateNodalVoltages();
	}
	else if (ElementSet_->doLinear(true)) {
		updateNodalVoltages();
	}
}

void Circuit::stepReactive() {
	for (auto *element : ElementList_) {
		if (element && element->isReactive()) {
			static_cast<Reactive *>(element)->time_step();
		}
	}
}

void Circuit::updateNodalVoltages() {
	auto _cnodes = ElementSet_->cnodes();

	for (auto &node : PinSet_) {
		if (!node) continue;

		int i = node->eqId();
		if (i == Pin::EquationID::Ground) {
			node->setVoltage(0.0);
		}
		else {
			const auto v = _cnodes[i]->v;
			node->setVoltage(std::isfinite(v) ? v : 0.0);
		}
	}
}

void Circuit::updateCurrents() {
	for (auto *element : ElementList_) {
		if (!element) continue;
		element->updateCurrents();
	}
}

void Circuit::displayEquations() {
	ElementSet_->displayEquations();
}
//END class Circuit
