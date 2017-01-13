#ifndef __RMU_H
#define __RMU_H

#include <vector>

#define DEBUG_PRINT

/*
* This the Reliability Management Unit
* This maintains the data-structures required for calculating the Life-time Reliablity of the many-core-system
*/

class CoreInfo
{
	protected:
		double pv_param; // The only external input. The other parameters are calculated using this.
		double frequency;
		double V; 
		double T; 
		double beta_factor;
		double MTTF;
		int isActive;
		int hyper_period_number;

	public:
		CoreInfo();

		/*
		* All the function have been implemented as virtual fucntions, so that a custom Reliability Model like RMU_EM or RMU_NBTI can use this
		* All a custom Reliability Model has to implement is updateMTTF() and updateAging() functions to be integrated with the tool.
		* A custom model can choose to override other functions to enable custom actions.
		*/

		virtual ~CoreInfo(); // Virtual Destructor To The Rescue :D :)
		virtual void setPVParam(double process_variation_parameter) { pv_param = process_variation_parameter; }
		virtual double getMTTF() { return MTTF; }
		virtual void updateMTTF() { }
		virtual void updateAging() { }
		virtual void setTemperature(double temp) { T = temp; }
		virtual void setBetaFactor(double factor) { beta_factor = factor; }
		virtual void setActive(int flag) { isActive = flag; }
		virtual void setHyperPeriodNumber(int h_num) {  hyper_period_number = h_num; }
};

class RMU
{
	public:
		std::vector<CoreInfo*> coreList;

	public:
		RMU();
		~RMU();
};

#endif