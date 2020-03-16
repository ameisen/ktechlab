#pragma once

#include "pch.hpp"

#include "nonlinear.h"

struct DiodeSettings {
	void reset() {
		*this = DiodeSettings{};
	}

	current_t I_S = 1e-15;	///< Diode saturation current
	real N = 1.0;	///< Emission coefficient
	voltage_t V_B = 4.7;	///< Reverse breakdown
// 	double R = 0.001;	///< Series resistance
};

/**
This simulates a diode. The simulated diode characteristics are:

@li I_s: Diode saturation current
@li V_T: Thermal voltage = kT/4 = 25mV at 20 C
@li n: Emission coefficient, typically between 1 and 2
@li V_RB: Reverse breakdown (large negative voltage)
@li G_RB: Reverse breakdown conductance
@li R_D: Base resistance of diode

@short Simulates the electrical property of diode-ness
@author David Saxton
*/
class Diode : public NonLinear {
	using Super = NonLinear;
	public:
		Diode();
		~Diode() override = default;

		void update_dc() override;
		void add_initial_dc() override;
		Element::Type type() const override { return Element_Diode; }
		DiodeSettings settings() const { return m_diodeSettings; }
		void setDiodeSettings(const DiodeSettings &settings);
		/**
		 * Returns the current flowing through the diode
		 */
		current_t current() const;

	protected:
		void updateCurrents() override;
		void calc_eq();

		struct IG final {
			current_t current = 0.0;
			conductance_t conductance = 0.0;
		};

		IG calcIG(voltage_t V, bool conductance = false) const;
		void updateLim();

		DiodeSettings m_diodeSettings;

		conductance_t g_new = 0.0, g_old = 0.0;
		current_t I_new = 0.0, I_old = 0.0;
		voltage_t V_prev = 0.0;
		voltage_t V_lim = 0.0;
};
