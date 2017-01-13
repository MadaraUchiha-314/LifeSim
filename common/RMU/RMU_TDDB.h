#ifndef __RMU_TDDB_H
#define __RMU_TDDB_H

#include "RMU.h"

/*
* These are the constants used in the calculation of reliability using the TDDB mechanism
*/
class Constants_TDDB
{
	public:

		static constexpr double T_base = 273.0;	/* Qualification temperature. At this value, the total processor FIT value will be 4000. By changing this temperature, we can model processors with different reliability qualification costs (Higher implies more expensive).*/ 
		static constexpr double TOTAL_TDDB_FITS = 800.0;

};

/*
* Class for calculating the TDDB related MTTF
* NOTE : We are not overriding the updateAging() because we are not simulating the aging.
*/

class CoreInfoTDDB : public CoreInfo
{
	double TDDB_fits;
	double TDDB_base_fits;
	double TDDB_inst;

	public:
		CoreInfoTDDB();
		~CoreInfoTDDB();
		void setPVParam(double process_variation_parameter);
		void updateMTTF();
};

#endif