#include "scheduler_pinned_base.h"
#include "simulator.h"
#include "core_manager.h"
#include "performance_model.h"
#include "os_compat.h"
#include "stats.h"
#include "thread.h"
#include "config.hpp"
#include "dvfs_manager.h"


#include <sstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <iterator>
#include <string>

// Pinned scheduler.
// Each thread has is pinned to a specific core (m_thread_affinity).
// Cores are handed out to new threads in round-robin fashion.
// If multiple threads share a core, they are time-shared with a configurable quantum

SchedulerPinnedBase::SchedulerPinnedBase(ThreadManager *thread_manager, SubsecondTime quantum)
   : SchedulerDynamic(thread_manager)
   , m_quantum(quantum)
   , m_last_periodic(SubsecondTime::Zero())
   , m_core_thread_running(Sim()->getConfig()->getApplicationCores(), INVALID_THREAD_ID)
   , m_quantum_left(Sim()->getConfig()->getApplicationCores(), SubsecondTime::Zero())

#ifdef MODIFIED_CODE
   , m_interval(SubsecondTime::FS((uint64_t)Sim()->getCfg()->getInt("general/interval_to_run_hotspot")))
   , m_last_periodic_interval(SubsecondTime::Zero())
#endif
#ifdef PERIODIC_WORKLOAD
   , m_hyper_period_number(0)
   , m_num_threads_exited(0)
   , m_last_hyper_period_time(SubsecondTime::Zero())
#endif
#ifdef USE_RMU
   , hyper_period_start_time(SubsecondTime::Zero())
#endif
{

   #ifdef USE_RMU
      end_times.resize(Sim()->getConfig()->getApplicationCores());
      core_temp.resize(Sim()->getConfig()->getApplicationCores());
   #endif

   // Initialise the Data Structures needed for Periodoic simulation of workloads
   #ifdef PERIODIC_WORKLOAD

      m_num_apps = 1;

      if (Sim()->getCfg()->hasKey("traceinput/num_apps"))
         m_num_apps = Sim()->getCfg()->getInt("traceinput/num_apps");

      threads_of_app.resize (m_num_apps);

   #endif

   #ifdef MODIFIED_CODE

      int num_cores = Sim()->getConfig()->getApplicationCores();

      if (Sim()->getCfg()->hasKey("traceinput/num_apps"))
         m_num_apps = Sim()->getCfg()->getInt("traceinput/num_apps");
      else
         m_num_apps = 1;

      m_energy_core_previous.resize (num_cores);   
      threads_on_core.resize (num_cores);
      current_thread_count.resize (m_num_apps);
      core_pv.resize(num_cores);
      m_ic_core_previous.resize(num_cores);

      String path_to_ptrace = Sim()->getCfg()->getString("general/path_to_ptrace");

      power_out_file.open (path_to_ptrace.c_str()); // The path is in config-paths.h

      for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
      {
         power_out_file<<"C"<<core_id<<" ";
         power_out_file.flush ();

      }
      power_out_file<<"\n";
      power_out_file.flush ();


      String path_to_pv_file = Sim()->getCfg()->getString("general/path_to_pv_file");


      std::ifstream pv_file;
      pv_file.open (path_to_pv_file.c_str());

      for (int i=0;i<num_cores;i++)
         pv_file>>core_pv[i];

      pv_file.close();

   #endif


}

core_id_t SchedulerPinnedBase::findFreeCoreForThread(thread_id_t thread_id)
{
   for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
   {
      if (m_thread_info[thread_id].hasAffinity(core_id) && m_core_thread_running[core_id] == INVALID_THREAD_ID)
      {
         return core_id;
      }
   }

   return INVALID_CORE_ID;
}

core_id_t SchedulerPinnedBase::threadCreate(thread_id_t thread_id)
{
   if (m_thread_info.size() <= (size_t)thread_id)
      m_thread_info.resize(m_thread_info.size() + 16);

   #ifdef PERIODIC_WORKLOAD

      int app_id = Sim()->getThreadManager()->getThreadFromID(thread_id)->getAppId();

      /*
      * Initially When the thread arrives, if it is of the next hyper-period then dont assign any core_id to it.
      * Source : https://groups.google.com/d/msg/snipersim/7Oj7Id3kyCI/CIeodQv9tlIJ
      */

      /*
      * NOTE : The way the benchmarks are written, A parent thread creates N-1 child threads and they all do the same work.
      * Before the stalled thread comes back into the running state, we need to set its affinity and other parameters.
      * The stalled thread typically spawns the other N-1 threads, so we have to set the current_thread_count[app_id] of this thread
      */ 

      int num_app_per_hyper_period = Sim()->getCfg()->getInt("general/num_app_per_hyper_period");

      if (app_id >= (m_hyper_period_number+1) * num_app_per_hyper_period)
      {
         threads_of_app[app_id].push_back(thread_id);
         m_thread_info[thread_id].setCoreRunning (INVALID_CORE_ID);
         current_thread_count[app_id]++;

         #ifdef DEBUG_PRINT
            std::cout<<"\n[DEBUG PRINT] Stalling Thread "<<thread_id<<"\n";
            fflush (stdout);
         #endif

         return INVALID_CORE_ID;
      }

   #endif

   if (m_thread_info[thread_id].hasAffinity())
   {
      // Thread already has an affinity set at/before creation
   }
   else
   {
      threadSetInitialAffinity(thread_id);
   }

   // The first thread scheduled on this core can start immediately, the others have to wait
   core_id_t free_core_id = findFreeCoreForThread(thread_id);
   if (free_core_id != INVALID_CORE_ID)
   {
      m_thread_info[thread_id].setCoreRunning(free_core_id);
      m_core_thread_running[free_core_id] = thread_id;
      m_quantum_left[free_core_id] = m_quantum;
      return free_core_id;
   }
   else
   {
      m_thread_info[thread_id].setCoreRunning(INVALID_CORE_ID);
      return INVALID_CORE_ID;
   }
}

void SchedulerPinnedBase::threadYield(thread_id_t thread_id)
{
   core_id_t core_id = m_thread_info[thread_id].getCoreRunning();

   if (core_id != INVALID_CORE_ID)
   {
      Core *core = Sim()->getCoreManager()->getCoreFromID(core_id);
      SubsecondTime time = core->getPerformanceModel()->getElapsedTime();

      m_quantum_left[core_id] = SubsecondTime::Zero();
      reschedule(time, core_id, false);

      if (!m_thread_info[thread_id].hasAffinity(core_id))
      {
         core_id_t free_core_id = findFreeCoreForThread(thread_id);
         if (free_core_id != INVALID_CORE_ID)
         {
            // We have just been moved to a different core(s), and one of them is free. Schedule us there now.
            reschedule(time, free_core_id, false);
         }
      }
   }
}

bool SchedulerPinnedBase::threadSetAffinity(thread_id_t calling_thread_id, thread_id_t thread_id, size_t cpusetsize, const cpu_set_t *mask)
{
   if (m_thread_info.size() <= (size_t)thread_id)
      m_thread_info.resize(thread_id + 16);

   m_thread_info[thread_id].setExplicitAffinity();

   if (!mask)
   {
      // No mask given: free to schedule anywhere.
      for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
      {
         m_thread_info[thread_id].addAffinity(core_id);
      }
   }
   else
   {
      m_thread_info[thread_id].clearAffinity();

      for(unsigned int cpu = 0; cpu < 8 * cpusetsize; ++cpu)
      {
         if (CPU_ISSET_S(cpu, cpusetsize, mask))
         {
            LOG_ASSERT_ERROR(cpu < Sim()->getConfig()->getApplicationCores(), "Invalid core %d found in sched_setaffinity() mask", cpu);

            m_thread_info[thread_id].addAffinity(cpu);
         }
      }
   }

   // We're setting the affinity of a thread that isn't yet created. Do nothing else for now.
   if (thread_id >= (thread_id_t)Sim()->getThreadManager()->getNumThreads())
      return true;

   if (thread_id == calling_thread_id)
   {
      threadYield(thread_id);
   }
   else if (m_thread_info[thread_id].isRunning()                           // Thread is running
            && !m_thread_info[thread_id].hasAffinity(m_thread_info[thread_id].getCoreRunning())) // but not where we want it to
   {
      // Reschedule the thread as soon as possible
      m_quantum_left[m_thread_info[thread_id].getCoreRunning()] = SubsecondTime::Zero();
   }
   else if (m_threads_runnable[thread_id]                                  // Thread is runnable
            && !m_thread_info[thread_id].isRunning())                      // Thread is not running (we can't preempt it outside of the barrier)
   {
      core_id_t free_core_id = findFreeCoreForThread(thread_id);
      if (free_core_id != INVALID_THREAD_ID)                               // Thread's new core is free
      {
         // We have just been moved to a different core, and that core is free. Schedule us there now.
         Core *core = Sim()->getCoreManager()->getCoreFromID(free_core_id);
         SubsecondTime time = std::max(core->getPerformanceModel()->getElapsedTime(), Sim()->getClockSkewMinimizationServer()->getGlobalTime());
         reschedule(time, free_core_id, false);
      }
   }

   return true;
}

bool SchedulerPinnedBase::threadGetAffinity(thread_id_t thread_id, size_t cpusetsize, cpu_set_t *mask)
{
   if (cpusetsize*8 < Sim()->getConfig()->getApplicationCores())
   {
      // Not enough space to return result
      return false;
   }

   CPU_ZERO_S(cpusetsize, mask);
   for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
   {
      if (
         m_thread_info[thread_id].hasAffinity(core_id)
         // When application has not yet done any sched_setaffinity calls, lie and return a fully populated affinity bitset.
         // This makes libiomp5 use all available cores.
         || !m_thread_info[thread_id].hasExplicitAffinity()
      )
         CPU_SET_S(core_id, cpusetsize, mask);
   }

   return true;
}

void SchedulerPinnedBase::threadStart(thread_id_t thread_id, SubsecondTime time)
{
   // Thread transitioned out of INITIALIZING, if it did not get a core assigned by threadCreate but there is a free one now, schedule it there
   core_id_t free_core_id = findFreeCoreForThread(thread_id);
   if (free_core_id != INVALID_THREAD_ID)
      reschedule(time, free_core_id, false);
}

void SchedulerPinnedBase::threadStall(thread_id_t thread_id, ThreadManager::stall_type_t reason, SubsecondTime time)
{
   // If the running thread becomes unrunnable, schedule someone else
   if (m_thread_info[thread_id].isRunning())
      reschedule(time, m_thread_info[thread_id].getCoreRunning(), false);
}

void SchedulerPinnedBase::threadResume(thread_id_t thread_id, thread_id_t thread_by, SubsecondTime time)
{
   // If our core is currently idle, schedule us now
   core_id_t free_core_id = findFreeCoreForThread(thread_id);
   if (free_core_id != INVALID_THREAD_ID)
      reschedule(time, free_core_id, false);
}

void SchedulerPinnedBase::threadExit(thread_id_t thread_id, SubsecondTime time)
{
   // If the running thread becomes unrunnable, schedule someone else
   if (m_thread_info[thread_id].isRunning())
      reschedule(time, m_thread_info[thread_id].getCoreRunning(), false);

   #ifdef USE_RMU
      end_times[m_thread_info[thread_id].getCoreRunning()] = time;
   #endif

   #ifdef PERIODIC_WORKLOAD
      m_num_threads_exited++;
   #endif

   #ifdef DEBUG_PRINT
      std::cout<<"\n[DEBUG PRINT] Thread exiting is "<<thread_id<<"\n";
      fflush(stdout);
   #endif

   #ifdef PERIODIC_WORKLOAD

      // Now its time to add the threads who have been waiting \m/
      // Condition is that the number of threads exited should be equal to number of threads in the hyper-period.

      int num_app_per_hyper_period = Sim()->getCfg()->getInt("general/num_app_per_hyper_period");
      int max_hyper_periods = m_num_apps / num_app_per_hyper_period;
      int num_threads_per_hyper_period = Sim()->getCfg()->getInt("general/num_threads_per_hyper_period");

      if ((m_num_threads_exited == num_threads_per_hyper_period) && (m_hyper_period_number < (max_hyper_periods-1)))
      {
         #ifdef DEBUG_PRINT
            std::cout<<"\n[DEBUG PRINT] Going to next hyper-period. Number of threads exited = "<<SchedulerDynamic::num_apps_exited<<"\n";
            fflush (stdout);
         #endif


         // 1. Generate LOG for temeperature using Hotspot
         // 2. Call Algorithm
         // 3. Read from file the new mapping

         power_out_file.close ();

         String hotspot_command = Sim()->getCfg()->getString("general/hotspot_command");

         int ret_val = system (hotspot_command.c_str());
         ret_val++;

         // Set the values needed for the RMU

         #ifdef USE_RMU

            /*
            * Steps :
            * 1. Parse the ttrace file generated by HotSpot
            * 2. Populate PV Parameter,temperature,isActive and beta_factor for each core
            * 3. Calculate the MTTF and update the aging
            */

            std::ifstream temp_file;
            std::ofstream mttf_file;
            String path_to_ttrace = Sim()->getCfg()->getString("general/path_to_ttrace");
            String path_to_mttf_file = Sim()->getCfg()->getString("general/path_to_mttf_file");
            temp_file.open (path_to_ttrace.c_str());
            mttf_file.open (path_to_mttf_file.c_str());

            double mttf_core;
            double temp;
            double pv;

            for(int core_id = 0; core_id < (int)Sim()->getConfig()->getApplicationCores(); core_id++)
            {
               std::string core_name;
               temp_file>>core_name>>temp;

               #ifdef DEBUG_PRINT
                  std::cout<<"\n[DEBUG_PRINT] Temperature of core "<<core_id<<" is "<<temp<<"\n";
                  std::cout<<"\n[DEBUG_PRINT] PV param of core "<<core_id<<" is "<<core_pv[core_id]<<"\n";
                  fflush (stdout);
               #endif

               core_temp[core_id] = temp;
               pv = core_pv[core_id];

               my_rmu.coreList[core_id]->setHyperPeriodNumber(m_hyper_period_number);
               my_rmu.coreList[core_id]->setPVParam(pv);
               my_rmu.coreList[core_id]->setTemperature(temp);

               if (threads_on_core[core_id].size() != 0)
                  my_rmu.coreList[core_id]->setActive(1);
               else
                  my_rmu.coreList[core_id]->setActive(0);

               // my_rmu.coreList[core_id]->setBetaFactor((double)(end_times[core_id].getInternalDataForced()/(time.getInternalDataForced()-hyper_period_start_time.getInternalDataForced())));
               my_rmu.coreList[core_id]->setBetaFactor (1.0);
               my_rmu.coreList[core_id]->updateMTTF();
               mttf_core = my_rmu.coreList[core_id]->getMTTF();

               #ifdef DEBUG_PRINT
                  std::cout<<"\n[DEBUG PRINT] MTTF of Core "<<core_id<<" is "<<mttf_core<<"\n";
               #endif

               // Write the MTTf to file
               mttf_file<<mttf_core<<" ";

               my_rmu.coreList[core_id]->updateAging();
            }

            mttf_file.close();
            temp_file.close();

         #endif


         // At this point we have 3 files. Temperature, Power and MTTF and we need to save them (Help! Help!) form plotting the graphs.

         std::ofstream copy_file_out;
         std::ifstream copy_file_in;

         // First Power File
         String suffix_num (((std::string("_") +  std::to_string(m_hyper_period_number)).c_str()));

         String power_folder = Sim()->getCfg()->getString("general/path_to_extra_folder");
         String power_folder_string ("/power/power_file");

         power_folder += power_folder_string;
         power_folder += suffix_num;

         copy_file_out.open (power_folder.c_str(),std::ios::binary);
         copy_file_in.open (Sim()->getCfg()->getString("general/path_to_ptrace").c_str(),std::ios::binary);

         copy_file_out<<copy_file_in.rdbuf();

         copy_file_out.close();
         copy_file_in.close();


         // Temperature File
         String temp_folder = Sim()->getCfg()->getString("general/path_to_extra_folder");
         String temp_folder_string ("/temp/temp_file");

         temp_folder += temp_folder_string;
         temp_folder += suffix_num;

         copy_file_out.open (temp_folder.c_str(),std::ios::binary);
         copy_file_in.open (Sim()->getCfg()->getString("general/path_to_ttrace").c_str(),std::ios::binary);

         copy_file_out<<copy_file_in.rdbuf();

         copy_file_out.close();
         copy_file_in.close();

         // MTTF File

         #ifdef USE_RMU

            String mttf_folder = Sim()->getCfg()->getString("general/path_to_extra_folder");
            String mttf_folder_string ("/mttf/mttf_file");

            mttf_folder += mttf_folder_string;
            mttf_folder += suffix_num;

            copy_file_out.open (mttf_folder.c_str(),std::ios::binary);
            copy_file_in.open (Sim()->getCfg()->getString("general/path_to_mttf_file").c_str(),std::ios::binary);

            copy_file_out<<copy_file_in.rdbuf();

            copy_file_out.close();
            copy_file_in.close();

         #endif

         // Reset it
         SchedulerDynamic :: num_apps_exited = 0;
         m_num_threads_exited = 0;
         m_hyper_period_number++;

         #ifdef USE_RMU
            hyper_period_start_time = time;
         #endif

         // Write the current mapping to the file
         print_core_thread_map();

         // Print Current time and epoch-number
         std::ofstream time_file;
         String path_to_time_file = Sim()->getCfg()->getString("general/path_to_time_file");

         time_file.open (path_to_time_file.c_str());

         time_file<<time<<"\t"<<time-m_last_hyper_period_time<<"\n";
         time_file.flush();

         time_file.close();

         m_last_hyper_period_time = time;

         String algorithm_command = Sim()->getCfg()->getString("general/algorithm_command");

         ret_val = system (algorithm_command.c_str());
         ret_val++;

         // Remake the power-trace file
         String path_to_ptrace = Sim()->getCfg()->getString("general/path_to_ptrace");
         power_out_file.open (path_to_ptrace.c_str());
         
         for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
         {
            power_out_file<<"C"<<core_id<<" ";
            power_out_file.flush ();

         }
         power_out_file<<"\n";
         power_out_file.flush ();

         int num_app_per_hyper_period = Sim()->getCfg()->getInt("general/num_app_per_hyper_period");
         int app_id_offset = m_hyper_period_number * num_app_per_hyper_period;

         // NOTE : We have to maintain the exact same data structures as done by MODIFIED_CODE so that both options can be used at the same time.
         std::ifstream app_core_mapping;
         String new_path_to_input_to_thermal_simulation = Sim()->getCfg()->getString("general/new_path_to_input_to_thermal_simulation");
         app_core_mapping.open (new_path_to_input_to_thermal_simulation.c_str());

         int core_id,app_id,thread_id;

         while (app_core_mapping >> core_id >> app_id >>  thread_id)
         {
            #ifdef DEBUG_PRINT
               std::cout<<"\n[DEBUG PRINT] Mapping from file "<<core_id<<" "<<app_id<<" "<<thread_id<<"\n";
               fflush (stdout);
            #endif

            // Exact same code as in scheduler_pinned.cc : SchedulerPinned::SchedulerPinned(), except for app_id+app_id_offset
            // We are inserting the entire mapping in to the map. So that when the child threads are created, they will directly be called from SchedulerPinned::threadSetInitialAffinity
            core_thread_map.insert (std::make_pair(std::make_pair(app_id+app_id_offset,thread_id),core_id));
         }

         app_core_mapping.close();

         // Maintain the data structures for the MODIFIED flag also.
         #ifdef MODIFIED_CODE

            for (int i=0;i<(int)threads_on_core.size();i++)
               threads_on_core[i].clear();

         #endif

         for (int i=app_id_offset;i<app_id_offset+num_app_per_hyper_period;i++)
         {
            #ifdef DEBUG_PRINT
               std::cout<<"\n[DEBUG PRINT] Re-Scheduling App Id = "<<i<<"\n";
            #endif

            for (int thread_count=0;thread_count<(int)threads_of_app[i].size();thread_count++)
            {
               // See which core to place this
               int core_id_to_place = core_thread_map[std::make_pair(i,thread_count)];

               thread_id = threads_of_app[i][thread_count];

               // Place the thread there.
               m_thread_info[thread_id].setExplicitAffinity();
               m_thread_info[thread_id].clearAffinity();
               m_thread_info[thread_id].addAffinity (core_id_to_place);
               m_core_thread_running[core_id_to_place] = thread_id;
               m_quantum_left[core_id_to_place] = m_quantum;
               m_thread_info[thread_id].setCoreRunning(core_id_to_place);

               m_thread_info[thread_id].setLastScheduledIn(time);

               #ifdef DEBUG_PRINT
                  std::cout<<"\n[DEBUG PRINT] Moving thread : "<<thread_id<<" to core "<<core_id_to_place<<"\n";
               #endif

               moveThread (thread_id,core_id_to_place,time);

               // Providing compactiblity for MODIFIED_CODE
               #ifdef MODIFIED_CODE
                  threads_on_core[core_id_to_place].push_back(thread_id);
                  mw_rank_of_thread.insert (std::make_pair(thread_id,thread_count));
               #endif
            }
         }
      }
      // This is for the last set of threads in the epoch. We just have to store the values and do teh calculation. No scheduling and re-mapping is required.
      else if ((m_num_threads_exited == num_threads_per_hyper_period) && (m_hyper_period_number == (max_hyper_periods-1)))
      {
         #ifdef DEBUG_PRINT
            std::cout<<"\n[DEBUG PRINT] Going to next hyper-period. Number of threads exited = "<<SchedulerDynamic::num_apps_exited<<"\n";
            fflush (stdout);
         #endif


         // 1. Generate LOG for temeperature using Hotspot
         // 2. Call Algorithm
         // 3. Read from file the new mapping

         power_out_file.close ();

         String hotspot_command = Sim()->getCfg()->getString("general/hotspot_command");

         int ret_val = system (hotspot_command.c_str());
         ret_val++;

         // Set the values needed for the RMU

         #ifdef USE_RMU

            /*
            * Steps :
            * 1. Parse the ttrace file generated by HotSpot
            * 2. Populate PV Parameter,temperature,isActive and beta_factor for each core
            * 3. Calculate the MTTF and update the aging
            */

            std::ifstream temp_file;
            std::ofstream mttf_file;
            String path_to_ttrace = Sim()->getCfg()->getString("general/path_to_ttrace");
            String path_to_mttf_file = Sim()->getCfg()->getString("general/path_to_mttf_file");
            temp_file.open (path_to_ttrace.c_str());
            mttf_file.open (path_to_mttf_file.c_str());

            double mttf_core;
            double temp;
            double pv;

            for(int core_id = 0; core_id < (int)Sim()->getConfig()->getApplicationCores(); ++core_id)
            {
               std::string core_name;
               temp_file>>core_name>>temp;

               #ifdef DEBUG_PRINT
                  std::cout<<"\n[DEBUG_PRINT] Temperature of core "<<core_id<<" is "<<temp<<"\n";
                  std::cout<<"\n[DEBUG_PRINT] PV param of core "<<core_id<<" is "<<core_pv[core_id]<<"\n";
                  fflush (stdout);
               #endif

               core_temp[core_id] = temp;
               pv = core_pv[core_id];

               my_rmu.coreList[core_id]->setPVParam(pv);
               my_rmu.coreList[core_id]->setTemperature(temp);

               if (threads_on_core[core_id].size() != 0)
                  my_rmu.coreList[core_id]->setActive(1);
               else
                  my_rmu.coreList[core_id]->setActive(0);

               // my_rmu.coreList[core_id]->setBetaFactor((double)(end_times[core_id].getInternalDataForced()/(time.getInternalDataForced()-hyper_period_start_time.getInternalDataForced())));
               my_rmu.coreList[core_id]->setBetaFactor (1.0);
               my_rmu.coreList[core_id]->updateMTTF();
               mttf_core = my_rmu.coreList[core_id]->getMTTF();

               #ifdef DEBUG_PRINT
                  std::cout<<"\n[DEBUG PRINT] MTTF of Core "<<core_id<<" is "<<mttf_core<<"\n";
               #endif

               // Write the MTTf to file
               mttf_file<<mttf_core<<" ";

               my_rmu.coreList[core_id]->updateAging();
            }

            mttf_file.close();
            temp_file.close();

         #endif


         // At this point we have 3 files. Temperature, Power and MTTF and we need to save them (Help! Help!) form plotting the graphs.

         std::ofstream copy_file_out;
         std::ifstream copy_file_in;

         // First Power File
         String suffix_num (((std::string("_") +  std::to_string(m_hyper_period_number)).c_str()));

         String power_folder = Sim()->getCfg()->getString("general/path_to_extra_folder");
         String power_folder_string ("/power/power_file");

         power_folder += power_folder_string;
         power_folder += suffix_num;

         copy_file_out.open (power_folder.c_str(),std::ios::binary);
         copy_file_in.open (Sim()->getCfg()->getString("general/path_to_ptrace").c_str(),std::ios::binary);

         copy_file_out<<copy_file_in.rdbuf();

         copy_file_out.close();
         copy_file_in.close();


         // Temperature File
         String temp_folder = Sim()->getCfg()->getString("general/path_to_extra_folder");
         String temp_folder_string ("/temp/temp_file");

         temp_folder += temp_folder_string;
         temp_folder += suffix_num;

         copy_file_out.open (temp_folder.c_str(),std::ios::binary);
         copy_file_in.open (Sim()->getCfg()->getString("general/path_to_ttrace").c_str(),std::ios::binary);

         copy_file_out<<copy_file_in.rdbuf();

         copy_file_out.close();
         copy_file_in.close();

         // MTTF File

         #ifdef USE_RMU

            String mttf_folder = Sim()->getCfg()->getString("general/path_to_extra_folder");
            String mttf_folder_string ("/mttf/mttf_file");

            mttf_folder += mttf_folder_string;
            mttf_folder += suffix_num;

            copy_file_out.open (mttf_folder.c_str(),std::ios::binary);
            copy_file_in.open (Sim()->getCfg()->getString("general/path_to_mttf_file").c_str(),std::ios::binary);

            copy_file_out<<copy_file_in.rdbuf();

            copy_file_out.close();
            copy_file_in.close();

         #endif
      }

   #endif
}

#ifdef MODIFIED_CODE

   void SchedulerPinnedBase::changeMapping()
   {
      /*
      * 1. Open the new mapping file
      * 2. Clear the afffinity of the thread
      * 3. Set the affinity of the new thread
      * 4. Change the quantum to zero
      * 5. Store the new mapping
      */ 

      int num_cores = Sim()->getConfig()->getApplicationCores();
      std::vector<std::vector<int> > temp_threads_on_core (num_cores);

      std::ifstream mapping_file;
      String path_to_mapping_file = Sim()->getCfg()->getString("general/path_to_mapping_file");
      mapping_file.open (path_to_mapping_file.c_str());

      int is_mapping_changed;
      int from,to;

      mapping_file >> is_mapping_changed;

      if (is_mapping_changed)
      {
         #ifdef DEBUG_PRINT
            std::cout<<"\n[DEBUG PRINT] Changing the Mapping\n";
         #endif

         while (mapping_file >> from >> to)
         {
            #ifdef DEBUG_PRINT
               std::cout<<"\n[DEBUG PRINT] Migrating threads From "<<from<<" to "<<to<<"\n";
            #endif

            for (std::vector<int>::iterator it=threads_on_core[from].begin();it!=threads_on_core[from].end();it++)
            {
               thread_id_t thread_id = *it;
               
               temp_threads_on_core[to].push_back (thread_id);

               m_thread_info[thread_id].setExplicitAffinity();
               m_thread_info[thread_id].clearAffinity();
               m_thread_info[thread_id].addAffinity(to);


               if (thread_id >= (thread_id_t)Sim()->getThreadManager()->getNumThreads())
                  continue;
               else if (m_thread_info[thread_id].isRunning()                           // Thread is running
                        && !m_thread_info[thread_id].hasAffinity(m_thread_info[thread_id].getCoreRunning())) // but not where we want it to
               {
                  // Reschedule the thread as soon as possible
                  m_quantum_left[m_thread_info[thread_id].getCoreRunning()] = SubsecondTime::Zero();
               }
               else if (m_threads_runnable[thread_id]                                  // Thread is runnable
                       && !m_thread_info[thread_id].isRunning())                      // Thread is not running (we can't preempt it outside of the barrier)
               {
                  core_id_t free_core_id = findFreeCoreForThread(thread_id);
                  if (free_core_id != INVALID_THREAD_ID)                               // Thread's new core is free
                  {
                     // We have just been moved to a different core, and that core is free. Schedule us there now.
                     Core *core = Sim()->getCoreManager()->getCoreFromID(free_core_id);
                     SubsecondTime time = std::max(core->getPerformanceModel()->getElapsedTime(), Sim()->getClockSkewMinimizationServer()->getGlobalTime());
                     reschedule(time, free_core_id, false);
                  }
               }
            }

            threads_on_core[from].clear();
         }

         for (int i=0;i<num_cores;i++)
         {
            for (std::vector<int>::iterator it=temp_threads_on_core[i].begin();it!=temp_threads_on_core[i].end();it++)
               threads_on_core[i].push_back (*it);
         }
      }

      mapping_file.close ();

   }

#endif

#ifdef MODIFIED_CODE

   void SchedulerPinnedBase::print_core_thread_map ()
   {
      int num_cores = Sim()->getConfig()->getApplicationCores();

      std::ofstream current_mapping_file;
      String path_to_currrent_mapping_file = Sim()->getCfg()->getString("general/path_to_current_mapping_file");
      current_mapping_file.open (path_to_currrent_mapping_file.c_str());
      
      current_mapping_file<<"Core_id App_id Mini-workload_rank\n";

      for (int i=0;i<num_cores;i++)
      {
         int app_id,thread_id;

         current_mapping_file<<i<<" ";

         if (threads_on_core[i].empty())
         {
            current_mapping_file<<"-1 -1\n";
            continue;
         }
	 

         for (std::vector<int>::iterator it=threads_on_core[i].begin();it!=threads_on_core[i].end();it++)
         {
            thread_id = *it;
            app_id = Sim()->getThreadManager()->getThreadFromID(thread_id)->getAppId();

            current_mapping_file<<app_id<<" "<<mw_rank_of_thread[thread_id]<<" ";
            current_mapping_file.flush ();
         }

         current_mapping_file<<"\n";
         current_mapping_file.flush();
      }

      current_mapping_file.close();
   }

#endif

#ifdef MODIFIED_CODE

   // This function is used to update the DVFS after every period. The DVFS values are read from a file and the valuesa re updated. A value <= 0 indiacated that core is broken
   void SchedulerPinnedBase::updateDVFS()
   {
      // Read from the file and change the frequencies
      String dvfs_file_name = Sim()->getCfg()->getString("general/dvfs_file");
      int num_cores = Sim()->getConfig()->getApplicationCores();

      std::ifstream dvfs_file;
      dvfs_file.open (dvfs_file_name.c_str());

      UInt64 frequency_in_mhz;

      for (core_id_t core_id=0;core_id<num_cores;core_id++)
      {
         dvfs_file>>frequency_in_mhz;
         Sim()->getMagicServer()->setFrequency(core_id, frequency_in_mhz);
      }

   }

#endif

void SchedulerPinnedBase::periodic(SubsecondTime time)
{
   // At the reset_time, we will update the hotspot temperature values and give a callback to scheduler to notify any changes.
   // Using this (time >= m_last_periodic_interval + m_interval) is a better way to write code, although it may be slow.

   #ifdef MODIFIED_CODE

      // if  ((current_threads_arrived>=95)&&(time >= m_last_periodic_interval + m_interval))

      int runtime_intervention = Sim()->getCfg()->getInt("general/runtime_intervention");

      if (runtime_intervention && (time >= m_last_periodic_interval + m_interval))
      {
         // 1. Generate LOG for temeperature using Hotspot
         // 2. Call Algorithm
         // 3. Read from file the new mapping

         power_out_file.close ();

         #ifdef DEBUG_PRINT
            std::cout<<"\n[DEBUG PRINT] Inside Periodic and Intervening\n";
         #endif

         String hotspot_command = Sim()->getCfg()->getString("general/hotspot_command");

         int ret_val = system (hotspot_command.c_str());
         ret_val++;

         // Write the current mapping to the file
         print_core_thread_map();

         // Print Current time and epoch-number
         std::ofstream time_file;
         String path_to_time_file = Sim()->getCfg()->getString("general/path_to_time_file");
         time_file.open (path_to_time_file.c_str());

         time_file<<time<<"\n";
         time_file.flush();

         time_file.close();

         /*
         * Instructions count logging and writing to corresponding file
         * The format will be : instruction_count_of_ith_core | start_time_interval | end_time_interval
         */
         String path_to_ic_file = Sim()->getCfg()->getString("general/path_to_ic_file");
         std::ofstream ic_file;
         ic_file.open (path_to_ic_file.c_str());

         for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
         {
            UInt64 instruction_count = Sim()->getCoreManager()->getCoreFromID(core_id)->getInstructionCount();
            instruction_count -= m_ic_core_previous[core_id];
            m_ic_core_previous[core_id] = Sim()->getCoreManager()->getCoreFromID(core_id)->getInstructionCount();

            // ic_file<<instruction_count<<" "<<m_last_periodic_interval<<" "<<time<<"\n"; 
            ic_file<<m_ic_core_previous[core_id]<<"\n";
         }

         ic_file.close();

         // Put algorithm call here

         String algorithm_command = Sim()->getCfg()->getString("general/algorithm_command");

         ret_val = system (algorithm_command.c_str());
         ret_val++;

         // Write code here to re-schedule the threads according to the new mappings
         changeMapping();
         updateDVFS();

         // Remake the power-trace file
         String path_to_ptrace = Sim()->getCfg()->getString("general/path_to_ptrace");
         power_out_file.open (path_to_ptrace.c_str());
         
         for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
         {
            power_out_file<<"C"<<core_id<<" ";
            power_out_file.flush ();

         }
         power_out_file<<"\n";
         power_out_file.flush ();

         m_last_periodic_interval = time; // Update the last time hotpot was called

      }

   #endif
      
   SubsecondTime delta = time - m_last_periodic;


   for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
   {

      // The energy of each core can be retrived as Sim()->getStatsManager()->getMetricObject("core", core_id, "energy-dynamic")->recordMetric()

      #ifdef MODIFIED_CODE

         UInt64 current_energy = Sim()->getStatsManager()->getMetricObject("core", core_id, "energy-dynamic")->recordMetric() + 
                                 Sim()->getStatsManager()->getMetricObject("core", core_id, "energy-static")->recordMetric()  + 
                                 Sim()->getStatsManager()->getMetricObject("L2", core_id, "energy-dynamic")->recordMetric()   +
                                 Sim()->getStatsManager()->getMetricObject("L2", core_id, "energy-static")->recordMetric();

         double power = (current_energy - m_energy_core_previous[core_id])/(double) delta.getInternalDataForced();

         double power_factor = (double)Sim()->getCfg()->getFloat("general/power_factor");

	      power *= power_factor;

         // Storing back the current energy
         m_energy_core_previous[core_id] = Sim()->getStatsManager()->getMetricObject("core", core_id, "energy-dynamic")->recordMetric()+ 
                                          Sim()->getStatsManager()->getMetricObject("core", core_id, "energy-static")->recordMetric()  + 
                                          Sim()->getStatsManager()->getMetricObject("L2", core_id, "energy-dynamic")->recordMetric()   +
                                          Sim()->getStatsManager()->getMetricObject("L2", core_id, "energy-static")->recordMetric();

         // Write to the power file
         power_out_file<<power<<" ";
         power_out_file.flush ();

      #endif

      if (delta > m_quantum_left[core_id] || m_core_thread_running[core_id] == INVALID_THREAD_ID)
      {
         reschedule(time, core_id, true);
      }
      else
      {
         m_quantum_left[core_id] -= delta;
      }
   }

   #ifdef MODIFIED_CODE

      power_out_file<<"\n";
      power_out_file.flush ();

   #endif

   m_last_periodic = time;
}

void SchedulerPinnedBase::reschedule(SubsecondTime time, core_id_t core_id, bool is_periodic)
{
   thread_id_t current_thread_id = m_core_thread_running[core_id];

   if (current_thread_id != INVALID_THREAD_ID
       && Sim()->getThreadManager()->getThreadState(current_thread_id) == Core::INITIALIZING)
   {
      // Thread on this core is starting up, don't reschedule it for now
      return;
   }

   thread_id_t new_thread_id = INVALID_THREAD_ID;
   SInt64 max_score = INT64_MIN;

   for(thread_id_t thread_id = 0; thread_id < (thread_id_t)m_threads_runnable.size(); ++thread_id)
   {
      if (m_thread_info[thread_id].hasAffinity(core_id) // Thread is allowed to run on this core
          && m_threads_runnable[thread_id] == true      // Thread is not stalled
          && (!m_thread_info[thread_id].isRunning()     // Thread is not already running somewhere else
              || m_thread_info[thread_id].getCoreRunning() == core_id)
      )
      {
         SInt64 score;
         if (m_thread_info[thread_id].isRunning())
            // Thread is currently running: negative score depending on how long it's already running
            score = SInt64(m_thread_info[thread_id].getLastScheduledIn().getPS()) - time.getPS();
         else
            // Thread is not currently running: positive score depending on how long we have been waiting
            score = time.getPS() - SInt64(m_thread_info[thread_id].getLastScheduledOut().getPS());

         // Find thread that was scheduled the longest time ago
         if (score > max_score)
         {
            new_thread_id = thread_id;
            max_score = score;
         }
      }
   }

   if (current_thread_id != new_thread_id)
   {
      // If a thread was running on this core, and we'll schedule another one, unschedule the current one
      if (current_thread_id != INVALID_THREAD_ID)
      {
         m_thread_info[current_thread_id].setCoreRunning(INVALID_CORE_ID);
         // Update last scheduled out time, with a small extra penalty to make sure we don't
         // reconsider this thread in the same periodic() call but for a next core
         m_thread_info[current_thread_id].setLastScheduledOut(time + SubsecondTime::PS(core_id));
         moveThread(current_thread_id, INVALID_CORE_ID, time);
      }

      // Set core as running this thread *before* we call moveThread(), otherwise the HOOK_THREAD_RESUME callback for this
      // thread might see an empty core, causing a recursive loop of reschedulings
      m_core_thread_running[core_id] = new_thread_id;

      // If we found a new thread to schedule, move it here
      if (new_thread_id != INVALID_THREAD_ID)
      {
         // If thread was running somewhere else: let that core know
         if (m_thread_info[new_thread_id].isRunning())
            m_core_thread_running[m_thread_info[new_thread_id].getCoreRunning()] = INVALID_THREAD_ID;
         // Move thread to this core
         m_thread_info[new_thread_id].setCoreRunning(core_id);
         m_thread_info[new_thread_id].setLastScheduledIn(time);
         moveThread(new_thread_id, core_id, time);
      }
   }

   m_quantum_left[core_id] = m_quantum;
}

String SchedulerPinnedBase::ThreadInfo::getAffinityString() const
{
   std::stringstream ss;

   for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
   {
      if (hasAffinity(core_id))
      {
         if (ss.str().size() > 0)
            ss << ",";
         ss << core_id;
      }
   }
   return String(ss.str().c_str());
}

void SchedulerPinnedBase::printState()
{
   printf("thread state:");
   for(thread_id_t thread_id = 0; thread_id < (thread_id_t)Sim()->getThreadManager()->getNumThreads(); ++thread_id)
   {
      char state;
      switch(Sim()->getThreadManager()->getThreadState(thread_id))
      {
         case Core::INITIALIZING:
            state = 'I';
            break;
         case Core::RUNNING:
            state = 'R';
            break;
         case Core::STALLED:
            state = 'S';
            break;
         case Core::SLEEPING:
            state = 's';
            break;
         case Core::WAKING_UP:
            state = 'W';
            break;
         case Core::IDLE:
            state = 'I';
            break;
         case Core::BROKEN:
            state = 'B';
            break;
         case Core::NUM_STATES:
         default:
            state = '?';
            break;
      }
      if (m_thread_info[thread_id].isRunning())
      {
         printf(" %c@%d", state, m_thread_info[thread_id].getCoreRunning());
      }
      else
      {
         printf(" %c%c%s", state, m_threads_runnable[thread_id] ? '+' : '_', m_thread_info[thread_id].getAffinityString().c_str());
      }
   }
   printf("  --  core state:");
   for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
   {
      if (m_core_thread_running[core_id] == INVALID_THREAD_ID)
         printf(" __");
      else
         printf(" %2d", m_core_thread_running[core_id]);
   }
   printf("\n");
}
