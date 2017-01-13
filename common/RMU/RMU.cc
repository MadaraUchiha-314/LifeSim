#include "RMU.h"
#include "simulator.h"
#include "config.hpp"
#include "RMU_EM.h"
#include "RMU_TDDB.h"
#include "RMU_TC.h"
#include "RMU_NBTI.h"


#include <cmath>
#include <iostream>

CoreInfo::CoreInfo()
	: pv_param(0.0)
	, frequency((double) Sim()->getCfg()->getFloat("perf_model/core/frequency")) // From Config File
	, V((double) Sim()->getCfg()->getFloat("power/vdd")) // From Config File
	, T(0.0)
	, beta_factor(0.0)
	, MTTF(0.0)
	, isActive(0)
	, hyper_period_number(0)
{ 
}
CoreInfo::~CoreInfo()
{
}

RMU::RMU()
{

	String failure_mehcanism = Sim()->getCfg()->getString("general/failure_mechanism");
	int num_cores = Sim()->getConfig()->getApplicationCores();

	if (failure_mehcanism == "electromigration")
	{
		for (int i=0;i<num_cores;i++)
			coreList.push_back(new CoreInfoEM());
	}
	else if (failure_mehcanism == "tddb")
	{
		for (int i=0;i<num_cores;i++)
			coreList.push_back(new CoreInfoTDDB());
	}
	else if (failure_mehcanism == "tc")
	{
		for (int i=0;i<num_cores;i++)
			coreList.push_back(new CoreInfoTC());
	}
	else if (failure_mehcanism == "nbti")
	{
		for (int i=0;i<num_cores;i++)
			coreList.push_back(new CoreInfoNBTI());
	}

}
RMU::~RMU()
{
}