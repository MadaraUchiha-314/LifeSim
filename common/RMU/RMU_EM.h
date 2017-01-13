#ifndef __RMU_EM_H
#define __RMU_EM_H

#include "RMU.h"

/*
* These are the constants used in the calculation of reliability using the Electromigration mechanism
*/

class Constants_EM
{
	public:
		// Constatnts used in the intialisation of CoreInfoEM
		static constexpr double C0 = 1.0;
		static constexpr double H0 = 1.2e-7;
		static constexpr double W0 = 2.0e-7;
		static constexpr double hTa0 = 1e-8;

		// Constants used to calculate MTTF
		static constexpr double D0 = 7.56e-5;
		static constexpr double q = 1.6e-19; 
		static constexpr double EV = 0.75*q;
		static constexpr double EVD = 0.65*q;
		static constexpr double kB = 1.38e-23;
		static constexpr double resistivity_Cu = 3e-8;
		static constexpr double resistivity_Ta = 3e-5;
		static constexpr double Z = 10.0;
		static constexpr double L = 5e-5;
		static constexpr double Omega = 1.66e-29;
		static constexpr double B = 1e11;
		static constexpr double rstress = 4e8;
		static constexpr double cstress = 5e8;
		static constexpr double f = 0.9;

};

class CoreInfoEM : public CoreInfo
{
	double core_capacitance;
	double H;
	double W;
	double hTa;
	double delta_R;

	public:
		CoreInfoEM();
		~CoreInfoEM();
		void setPVParam(double process_variation_parameter);
		void updateAging();
		void updateMTTF();
};
#endif