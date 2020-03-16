#include "pin.h"
#include "wire.h"

#include <QDebug>

#include <cassert>

Wire::Wire(Pin *startPin, Pin *endPin) :
	m_pStartPin(startPin),
	m_pEndPin(endPin)
{
	assert(startPin);
	assert(endPin);

	m_pStartPin->addOutputWire(this);
	m_pEndPin->addInputWire(this);
}

bool Wire::calculateCurrent() {
	m_bIsConnected = !m_pStartPin.isNull() && !m_pEndPin.isNull();

	m_bCurrentIsKnown = false;
	m_current = 0.0;

	if (!m_pStartPin.isNull() && m_pStartPin->currentIsKnown() && m_pStartPin->numWires() < 2) {
		m_current = m_pStartPin->current();
		m_bCurrentIsKnown = true;
		return true;
	}

	if (!m_pEndPin.isNull() && m_pEndPin->currentIsKnown() && m_pEndPin->numWires() < 2) {
		m_current = -m_pEndPin->current();
		m_bCurrentIsKnown = true;
		return true;
	}

	if (!m_pStartPin.isNull() && m_pStartPin->currentIsKnown()) {
		auto localCurrent = m_pStartPin->current();
		bool currentKnown = true;

		for (auto &wire : m_pStartPin->outputWireList()) {
			if (!wire || wire == this) continue;

			if (wire->currentIsKnown()) {
				localCurrent -= wire->current();
			}
			else {
				currentKnown = false;
			}
		}

		for (auto &wire : m_pStartPin->inputWireList()) {
			if (!wire || wire == this) continue;

			if (wire->currentIsKnown()) {
				localCurrent += wire->current();
			}
			else {
				currentKnown = false;
			}
		}

		if (currentKnown) {
			m_current = localCurrent;
			m_bCurrentIsKnown = true;
			return true;
		}
	}

	if (!m_pEndPin.isNull() && m_pEndPin->currentIsKnown()) {
		double localCurrent = -m_pEndPin->current();
		bool currentKnown = true;

		for (auto &wire : m_pEndPin->outputWireList()) {
			if (!wire || wire == this) continue;

			if (wire->currentIsKnown()) {
				localCurrent += wire->current();
			}
			else {
				currentKnown = false;
			}
		}

		for (auto &wire : m_pEndPin->inputWireList()) {
			if (!wire || wire == this) continue;

			if (wire->currentIsKnown()) {
				localCurrent -= wire->current();
			}
			else {
				currentKnown = false;
			}
		}

		if (currentKnown) {
			m_current = localCurrent;
			m_bCurrentIsKnown = true;
			return true;
		}
	}

	return false;
}

voltage_t Wire::getVoltage() const {
	if (!m_pStartPin.isNull()) {
		return m_pStartPin->voltage();
	}

	if (!m_pEndPin.isNull()) {
		return -m_pEndPin->voltage();
	}

	return 0.0;
}

void Wire::setCurrentKnown(bool known) {
	if (!(m_bCurrentIsKnown = known)) {
		m_current = 0.0;
	}
}
