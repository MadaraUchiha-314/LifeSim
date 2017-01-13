#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>

#include "paths.h"

#define freq_1 1000.0
#define freq_2 900.0
#define freq_3 800.0
#define freq_4 700.0

using namespace std;
/*
* This is an example of how DVFS can be implemented inside Minerva
* Example shown here : If the temperature goes up, reduce the frequency by one level which is 10% less than the current frequency of the core
*/
int main ()
{
	string s;
	double T;

	// Keeping the temperature cap as 90 celcius
	double temperature_cap = 273.15 + 90.0;
	long long freq_in_mhz[NUM_CORES];

	ifstream dvfs_file_in;
	dvfs_file_in.open (PATH_TO_DVFS_FILE);

	for (int i=0;i<NUM_CORES;i++)
		dvfs_file_in>>freq_in_mhz[i];

	dvfs_file_in.close();

	ifstream temperature_file;
	temperature_file.open (PATH_TO_TTRACE);

	for (int i=0;i<NUM_CORES;i++)
	{
		temperature_file>>s;
		cout<<s<<" ";
		temperature_file>>T;
		cout<<T<<"\n";
		if (T > temperature_cap)
		{
			if (freq_in_mhz[i] == freq_1)
				freq_in_mhz[i] = freq_2;
			else if (freq_in_mhz[i] == freq_2)
				freq_in_mhz[i] = freq_3;
			else
				freq_in_mhz[i] = freq_4;
		}
	}
	ofstream dvfs_file_out;
	dvfs_file_out.open (PATH_TO_DVFS_FILE);

	for (int i=0;i<NUM_CORES;i++)
		dvfs_file_out<<freq_in_mhz[i]<<" ";
	dvfs_file_out.close();
	return 0;
}