#pragma once

#include "pch.hpp"

#include "component.h"

#include "electronics/simulation/resistance.h"

class Resistance;

/**
@short Signal Lamp - glows when current flows
@author David Saxton
*/
class ECSignalLamp final : public Component {
	using Super = Component;
public:
	static constexpr const cstring Category = "ec";
	static constexpr const cstring ID = "signal_lamp";
	static constexpr const cstring Name = "Signal Lamp";

	ECSignalLamp(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
	~ECSignalLamp() override = default;

	static Item* construct(ItemDocument *itemDocument, bool newItem, const char *id);
	static LibraryItem *libraryItem();

	void stepNonLogic() override;
	bool doesStepNonLogic() const override { return true; }

private:
	bool isCurrentKnown() const;

	void drawShape(QPainter &p) override;

	std::unique_ptr<Resistance> resistance;

	real avgPower = 0.0;
	power_t currentWattage = 0.0;
	uint advanceSinceUpdate = 0;
};
