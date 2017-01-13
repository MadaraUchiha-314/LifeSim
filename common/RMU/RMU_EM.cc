#include "RMU_EM.h"
#include "simulator.h"
#include "config.hpp"

#include <cmath>
#include <iostream>

CoreInfoEM::CoreInfoEM()
 :  CoreInfo()
	, core_capacitance(0.0)
	, H(0.0)
	, W(0.0)
	, hTa(0.0)
	, delta_R(0.0)
{
}

CoreInfoEM::~CoreInfoEM()
{
}

// Overriden Function
void CoreInfoEM::setPVParam(double process_variation_parameter)
{
	pv_param = process_variation_parameter;
	frequency = pv_param * frequency;
	core_capacitance = pv_param * Constants_EM::C0;
	H = pv_param * Constants_EM::H0;
	W = pv_param * Constants_EM::W0;
	hTa = pv_param * Constants_EM::hTa0;
}
void CoreInfoEM::updateAging()
{
	double D;
	double current,curden;
	double velocity;
	double R_incr_rate;
	double change_in_resistance;
	double delta_V,delta_freq;
	double length_of_epoch_in_sec = (double) Sim()->getCfg()->getFloat("general/epoch_length");


	D = Constants_EM::D0*exp(-(Constants_EM::EV+Constants_EM::EVD)/Constants_EM::kB/T);
	
	current = 325 * 2 * 3.14 * frequency * core_capacitance * V * beta_factor;
	curden = current/W/H;

	velocity = D*Constants_EM::q*Constants_EM::Z*Constants_EM::resistivity_Cu*curden/Constants_EM::kB/T;
	R_incr_rate = velocity*(Constants_EM::resistivity_Ta/hTa/(2*H+hTa)-Constants_EM::resistivity_Cu/H/W);

	change_in_resistance = R_incr_rate * length_of_epoch_in_sec;
	delta_R += change_in_resistance;

	delta_V  = -(current * change_in_resistance);
	V += delta_V;

	delta_freq = delta_V * (frequency/V);
	frequency += delta_freq;
}

void CoreInfoEM::updateMTTF()
{
	double D;
	double current,curden;
	double velocity;
	double R_incr_rate;
	double R0;
	double t_grow;
	double t_nuc;
	double kappa;
	double stress;


	double max_mttf_years = (double)Sim()->getCfg()->getFloat("general/max_mttf_years");

	if (delta_R != 0.0)
	{
		D = Constants_EM::D0*exp(-(Constants_EM::EV+Constants_EM::EVD)/Constants_EM::kB/T);
		
		if (isActive)
			current = 325 * 2 * 3.14 * frequency * core_capacitance * V * beta_factor;
		else
			current = 0.000001;

		curden = current/W/H;

		velocity = D*Constants_EM::q*Constants_EM::Z*Constants_EM::resistivity_Cu*curden/Constants_EM::kB/T;
		R_incr_rate = velocity*(Constants_EM::resistivity_Ta/hTa/(2*H+hTa)-Constants_EM::resistivity_Cu/H/W);

		R0 = Constants_EM::resistivity_Cu*Constants_EM::L/W/H;

		double failure_criterion = (double)Sim()->getCfg()->getFloat("general/failure_criterion");

		t_grow = ((failure_criterion*R0)/100 - delta_R)/R_incr_rate;

		MTTF = t_grow;

		if (MTTF > (max_mttf_years * pv_param * 60 * 60 * 24 * 365))
			MTTF = (max_mttf_years * pv_param * 60 * 60 * 24 * 365);
	}
	else if (isActive)
	{
		D = Constants_EM::D0*exp(-(Constants_EM::EV+Constants_EM::EVD)/Constants_EM::kB/T);
		kappa = D*Constants_EM::B*Constants_EM::Omega/Constants_EM::kB/T;

		current = 325 * 2 * 3.14 * frequency * core_capacitance * V * beta_factor;
		curden = current/W/H;

		stress = Constants_EM::q*Constants_EM::Z*Constants_EM::resistivity_Cu*curden*Constants_EM::L/2/Constants_EM::Omega+Constants_EM::rstress;

		t_nuc = Constants_EM::L*Constants_EM::L/kappa*exp(-Constants_EM::f*Constants_EM::Omega/Constants_EM::kB/T*stress)*log((stress-Constants_EM::rstress)/(stress-Constants_EM::cstress));

		velocity = D*Constants_EM::q*Constants_EM::Z*Constants_EM::resistivity_Cu*curden/Constants_EM::kB/T;
		R_incr_rate = velocity*(Constants_EM::resistivity_Ta/hTa/(2*H+hTa)-Constants_EM::resistivity_Cu/H/W);

		R0 = Constants_EM::resistivity_Cu*Constants_EM::L/W/H;

		double failure_criterion = (double)Sim()->getCfg()->getFloat("general/failure_criterion");

		t_grow = failure_criterion*R0/100/R_incr_rate;

		MTTF = t_grow + t_nuc;

		if (MTTF > (max_mttf_years * pv_param * 60 * 60 * 24 * 365))
			MTTF = (max_mttf_years * pv_param * 60 * 60 * 24 * 365);
	}
	else
	{
		MTTF = (max_mttf_years * pv_param * 60 * 60 * 24 * 365);
	}
}