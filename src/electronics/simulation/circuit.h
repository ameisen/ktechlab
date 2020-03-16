#pragma once

#include "pch.hpp"

#include <QPointer>
#include <QStringList>
#include <QList>

#include "elementset.h"
#include "math/quickvector.h"

#include <memory>

class CircuitDocument;
class Wire;
class Pin;
class Element;
class LogicOut;

/**
Usage of this class (usually invoked from CircuitDocument):
(1) Add Wires, Pins and Elements to the class as appropriate
(2) Call init to initialize the simulation
(3) Control the simulation with step()

This class can be considered a bridge between the gui-tainted CircuitDocument - specific
to this implementation, and the pure untainted ElementSet. Please keep it that way.

@short Simulates a collection of components
@author David Saxton
*/
class Circuit final {
	struct LogicCacheNode final {
		LogicCacheNode() = default;
		~LogicCacheNode() = default;

		QuickVector data = QuickVector{0};
		std::unique_ptr<LogicCacheNode> high;
		std::unique_ptr<LogicCacheNode> low;
		bool hasData = false;
	};

public:
	Circuit();
	~Circuit() = default;

	void addPin(Pin *node);
	void addElement(Element *element);

	bool contains(Pin *pin);
	bool containsNonLinear() const { return ElementSet_->containsNonLinear(); }

	void init();
	/**
		* Called after everything else has been setup - before doNonLogic or
		* doLogic are called for the first time. Preps the circuit.
		*/
	void initCache();
	/**
		* Marks all cached results as invalidated and removes them.
		*/
	void setCacheInvalidated();
	/**
		* Solves for non-logic elements
		*/
	void doNonLogic();
	/**
		* Solves for logic elements (i.e just does fbSub)
		*/
	void doLogic() { ElementSet_->doLinear(false); }

	void displayEquations();
	void updateCurrents();

	void createMatrixMap();
	/**
		* This will identify the ground node and non-ground nodes in the given set.
		* Ground will be given the eqId -1, non-ground of 0.
		* @param highest The highest ground type of the groundnodes found. If no
		ground nodes were found, this will be (GroundType::Never-1).
		* @returns the number of ground nodes. If all nodes are at or below the
		* 			GroundType::Never threshold, then this will be zero.
		*/
	static int identifyGround(const QPtrList<Pin> &nodeList, int &highest);
	static int identifyGround(const QPtrList<Pin> &nodeList) {
		int highest = 0;
		return identifyGround(nodeList, highest);
	}

	void setNextChanged(Circuit *circuit, int chain) { NextChanged_[chain] = circuit; }
	Circuit * nextChanged(int chain) const { return NextChanged_[chain]; }

	bool isCacheable() const { return bool(LogicCacheBase_); }

protected:
	void cacheAndUpdate();
	/**
		* Update the nodal voltages from those calculated in ElementSet
		*/
	void updateNodalVoltages();
	/**
		* Step the reactive elements.
		*/
	void stepReactive();
	/**
		* Returns true if any of the nodes are ground
		*/
	static bool recursivePinAdd(Pin *node, QPtrSet<Pin> &unassignedNodes, QPtrSet<Pin> &associated, QPtrVector<Pin> &nodes);

	QPtrSet<Pin> PinSet_;
	QList<Element *> ElementList_;
	QList<LogicOut *> LogicOutList_;
	Circuit * NextChanged_[2] = { nullptr, nullptr };
	std::unique_ptr<ElementSet> ElementSet_;
	std::unique_ptr<LogicCacheNode> LogicCacheBase_;

	int NonLogicCount_ = 0;

public:
	// Sometimes, things make more sense being public than requiring tons of setters/getters.
	bool canAddChanged = true;
};
