#ifndef __RMU_TC_H
#define __RMU_TC_H

#include "RMU.h"

class Constants_TC
{
	public:

		static constexpr double T_base = 345.0;	/* Qualification temperature. At this value, the total processor FIT value will be 4000. By changing this temperature, we can model processors with different reliability qualification costs (Higher implies more expensive).*/ 
		static constexpr double ambient = 273.0 + 45.0;
		static constexpr double TC_exponent = 2.35;
		static constexpr double TOTAL_TC_FITS = 800.0;
};

class CoreInfoTC : public CoreInfo
{	
	double TC_base_fits;
	double TC_fits;
	double TC_inst;
	double mean;

	public:
		CoreInfoTC();
		~CoreInfoTC();
		void updateMTTF();
};



#endif