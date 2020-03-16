#include "pin.h"

#include <cassert>

#include <QDebug>

#include <algorithm>

Pin::Pin(ECNode *parent) :
	m_pECNode(parent)
{
	assert(parent);
}

Pin::~Pin() {
	for (auto &wire : m_inputWireList) {
		if (!wire) continue;
		delete wire.data();
	}

	for (auto &wire : m_outputWireList) {
		if (!wire) continue;
		delete wire.data();
	}
}

QPtrList<Pin> Pin::localConnectedPins() const {
	QPtrList<Pin> pins;
	pins.reserve(m_inputWireList.size() + m_outputWireList.size() + m_switchConnectedPins.size());

	for (auto &wire : m_inputWireList) {
		if (!wire) continue;
		pins << wire->startPin();
	}

	for (auto &wire : m_outputWireList) {
		if (!wire) continue;
		pins << wire->endPin();
	}

	for (auto &pin : m_switchConnectedPins) {
		if (!pin) continue;
		pins << pin;
	}

	return pins;
}

void Pin::setSwitchConnected(Pin *pin, bool isConnected) {
	if (!pin) return;

	if (isConnected) {
		m_switchConnectedPins.insert(pin);
	}
	else {
		m_switchConnectedPins.remove(pin);
	}
}

void Pin::setSwitchCurrentsUnknown() {
    if (!m_switchList.empty()) {
      m_switchList.removeFirst();
    }
		else {
  		qDebug() << "Pin::setSwitchCurrentsUnknown - WARN - unexpected empty switch list";
    }
    m_unknownSwitchCurrents = m_switchList;
}

void Pin::addCircuitDependentPin(Pin *pin) {
	if (pin && !m_circuitDependentPins.contains(pin)) {
		m_circuitDependentPins.append(pin);
	}
}

void Pin::addGroundDependentPin(Pin *pin) {
	if (pin && !m_groundDependentPins.contains(pin)) {
		m_groundDependentPins.append(pin);
	}
}

void Pin::removeDependentPins() {
	m_circuitDependentPins.clear();
	m_groundDependentPins.clear();
}


void Pin::addElement(Element *e) {
	if (e && !m_elementList.contains(e)) {
		m_elementList.append(e);
	}
}

void Pin::removeElement(Element *e) {
	m_elementList.removeAll(e);
}

void Pin::addSwitch(Switch *sw) {
	if (sw && !m_switchList.contains(sw)) {
		m_switchList << sw;
	}
}

void Pin::removeSwitch(Switch *sw) {
	m_switchList.removeAll(sw);
}

void Pin::addInputWire(Wire *wire) {
	if (wire && !m_inputWireList.contains(wire)) {
		m_inputWireList << wire;
	}
}

void Pin::addOutputWire(Wire *wire) {
	if (wire && !m_outputWireList.contains(wire)) {
		m_outputWireList << wire;
	}
}

bool Pin::calculateCurrentFromWires() {
	auto &inputs = m_inputWireList;
	auto &outputs = m_outputWireList;

	inputs.removeAll(nullptr);
	outputs.removeAll(nullptr);

	m_current = 0.0;
	m_bCurrentIsKnown = false;

	// If a pin has no outputs, then it cannot have a circuituous connection and thus
	// cannot pass current
	if (outputs.isEmpty() || inputs.isEmpty()) {
		return false;
	}

	double current = 0.0;

	bool inputsFound = false;
	for (auto &wire : inputs) {
		if (Ptr::isNull(wire)) continue;
		inputsFound = true;
		if (!wire->currentIsKnown()) {
			return false;
		}

		current -= wire->current();
	}

	if (!inputsFound) {
		return false;
	}

	bool outputsFound = false;
	for (auto &wire : outputs) {
		if (Ptr::isNull(wire)) continue;
		outputsFound = true;
		if (!wire->currentIsKnown()) {
			return false;
		}

		current += wire->current();
	}

	if (!outputsFound) {
		return false;
	}

	m_current = current;

	m_bCurrentIsKnown = true;
	return true;
}
