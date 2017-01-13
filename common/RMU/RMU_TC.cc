#include "RMU_TC.h"

#include "simulator.h"
#include "config.hpp"

#include <cmath>

CoreInfoTC::CoreInfoTC()
 : CoreInfo()
 , TC_base_fits(0.0)
 , TC_fits(0.0)
 , TC_inst(0.0)
 , mean(0.0)
{

	int num_cores = Sim()->getConfig()->getApplicationCores();

	TC_base_fits = Constants_TC::TOTAL_TC_FITS/double(num_cores);
	TC_fits = Constants_TC::TOTAL_TC_FITS/double(num_cores);
}

CoreInfoTC::~CoreInfoTC()
{
}

void CoreInfoTC::updateMTTF()
{
	if (hyper_period_number>1)
	{     	
        mean = (mean*(hyper_period_number-1) + T)/(double)hyper_period_number;
    } 

	double TC_base_temp_diff = Constants_TC::T_base - Constants_TC::ambient;

	TC_fits = pow(((mean-Constants_TC::ambient)/(TC_base_temp_diff)),Constants_TC::TC_exponent)*TC_base_fits;

	TC_inst = TC_fits;

	MTTF = (30.0*3805.17503)/TC_inst;
}