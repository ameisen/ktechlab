#include <cmath>

#include <QDebug>
#include <QTimer>

#include "circuitdocument.h"
#include "component.h"
#include "ecnode.h"
#include "pin.h"
#include "resistance.h"
#include "simulator.h"
#include "switch.h"

Switch::Switch(Component *parent, Pin *p1, Pin *p2, State state) :
	Component_(parent),
	StopBouncingTimer_(new QTimer(this)),
	Pins_({p1, p2}),
	State_(State(!bool(state)))
{
	connect(StopBouncingTimer_, SIGNAL(timeout()), this, SLOT(stopBouncing()));

	// Force update
	setState(state);
}

Switch::~Switch() {
	if (Pins_[0]) {
		Pins_[0]->setSwitchConnected(Pins_[1], false);
	}
	if (Pins_[1]) {
		Pins_[1]->setSwitchConnected(Pins_[0], false);
	}
}

void Switch::setState(State state) {
	if (State_ == state) return;

	State_ = state;

	if (Bounce_) {
		startBouncing();
	}
	else {
		reconnectAll();
	}
}

void Switch::setBounce(bool bounce, int msec) {
	Bounce_ = bounce;
	BouncePeriod_ = msec;
}

void Switch::startBouncing() {
	if (BouncingResistance_) {
		// Already active?
		return;
	}

	if (!Component_->circuitDocument()) {
		return;
	}

	BouncingResistance_ = Component_->createResistance(Pins_[0], Pins_[1], BounceResistance);
	BounceStart_ = Simulator::self()->time();

	//FIXME: I broke the bounce feature when I cleaned this out of the simulator,
	// should probably be put into circuit document or some other solution which doesn't
	// contaminate that many other classes.

	//	Simulator::self()->attachSwitch( this );
	// 	qDebug() << "BounceStart_="<<BounceStart_<<" BouncePeriod_="<<BouncePeriod_<<endl;

	// initialize random generator
	RandomEngine_ = std::make_unique<RandomEngine>(std::random_device{}());

	// Give our bounce resistor an initial value
	bounce();
}

void Switch::bounce() {
	const auto bouncedTimeMS = ((Simulator::self()->time() - BounceStart_) * 1000ll) / LOGIC_UPDATE_RATE;

	if (bouncedTimeMS >= BouncePeriod_) {
		if (!StopBouncingTimer_->isActive()) {
    	StopBouncingTimer_->setSingleShot(true);
			StopBouncingTimer_->start(0);
    }

		return;
	}

	std::uniform_real_distribution<conductance_t> distribution(0.0, 1.0);
	// 4th power of the conductance seems to give a nice distribution
	const auto randomConductance = std::pow(distribution(*RandomEngine_.get()), 4.0);

	BouncingResistance_->setConductance(randomConductance);
}

void Switch::stopBouncing() {
	Component_->removeElement(BouncingResistance_, true);
	BouncingResistance_ = nullptr;
	RandomEngine_ = nullptr;

	reconnectAll();
}

void Switch::reconnectAll() {
	const bool connected = isClosed();

	if (Pins_[0]) {
		Pins_[0]->setSwitchConnected(Pins_[1], connected);
	}
	if (Pins_[1]) {
		Pins_[1]->setSwitchConnected(Pins_[0], connected);
	}

	if (const auto &circuitDocument = Component_->circuitDocument()) {
		circuitDocument->requestAssignCircuits();
	}
}

bool Switch::calculateCurrent() {
	if (!Pins_[0] || !Pins_[1]) {
		return false;
	}

	if (isOpen()) {
		Pins_[0]->setSwitchCurrentKnown(this);
		Pins_[1]->setSwitchCurrentKnown(this);
		return true;
	}

	current_t current = 0.0;
	bool currentKnown = false;

	bool reversePolarity = true;

	for (auto &pin : Pins_) {
		reversePolarity = !reversePolarity;

		if (!pin) continue;

		const auto &inputs = pin->inputWireList();
		const auto &outputs = pin->outputWireList();

		currentKnown = true;
		current = 0.0;

		for (auto &wire : inputs) {
			if (!wire) continue;

			if (!wire->currentIsKnown()) {
				currentKnown = false;
				break;
			}

			current += wire->current();
		}

		if (!currentKnown) continue;

		for (auto &wire : outputs) {
			if (!wire) continue;

			if (!wire->currentIsKnown()) {
				currentKnown = false;
				break;
			}

			current -= wire->current();
		}

		if (!currentKnown) continue;

		if (reversePolarity) {
			current = -current;
		}
	}

	if (!currentKnown) return false;

	// TODO : I don't believe that this is correct, because either pin can be in/out.
	for (auto &pin : Pins_) {
		current = -current;
		if (!pin) continue;

		pin->setSwitchCurrentKnown(this);
		pin->mergeCurrent(current);
	}

	return true;
}

#include "moc_switch.cpp"
