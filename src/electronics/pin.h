#pragma once

#include "pch.hpp"

#include "switch.h"
#include "wire.h"

#include <QPointer>
#include <QObject>
#include <QList>
#include <QSet>

#include <algorithm>

class ECNode;
class Element;
class Pin;
class Wire;

/**
@author David Saxton
*/
class Pin final : public QObject {
	Q_OBJECT

	public:
		/**
		 * Priorities for ground pin. GroundType::Always will (as expected) always assign
		 * the given pin as ground, GroundType::Never will never do. If no GroundType::Always pins
		 * exist, then the pin with the highest priority will be set as ground -
		 * if there is at least one pin that is not of ground type GroundType::Never. These
		 * are only predefined recommended values, so if you choose not to use one
		 * of these, please respect the priorities with respect to the examples, and
		 * always specify a priority between 0 and 20.
		 * @see groundLevel
		 */
		struct GroundType final {
			GroundType() = delete;
			enum Values : int {
				Always = 0, // ground
				High = 5, // voltage points
				Medium = 10, // voltage sources
				Low = 15, // current sources
				Never = 20 // everything else
			};
		};

		struct EquationID final {
			EquationID() = delete;
			enum Values : int {
				Unset = -2,
				Ground = -1,
				NonGround = 0
			};
		};

		Pin(ECNode *parent);
		~Pin() override;

		ECNode * parentECNode() const { return m_pECNode; }
		/**
		 * This function returns the pins that are directly connected to this pins:
		 * either at the ends of connected wires, or via switches.
		 */
		QPtrList<Pin> localConnectedPins() const;
		/**
		 * Adds/removes the given pin to the list of ones that this pin is/isn't
		 * connected to via a switch.
		 */
		void setSwitchConnected(Pin *pin, bool isConnected);
		/**
		 * After calculating the nodal voltages in the circuit, this function should
		 * be called to tell the pin what its voltage is.
		 */
		void setVoltage(voltage_t v) { m_voltage = v; }
		/**
		 * Returns the voltage as set by setVoltage.
		 */
		voltage_t getVoltage() const { return m_voltage; }
		voltage_t voltage() const { return getVoltage(); }

		/**
		 * After calculating nodal voltages, each component will be called to tell
		 * its pins what the current flowing *into* the component is. This sets it
		 * to zero in preparation to merging the current.
		 */
		void resetCurrent() { m_current = 0.0; }
		/**
		 * Adds the given current to that already flowing into the pin.
		 * @see setCurrent
		 */
		void mergeCurrent(current_t i) { m_current += i; }
		/**
		 * Returns the current as set by mergeCurrent.
		 */
		current_t getCurrent() const { return m_current; }
		current_t current() const { return getCurrent(); }
		/**
		 * In many cases (such as if this pin is a ground pin), the current
		 * flowing into the pin has not been calculated, and so the value
		 * returned by current() cannot be trusted.
		 */
		void setCurrentKnown(bool isKnown) { m_bCurrentIsKnown = isKnown; }
		/**
		 * Tell thie Pin that none of the currents from the switches have yet
		 * been merged.
		 */
		void setSwitchCurrentsUnknown(); // { m_switchList.erase( 0l ); m_unknownSwitchCurrents = m_switchList; }
		/**
		 * This returns the value given by setCurrentKnown AND'd with whether
		 * we know the current from each switch attached to this pin.
		 * @see setCurrentKnown
		 */
		bool currentIsKnown() const { return m_bCurrentIsKnown && m_unknownSwitchCurrents.isEmpty(); }
		/**
		 * Tells the Pin that the current from the given switch has been merged.
		 */
		void setSwitchCurrentKnown(Switch *sw) { m_unknownSwitchCurrents.removeAll(sw); }
		/**
		 * Tries to calculate the Pin current from the input / output wires.
		 * @return whether was successful.
		 */
		bool calculateCurrentFromWires();
		/**
		 * Sets the "ground type" - i.e. the priority that this pin has for being
		 * ground over other pins in the circuit. Lower gt = higher priority. It's
		 * recommended to use Pin::GroundType.
		 */
		void setGroundType(int gt) { m_groundType = gt; }
		/**
		 * Returns the priority for ground.
		 */
		int getGroundType() const { return m_groundType; }
		/**
		 * Adds a dependent pin - one whose voltages will (or might) affect the
		 * voltage of this pin. This is set by Component.
		 */
		void addCircuitDependentPin(Pin *pin);
		/**
		 * Adds a dependent pin - one whose voltages will (or might) affect the
		 * voltage of this pin. This is set by Component.
		 */
		void addGroundDependentPin(Pin *pin);
		/**
		 * Removes all Circuit and Ground dependent pins.
		 */
		void removeDependentPins();
		/**
		 * Returns the ids of the pins whose voltages will affect this pin.
		 * @see void setDependentPins( QStringList ids )
		 */
		const QPtrList<Pin> & circuitDependentPins() const { return m_circuitDependentPins; }
		/**
		 * Returns the ids of the pins whose voltages will affect this pin.
		 * @see void setDependentPins( QStringList ids )
		 */
		const QPtrList<Pin> & groundDependentPins() const { return m_groundDependentPins; }
		/**
		 * Use this function to set the pin identifier for equations,
		 * which should be done every time new pins are registered.
		 */
		void setEqId(int id) { m_eqId = id; }
		/**
		 * The equation identifier.
		 * @see setEqId
		 */
		int eqId() const { return m_eqId; }
		/**
		 * Returns a list of elements that will affect this pin (e.g. if this
		 * pin is part of a resistor, then that list will contain a pointer to a
		 * Resistance element)
		 */
		const QList<Element *> & elements() const { return m_elementList; }
		/**
		 * Adds an element to the list of those that will affect this pin.
		 */
		void addElement(Element *e);
		/**
		 * Removes an element from the list of those that will affect this pin.
		 */
		void removeElement(Element *e);
		/**
		 * Adds an switch to the list of those that will affect this pin.
		 */
		void addSwitch(Switch *e);
		/**
		 * Removes an switch from the list of those that will affect this pin.
		 */
		void removeSwitch(Switch *e);

		void addInputWire(Wire *wire);
		void addOutputWire(Wire *wire);
		void removeWire(Wire *wire);
		const QPtrList<Wire> & inputWireList() const { return m_inputWireList; }
		const QPtrList<Wire> & outputWireList() const { return m_outputWireList; }
		int numWires() const { return m_inputWireList.size() + m_outputWireList.size(); }

	protected:
		// TODO : Change these to QSets
		QPtrList<Pin> m_circuitDependentPins;
		QPtrList<Pin> m_groundDependentPins;
		QPtrSet<Pin> m_switchConnectedPins;

		QList<Element *> m_elementList;

		QPtrList<Wire> m_inputWireList;
		QPtrList<Wire> m_outputWireList;

		QPtrList<Switch> m_switchList;
		QPtrList<Switch> m_unknownSwitchCurrents;

		ECNode * const m_pECNode = nullptr;

		double m_voltage = 0.0;
		double m_current = 0.0;

		int m_eqId = EquationID::Unset;
		int m_groundType = GroundType::Never;

		bool m_bCurrentIsKnown = false;
};
