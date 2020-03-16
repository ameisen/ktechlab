#include "matrix.h"
#include "nonlinear.h"

#include <cmath>
#include <algorithm>

static constexpr const double KTL_MAX_DOUBLE = 1.7976931348623157e+308;///< 7fefffff ffffffff
static const int KTL_MAX_EXPONENT = int( std::log(KTL_MAX_DOUBLE) );

double NonLinear::diodeCurrent( double v, double I_S, double N ) const {
	return I_S * (std::exp( std::min( v / (N * V_T), double(KTL_MAX_EXPONENT) ) ) - 1);
}

double NonLinear::diodeConductance( double v, double I_S, double N ) const {
	const double Vt = V_T * N;
	return I_S * std::exp( std::min( v / Vt, double(KTL_MAX_EXPONENT) ) ) / Vt;
}

double NonLinear::diodeVoltage( double V, double V_prev, double N, double V_lim ) const {
	const double Vt = V_T * N;

	if ( V > V_lim && std::abs( V - V_prev ) > 2 * Vt ) {
		if ( V_prev > 0 ) {
			double arg = (V - V_prev) / Vt;
			arg = (arg > 0) ? (arg - 2.0) : (2.0 - arg);
			V = V_prev + Vt * (2 + std::log(arg));
		}
		else
			V = (V_prev < 0) ?
				(Vt * std::log (V / Vt)) :
				V_lim;
	}
	else {
		if ( V < 0 ) {
			double arg = (V_prev > 0) ? (-1 - V_prev) : (2 * V_prev - 1);
			V = std::max(V, arg);
		}
	}
	return V;
}

double NonLinear::diodeLimitedVoltage( double I_S, double N ) const {
	double Vt = N * V_T;

	return Vt * std::log( Vt / M_SQRT2 / I_S );
}

void NonLinear::diodeJunction( double V, double I_S, double N, double &I, double &g ) const {
	double Vt = N * V_T;

	if (V < -3 * Vt) {
		double a = 3 * Vt / (V * M_E);
		a = a * a * a;
		I = -I_S * (1 + a);
		g = +I_S * 3 * a / V;
	}
	else {
		double e = std::exp( std::min( V / Vt, double(KTL_MAX_EXPONENT) ) );
		I = I_S * (e - 1);
		g = I_S * e / Vt;
	}
}

double NonLinear::fetVoltage( double V, double V_prev, double Vth ) const {
	double V_tst_hi = std::abs( 2 * (V_prev - Vth) ) + 2.0;
	double V_tst_lo = V_tst_hi / 2;
	double V_tox = Vth + 3.5;
	double delta_V = V - V_prev;

	if ( V_prev >= Vth ) {
		// on
		if ( V_prev >= V_tox ) {
			if ( delta_V <= 0 ) {
				// going off
				if ( V >= V_tox ) {
					if ( -delta_V > V_tst_lo )
						return V_prev - V_tst_lo;

					return V;
				}

				return std::max( V, Vth + 2 );
			}

			// staying on
			if ( delta_V >= V_tst_hi )
				return V_prev + V_tst_hi;

			return V;
		}

		// middle region
		if ( delta_V <= 0 ) {
			// decreasing
			return std::max( V, Vth - 0.5 );
		}

		// increasing
		return std::min( V, Vth + 4 );
	}

	//  off
	if ( delta_V <= 0 ) {
		// staying off
		if ( -delta_V > V_tst_hi )
			return V_prev - V_tst_hi;

		return V;
	}

	// going on
	if ( V <= Vth + 0.5 ) {
		if ( delta_V > V_tst_lo )
			return V_prev + V_tst_lo;

		return V;
	}

	return Vth + 0.5;
}

double NonLinear::fetVoltageDS( double V, double V_prev ) const {
	if ( V_prev >= 3.5 ) {
		if ( V > V_prev )
			return std::min( V, 3.0 * V_prev + 2.0 );
		else if (V < 3.5)
			return std::max( V, 2.0 );

		return V;
	}

	if ( V > V_prev )
		return std::min( V, 4.0 );

	return std::max( V, -0.5 );
}

void NonLinear::mosDiodeJunction( double V, double I_S, double N, double &I, double &g ) const {
	double Vt = N * V_T;

	double _g;
	double _I;

	if ( V <= 0 ) {
		_g = I_S / Vt;
		_I = _g * V;
	}
	else {
		double e = std::exp( std::min ( V / Vt, double(KTL_MAX_EXPONENT) ) );
		_I = I_S * (e - 1.0);
		_g = I_S * e / Vt;
	}

	I = _I + (V * I_S);
	g = _g + I_S;
}
