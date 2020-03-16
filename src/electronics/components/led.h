#pragma once

#include "component.h"
#include "ecdiode.h"

/**
@short Simulates a LED
@author David Saxton
*/
class LED final : public ECDiode {
	using Super = ECDiode;

	static constexpr const current_t MinCurrent = 0.002;
	static constexpr const current_t MaxCurrent = 0.018;
public:
	static constexpr const cstring Category = "ec";
	static constexpr const cstring ID = "led";
	static constexpr const cstring Name = "LED";

	LED(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
	~LED() override = default;

	static Item* construct(ItemDocument *itemDocument, bool newItem, const char *id);
	static LibraryItem *libraryItem();

	void dataChanged() override;
	void stepNonLogic() override;
	bool doesStepNonLogic() const override { return true; }

	static real getBrightnessReal(current_t current, current_t minCurrentV = MinCurrent, current_t maxCurrentV = MaxCurrent);
	static int getBrightness(current_t current, current_t minCurrentV = MinCurrent, current_t maxCurrentV = MaxCurrent);

private:
	void drawShape(QPainter &p) override;

	Property &zeroColor;
	Property &minCurrent;
	Property &maxCurrent;
	Point3<real> color = {0.0, 0.0, 0.0};

	real brightness = 0.0;
};
