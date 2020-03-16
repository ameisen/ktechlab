#pragma once

#include "pch.hpp"

#include "pin.h"

#include <qpointer.h>
#include <qobject.h>

class Pin;

/**
@author David Saxton
*/
class Wire final : public QObject {
	Q_OBJECT

	public:
		Wire(Pin *startPin, Pin *endPin);
		~Wire() override = default;

		/**
		 * Attempts to calculate the current that is flowing through
		 * the connector. Returns true if successfuly, otherwise returns false
		 */
		bool calculateCurrent();
		/**
		 * Returns true if the current flowing through the connector is known
		 */
		bool currentIsKnown() const { return m_bCurrentIsKnown; }

		// Returns whether or not the wire is connected to anything.
		bool isConnected() const { return m_bIsConnected; }

		/**
		 * Set whether the actual current flowing into this node is known (in some
		 * cases - such as this node being ground - it is not known, and so the
		 * value returned by current() cannot be relied on.
		 */
		void setCurrentKnown( bool known );
		/**
		 * Returns the current flowing through the connector.
		 * This only applies for electronic connectors
		 */
		current_t getCurrent() const { return m_current; }
		current_t current() const { return getCurrent(); }
		/**
		 * Returns the voltage at the connector. This is an average of the
		 * voltages at either end.
		 */
		voltage_t getVoltage() const;
		voltage_t voltage() const { return getVoltage(); }

		Pin *startPin() const { return m_pStartPin; }
		Pin *endPin() const { return m_pEndPin; }

// protected:

private:
	QPointer<Pin> m_pStartPin;
	QPointer<Pin> m_pEndPin;
	current_t m_current = 0.0;
	bool m_bCurrentIsKnown = false;
	bool m_bIsConnected = false;
};
