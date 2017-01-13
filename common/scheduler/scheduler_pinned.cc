#include "scheduler_pinned.h"
#include "config.hpp"
#include "thread.h"

#include <iostream>
#include <fstream>

SchedulerPinned::SchedulerPinned(ThreadManager *thread_manager)
   : SchedulerPinnedBase(thread_manager, SubsecondTime::NS(Sim()->getCfg()->getInt("scheduler/pinned/quantum")))
   , m_interleaving(Sim()->getCfg()->getInt("scheduler/pinned/interleaving"))
   , m_next_core(0)
{
   m_core_mask.resize(Sim()->getConfig()->getApplicationCores());

   for (core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id++)
   {
       m_core_mask[core_id] = Sim()->getCfg()->getBoolArray("scheduler/pinned/core_mask", core_id);
   }


   #ifdef MODIFIED_CODE

      // Code for making all the affinites = false.
      for (core_id_t core_id = 0;core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id++)
         m_core_mask[core_id] = false;


      std::ifstream app_core_mapping;
      String path_to_input_to_thermal_simulation = Sim()->getCfg()->getString("general/path_to_input_to_thermal_simulation");
      app_core_mapping.open (path_to_input_to_thermal_simulation.c_str()); // The path is in config-paths.h

      int core_id,app_id,thread_id;

      while (app_core_mapping >> core_id >> app_id  >> thread_id)
      {
         #ifdef DEBUG_PRINT
            std::cout<<"\n[DEBUG PRINT] Mapping from file "<<core_id<<" "<<app_id<<" "<<thread_id<<"\n";
            fflush (stdout);
         #endif

         if (app_id != -1)
            core_thread_map.insert (make_pair(std::make_pair(app_id,thread_id),core_id));

      }

      app_core_mapping.close();

   #endif

}

core_id_t SchedulerPinned::getNextCore(core_id_t core_id)
{
   while(true)
   {
      core_id += m_interleaving;
      if (core_id >= (core_id_t)Sim()->getConfig()->getApplicationCores())
      {
         core_id %= Sim()->getConfig()->getApplicationCores();
         core_id += 1;
         core_id %= m_interleaving;
      }
      if (m_core_mask[core_id])
         return core_id;
   }
}

core_id_t SchedulerPinned::getFreeCore(core_id_t core_first)
{
   core_id_t core_next = core_first;

   do
   {
      if ((m_core_thread_running[core_next] == INVALID_THREAD_ID) && (m_core_mask[core_next]))
         return core_next;

      core_next = getNextCore(core_next);
   }
   while(core_next != core_first);

   return core_first;
}

void SchedulerPinned::threadSetInitialAffinity(thread_id_t thread_id)
{  
   
   // Below functions are not required in the modified case

   #ifndef MODIFIED_CODE
      core_id_t core_id = getFreeCore(m_next_core);
      m_next_core = getNextCore(core_id);
   #endif

   /*
   * Here we have to read from the initial-config file and then set the affinity likewise
   * Steps :
   * 1. Read from the map core_thread_map
   * 2. From the thread_id, get the app_id
   * 3. Set the affinity of the thread accordingly
   */

   #ifdef MODIFIED_CODE

      int key_thread,key_app;
      key_app = Sim()->getThreadManager()->getThreadFromID(thread_id)->getAppId();
      key_thread = current_thread_count[key_app];

      mw_rank_of_thread.insert (std::make_pair(thread_id,key_thread));

      // Add a condition to check whether key is in the map. It will anyway insert into the map.
      core_id_t core_id = core_thread_map[std::make_pair(key_app,key_thread)];

      threads_on_core[core_id].push_back (thread_id); // Store the info about where each thread is mapped

      current_thread_count[key_app] += 1;

      #ifdef DEBUG_PRINT
         std::cout<<"\n[DEBUG PRINT] Thread id is "<<thread_id<<" App id is "<<Sim()->getThreadManager()->getThreadFromID(thread_id)->getAppId()<<"\n";
         std::cout<<"\n[DEBUG PRINT] Placing Thread : "<<thread_id<<" in Core : "<<core_id<<"\n";
         fflush (stdout);
      #endif

   #endif

   m_thread_info[thread_id].setAffinitySingle(core_id); // Setting the affinity of the required core
}
