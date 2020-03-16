#pragma once

#include "component.h"

/**
@short Simple diode
@author David Saxton
*/
class ECDiode : public Component {
	using Super = Component;
public:
	static constexpr const cstring Category = "ec";
	static constexpr const cstring ID = "diode";
	static constexpr const cstring Name = "Diode";

	ECDiode(ICNDocument *icnDocument, bool newItem, const char *id = nullptr);
	~ECDiode() override = default;

	static Item* construct(ItemDocument *itemDocument, bool newItem, const char *id);
	static LibraryItem *libraryItem();

protected:
	void drawShape(QPainter &p) override;
	void dataChanged() override;

	Property &saturationCurrent;
	Property &emissionCoefficient;
	Property &breakdownVoltage;
	Diode *m_diode = nullptr;
};
