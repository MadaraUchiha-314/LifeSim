#ifndef __RMU__NBTI_H
#define __RMU__NBTI_H

#include "RMU.h"

class Constants_NBTI
{
	public:

		static constexpr double T_base = 345.0;	/* Qualification temperature. At this value, the total processor FIT value will be 4000. By changing this temperature, we can model processors with different reliability qualification costs (Higher implies more expensive).*/ 
		static constexpr double TOTAL_NBTI_FITS = 800.0;
		static constexpr double NBTI_beta = 0.3;
};

class CoreInfoNBTI : public CoreInfo
{	
	double NBTI_fits;
	double NBTI_base_fits;
	double NBTI_inst;

	public:
		CoreInfoNBTI();
		~CoreInfoNBTI();
		void updateMTTF();
};




#endif