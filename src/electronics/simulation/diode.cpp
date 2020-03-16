#include "diode.h"
#include "elementset.h"
#include "matrix.h"

#include <cmath>

#include <algorithm>

//BEGIN class Diode
Diode::Diode() :
	Super()
{
	m_numCNodes = 2;
	updateLim();
}

void Diode::add_initial_dc() {
	g_new = g_old = I_new = I_old = V_prev = 0.0;
	update_dc();
}

current_t Diode::current() const {
	if (!b_status)
		return 0.0;

	return calcIG(p_cnode[0]->v - p_cnode[1]->v).current;
}

void Diode::updateCurrents() {
	if (!b_status)
		return;

	m_cnodeI[1] = current();
	m_cnodeI[0] = -m_cnodeI[1];
}

void Diode::update_dc() {
	if (!b_status)
		return;

	calc_eq();

	const auto g_diff = g_new - g_old;
	A_g( 0, 0 ) += g_diff;
	A_g( 1, 1 ) += g_diff;
	A_g( 0, 1 ) -= g_diff;
	A_g( 1, 0 ) -= g_diff;

	const auto I_diff = I_new - I_old;
	b_i( 0 ) -= I_diff;
	b_i( 1 ) += I_diff;

	g_old = g_new;
	I_old = I_new;
}

void Diode::calc_eq() {
	auto N = m_diodeSettings.N;
	auto V_B = m_diodeSettings.V_B;
// 	double R = m_diodeSettings.R;

	auto v = p_cnode[0]->v - p_cnode[1]->v;

	// adjust voltage to help convergence
	if ( V_B != 0.0 && v < std::min(0.0, -V_B + 10.0 * N ) ) {
		auto V = -(v + V_B);
		V = diodeVoltage( V, -(V_prev + V_B), N, V_lim );
		v = -(V + V_B);
	}
	else {
		v = diodeVoltage( v, V_prev, N, V_lim );
	}

	V_prev = v;

	auto ig = calcIG(v, true);
	g_new = ig.conductance;
	auto I_D = ig.current;

	I_new = I_D - (v * ig.conductance);
}

Diode::IG Diode::calcIG(voltage_t V, bool conductance) const {
	const auto I_S = m_diodeSettings.I_S;
	const auto N = m_diodeSettings.N;
	const auto V_B = m_diodeSettings.V_B;

	const auto g_tiny = (V < - 10.0 * V_T * N && V_B != 0.0) ? I_S : 0.0;

	IG result;

	if ( V >= (-3.0 * N * V_T) ) {
		result.current = diodeCurrent( V, I_S, N ) + (g_tiny * V);
		if (conductance)
			result.conductance = diodeConductance( V, I_S, N ) + g_tiny;
	}
	else if ( V_B == 0 || V >= -V_B ) {
		auto a = (3.0 * N * V_T) / (V * M_E);
		a = std::pow(a, 3.0);
		result.current = (-I_S * (1.0 + a)) + (g_tiny * V);
		if (conductance)
			result.conductance = ((I_S * 3.0 * a) / V) + g_tiny;
	}
	else {
		auto a = std::exp( -(V_B + V) / N / V_T );
		result.current = (-I_S * a) + (g_tiny * V);
		if (conductance)
			result.conductance = I_S * a / V_T / N + g_tiny;
	}

	return result;
}

void Diode::setDiodeSettings(const DiodeSettings &settings) {
	m_diodeSettings = settings;
	updateLim();
	if (p_eSet) {
		p_eSet->setCacheInvalidated();
	}
}

void Diode::updateLim() {
	auto I_S = m_diodeSettings.I_S;
	auto N = m_diodeSettings.N;
	V_lim = diodeLimitedVoltage( I_S, N );
}
//END class Diode
