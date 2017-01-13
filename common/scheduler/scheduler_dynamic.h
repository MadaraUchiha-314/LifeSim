#ifndef __SCHEDULER_DYNAMIC_H
#define __SCHEDULER_DYNAMIC_H

#include "scheduler.h"
#include "hooks_manager.h"
#include "magic_server.h"

#include <vector>
#include <iostream>

#include "config-paths.h"

class SchedulerDynamic : public Scheduler
{
   public:
      SchedulerDynamic(ThreadManager *thread_manager);
      virtual ~SchedulerDynamic();

      virtual core_id_t threadCreate(thread_id_t) = 0;
      virtual void periodic(SubsecondTime time) {}
      virtual void threadStart(thread_id_t thread_id, SubsecondTime time) {}
      virtual void threadStall(thread_id_t thread_id, ThreadManager::stall_type_t reason, SubsecondTime time) {}
      virtual void threadResume(thread_id_t thread_id, thread_id_t thread_by, SubsecondTime time) {}
      virtual void threadExit(thread_id_t thread_id, SubsecondTime time) {}

      static int num_apps_exited;

   protected:
      std::vector<bool> m_threads_runnable;

      void moveThread(thread_id_t thread_id, core_id_t core_id, SubsecondTime time);

   private:
      bool m_in_periodic;

      void __periodic(SubsecondTime time);
      void __roi_begin();
      void __roi_end();
      void __threadStart(thread_id_t thread_id, SubsecondTime time);
      void __threadStall(thread_id_t thread_id, ThreadManager::stall_type_t reason, SubsecondTime time);
      void __threadResume(thread_id_t thread_id, thread_id_t thread_by, SubsecondTime time);
      void __threadExit(thread_id_t thread_id, SubsecondTime time);

      // Hook stubs
      static SInt64 hook_periodic(UInt64 ptr, UInt64 time)
      { ((SchedulerDynamic*)ptr)->__periodic(*(subsecond_time_t*)&time); return 0; }
      static SInt64 hook_thread_start(UInt64 ptr, UInt64 _args)
      {
         HooksManager::ThreadTime *args = (HooksManager::ThreadTime *)_args;
         ((SchedulerDynamic*)ptr)->__threadStart(args->thread_id, args->time);
         return 0;
      }
      static SInt64 hook_thread_stall(UInt64 ptr, UInt64 _args)
      {
         HooksManager::ThreadStall *args = (HooksManager::ThreadStall *)_args;
         ((SchedulerDynamic*)ptr)->__threadStall(args->thread_id, args->reason, args->time);
         return 0;
      }
      static SInt64 hook_thread_resume(UInt64 ptr, UInt64 _args)
      {
         HooksManager::ThreadResume *args = (HooksManager::ThreadResume *)_args;
         ((SchedulerDynamic*)ptr)->__threadResume(args->thread_id, args->thread_by, args->time);
         return 0;
      }
      static SInt64 hook_thread_exit(UInt64 ptr, UInt64 _args)
      {
         HooksManager::ThreadTime *args = (HooksManager::ThreadTime *)_args;
         ((SchedulerDynamic*)ptr)->__threadExit(args->thread_id, args->time);
         return 0;
      }
      static SInt64 end_of_benchmark_callback (UInt64 dummy, UInt64 _args)
      {

         #ifdef DEBUG_PRINT

            MagicServer::MagicMarkerType *marker_type = (MagicServer::MagicMarkerType*) _args ;
            if (marker_type->str != NULL)
               std::cout<<"\n[DEBUG PRINT] Arg 0 = "<<marker_type->arg0<<"\t Arg 1 = "<<marker_type->arg1<<" String is "<<marker_type->str<<"\n";
            else
               std::cout<<"\n[DEBUG PRINT] Arg 0 = "<<marker_type->arg0<<"\t Arg 1 = "<<marker_type->arg1<<"\n";

            fflush (stdout);

         #endif

         #ifdef PERIODIC_WORKLOAD
            // TODO: Remove the hard-coding ASAP
            if (marker_type->arg0 == 1991)
            {
               num_apps_exited++;
               #ifdef DEBUG_PRINT
                  std::cout<<"\n[DEBUG PRINT] Got App Exit Marker\n";
                  fflush (stdout);
               #endif
            }  
         #endif
            
         return 0;            
      }
};

#endif // __SCHEDULER_DYNAMIC_H
