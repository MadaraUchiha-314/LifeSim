#ifndef __SCHEDULER_PINNED_BASE_H
#define __SCHEDULER_PINNED_BASE_H

#include "scheduler_dynamic.h"
#include "simulator.h"

#include <fstream>
#include <map>
#include <utility>

#include "RMU.h"

class SchedulerPinnedBase : public SchedulerDynamic
{
   public:
      SchedulerPinnedBase(ThreadManager *thread_manager, SubsecondTime quantum);

      virtual core_id_t threadCreate(thread_id_t);
      virtual void threadYield(thread_id_t thread_id);
      virtual bool threadSetAffinity(thread_id_t calling_thread_id, thread_id_t thread_id, size_t cpusetsize, const cpu_set_t *mask);
      virtual bool threadGetAffinity(thread_id_t thread_id, size_t cpusetsize, cpu_set_t *mask);

      virtual void periodic(SubsecondTime time);
      virtual void threadStart(thread_id_t thread_id, SubsecondTime time);
      virtual void threadStall(thread_id_t thread_id, ThreadManager::stall_type_t reason, SubsecondTime time);
      virtual void threadResume(thread_id_t thread_id, thread_id_t thread_by, SubsecondTime time);
      virtual void threadExit(thread_id_t thread_id, SubsecondTime time);

   protected:
      class ThreadInfo
      {
         public:
            ThreadInfo()
               : m_has_affinity(false)
               , m_explicit_affinity(false)
               , m_core_affinity(Sim()->getConfig()->getApplicationCores(), false)
               , m_core_running(INVALID_CORE_ID)
               , m_last_scheduled_in(SubsecondTime::Zero())
               , m_last_scheduled_out(SubsecondTime::Zero())
            {}
            /* affinity */
            void clearAffinity()
            {
               for(auto it = m_core_affinity.begin(); it != m_core_affinity.end(); ++it)
                  *it = false;
            }
            void setAffinitySingle(core_id_t core_id)
            {
               clearAffinity();
               addAffinity(core_id);
            }
            void addAffinity(core_id_t core_id) { m_core_affinity[core_id] = true; m_has_affinity = true; }
            bool hasAffinity(core_id_t core_id) const { return m_core_affinity[core_id]; }
            String getAffinityString() const;
            /* running on core */
            bool hasAffinity() const { return m_has_affinity; }
            bool hasExplicitAffinity() const { return m_explicit_affinity; }
            void setExplicitAffinity() { m_explicit_affinity = true; }
            void setCoreRunning(core_id_t core_id) { m_core_running = core_id; }
            core_id_t getCoreRunning() const { return m_core_running; }
            bool isRunning() const { return m_core_running != INVALID_CORE_ID; }
            /* last scheduled */
            void setLastScheduledIn(SubsecondTime time) { m_last_scheduled_in = time; }
            void setLastScheduledOut(SubsecondTime time) { m_last_scheduled_out = time; }
            SubsecondTime getLastScheduledIn() const { return m_last_scheduled_in; }
            SubsecondTime getLastScheduledOut() const { return m_last_scheduled_out; }
         private:
            bool m_has_affinity;
            bool m_explicit_affinity;
            std::vector<bool> m_core_affinity;
            core_id_t m_core_running;
            SubsecondTime m_last_scheduled_in;
            SubsecondTime m_last_scheduled_out;
      };

      // Configuration
      const SubsecondTime m_quantum;
      // Global state
      SubsecondTime m_last_periodic;
      // Keyed by thread_id
      std::vector<ThreadInfo> m_thread_info;
      // Keyed by core_id
      std::vector<thread_id_t> m_core_thread_running;
      std::vector<SubsecondTime> m_quantum_left;


      #ifdef MODIFIED_CODE

         // Indexed by core_id
         std::vector<UInt64> m_energy_core_previous; // Store the energy values of the core in the previos cycle
         std::ofstream power_out_file; // The file name is power-trace.ptrace
         // Indexed by core_id. Each entry for core_id contains a list of threads.
         typedef std::pair<int,int> CoreKey;
         std::map<CoreKey,int> core_thread_map; // For a given (thread_id and app_id) as key it gives the core on whcih it should be scheduled
         std::vector<std::vector<int> > threads_on_core; // At any instant of time, this holds the lists of threads running on a core
         std::vector<int> current_thread_count;
         std::map<int,int> mw_rank_of_thread;
         SubsecondTime m_interval;
         SubsecondTime m_last_periodic_interval;
         int m_num_apps;
         std::vector<double> core_pv;
         std::vector<UInt64> m_ic_core_previous; // Stores the instruction count of each core in the previous interval

      #endif

      #ifdef PERIODIC_WORKLOAD
         int m_hyper_period_number;
         std::vector<std::vector<int> > threads_of_app; // Keyed by app_id
         int m_num_threads_exited;
         SubsecondTime m_last_hyper_period_time;

      #endif

      #ifdef USE_RMU
         std::vector<SubsecondTime> end_times;
         SubsecondTime hyper_period_start_time;
         std::vector<double> core_temp;
         RMU my_rmu;
      #endif

      virtual void threadSetInitialAffinity(thread_id_t thread_id) = 0;

      core_id_t findFreeCoreForThread(thread_id_t thread_id);
      void reschedule(SubsecondTime time, core_id_t core_id, bool is_periodic);
      void printState();
      void changeMapping();
      void print_core_thread_map();
      void updateDVFS();
};

#endif // __SCHEDULER_PINNED_BASE_H
