#include "RMU_NBTI.h"

#include "simulator.h"
#include "config.hpp"

#include <cmath>

CoreInfoNBTI::CoreInfoNBTI()
 : CoreInfo()
 , NBTI_fits(0.0)
 , NBTI_base_fits(0.0)
 , NBTI_inst(0.0)
{
	int num_cores = Sim()->getConfig()->getApplicationCores();

	NBTI_base_fits = Constants_NBTI::TOTAL_NBTI_FITS/(double)num_cores;
	NBTI_fits = Constants_NBTI::TOTAL_NBTI_FITS/(double)num_cores;
}

CoreInfoNBTI::~CoreInfoNBTI()
{
}

void CoreInfoNBTI::updateMTTF()
{
	// double temp_ratio; 	/* Ratio of temperature diffs*/
    // double temp_diff; 	 Temp diff 
    // double rel_exp;
    double fits_ratio;

	double NBTI_basemttf;
	double NBTI_currentmttf;
        
	// temp_ratio = Constants_NBTI::T_base/T;
	// temp_diff = (1.0/Constants_NBTI::T_base) - (1.0/T);

    // rel_exp = 795.2*temp_diff; // Don't really know why this line is there xD

	NBTI_basemttf = pow(((log(1.6368/(1.0+2.0*pow(2.7182818,(856.1067/Constants_NBTI::T_base)))) - log((1.6368/(1.0+2.0*pow(2.7182818,(856.1067/Constants_NBTI::T_base))))-0.01))*(Constants_NBTI::T_base/(pow(2.7182818,(-795.2/Constants_NBTI::T_base))))),(1.0/Constants_NBTI::NBTI_beta));
			
	NBTI_currentmttf = pow(((log(1.6368/(1.0+2.0*pow(2.7182818,(856.1067/T)))) - log((1.6368/(1.0+2.0*pow(2.7182818,(856.1067/T))))-0.01))*(T/(pow(2.7182818,(-795.2/T))))),(1.0/Constants_NBTI::NBTI_beta));

	fits_ratio = NBTI_basemttf/NBTI_currentmttf;

    if (hyper_period_number>1)
    {
        NBTI_fits = (NBTI_fits*(hyper_period_number-1) + fits_ratio*NBTI_base_fits)/hyper_period_number;
    }

	NBTI_inst = fits_ratio*NBTI_base_fits;

}