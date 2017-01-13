#include "RMU_TDDB.h"

#include "simulator.h"
#include "config.hpp"

#include <cmath>

CoreInfoTDDB::CoreInfoTDDB()
 : CoreInfo()
 , TDDB_fits(0.0)
 , TDDB_base_fits(0.0)
 , TDDB_inst(0.0)

{
	int num_cores = Sim()->getConfig()->getApplicationCores();

	/*
	*  We are dividing the TOTAL_TDDB_FITS by the number of cores as it was previously (unit_area / total_area), which is the same as the number of cores in our case
	*/

	TDDB_base_fits = (Constants_TDDB::TOTAL_TDDB_FITS)/(double)num_cores;

	/* Comment from RAMP 2.0 */
	/* Initial FIT values are same as base FIT values */

	TDDB_fits = (Constants_TDDB::TOTAL_TDDB_FITS)/(double)num_cores;

}

CoreInfoTDDB::~CoreInfoTDDB()
{	
}

void CoreInfoTDDB::setPVParam(double process_variation_parameter)
{
	pv_param = process_variation_parameter;
	frequency = pv_param * frequency;
	V = pv_param * V;
}

/*
* The reliability code is taken from RAMP 2.0.
*/

void CoreInfoTDDB::updateMTTF()
{
	double TBDratio; 						/* Non temperature term ratio*/
	double base_exp;						/* Base temperature reliability factor*/
	double new_exp;							/* New reliability factor*/
	double fits_ratio;

	base_exp = pow(2.718,((0.759/((8.62e-5)*Constants_TDDB::T_base))-(66.8/((8.62e-5)*pow(Constants_TDDB::T_base,2.0)))-9.7099));
	new_exp = pow(2.718,((0.759/((8.62e-5)*(T)))-(66.8/((8.62e-5)*pow((T),2.0)))-9.7099));

	TBDratio = (pow((((1.10*(pow(frequency,0.42206))))/(1.10)),(78 - 0.081*(T))))*(2.0*0.50)*(pow(10.0,((16.0 - 16.0 )/2.0)));  

    fits_ratio = TBDratio*(base_exp/new_exp);

	if (hyper_period_number>1)
	{     	
        TDDB_fits = (TDDB_fits*(hyper_period_number-1) + fits_ratio*TDDB_base_fits)/hyper_period_number;
    } 
	
	TDDB_inst = fits_ratio*TDDB_base_fits; // We have got the FITS, now to calculate the MTTF


	/* 
	* Comment from RAMP 2.0
	* The MTTF value of each structure due to each failure mechanism depends on its
 	* FIT value. 1 FIT represents one failure in 10E9 operating hours. Therefore, a
 	* FIT value of 3805.17503 gives an MTTF of 30 years. 
 	*/


 	MTTF = (30.0*3805.17503)/TDDB_inst;
}