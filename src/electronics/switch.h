#pragma once

#include "pch.hpp"

#include <QPointer>
#include <QObject>

#include <memory>
#include <random>

class CircuitDocument;
class Component;
class Pin;
class Resistance;
class QTimer;

/**
@author David Saxton
*/

class Switch : public QObject {
	Q_OBJECT

	using RandomEngine = std::default_random_engine;
public:
	static constexpr const int DefaultBouncePeriodMS = 5;
	static constexpr const resistance_t BounceResistance = 10000.0;

	enum class State : bool {
		Open = false,
		Closed = true
	};

	Switch(Component *parent, Pin *p1, Pin *p2, State state);
	~Switch() override;
	/**
	 * If bouncing has been set to true, then the state will not switch
	 * immediately to that given.
	 */
	void setState(State state);
	State getState() const { return State_; }

	bool isOpen() const { return State_ == State::Open; }
	bool isClosed() const { return State_ == State::Closed; }

	/**
	 * Tell the switch whether to bounce or not, for the given duration,
	 * when the state is changed.
	 */
	void setBounce(bool bounce, int msec = DefaultBouncePeriodMS);

	bool isBouncing() const { return BouncingResistance_ != nullptr; }

	/**
	 * Attempts to calculate the current that is flowing through the switch.
	 * (If all the connectors at one of the ends know their currents, then
	 * this switch will give the current to the pins at either end).
	 * @return whether it was successful.
	 * @see CircuitDocument::calculateConnectorCurrents
	 */
	bool calculateCurrent();

	Component * getComponent() const { return Component_; }

protected slots:
	/**
	 * Called from a QTimer timeout - our bouncing period has come to an
	 * end. This will then fully disconnect or connect the pins depending
	 * on the current state.
	 */
	void stopBouncing();

protected:
	void startBouncing();
	/**
	 * Tell the switch to continue bouncing (updates the resistance value).
	 * Called from the simulator.
	 */
	void bounce();

	void reconnectAll();

	llong BounceStart_ = 0ll; // Simulator time that bouncing started
	Resistance *BouncingResistance_ = nullptr;
	Component *Component_ = nullptr;
	QTimer *StopBouncingTimer_ = nullptr;
	std::unique_ptr<RandomEngine> RandomEngine_;
	QPointer<Pin> Pins_[2];
	int BouncePeriod_ = DefaultBouncePeriodMS;
	State State_ = State::Open;
	bool Bounce_ = false;
};
