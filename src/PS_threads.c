
#include "PS_threads.h"
#include "utils.h"
#include "scheduler.h"
#include <string.h>
#include "PS_error.h"

#include <stdio.h>



extern hidden void create_decay_scheduler(computing_param* param);
extern hidden void destroy_decay_scheduler(computing_param* param);

static __thread struct ps_thread ps_tls_data;

hidden struct ps_thread *get_ps_thread (void)
{
   return &ps_tls_data;
}

hidden PS_Pool* create_pool(int size)
{
   ps_thread * threads = malloc(sizeof(struct ps_thread) * size);
   if (threads == NULL)
      fatal_error("out of memory");
   PS_Pool* pool = malloc(sizeof(PS_Pool));
   if (pool == NULL)
      fatal_error("out of memory");
   pool->threads = threads;
   pool->size = size;
   return pool;
}

hidden int destroy_pool(PS_Pool* pool)
{
   if (pool == NULL || pool->threads == NULL)
      return -1;
   free(pool->threads);
   free(pool);
   return 0;
}

hidden static void* boot_computing(void* args)
{

   ps_thread * arg = (struct ps_thread *) args;
   struct ps_thread* tld = get_ps_thread();

   tld->param = arg->param;
   tld->t = arg->t;
   tld->id = arg->id;
   tld->ps_pool = arg->ps_pool;
   tld->pool_size = arg->pool_size;
   tld->timer.start = 0;
   tld->timer.end = 0;

   computing_param * param = tld->param;
   parallel_scheduler next = param->next;

   PS_interval interval;
   if (param->sctl.ps_task->is_loop)
   {
      do
      {
         next(&interval, &param->sctl);
         if (interval.start != interval.end)
         {
            tld->timer.start = PS_get_wall_time();
            param->sctl.ps_task->routine(param->sctl.ps_task->context,
                                         &interval);
            tld->timer.end = PS_get_wall_time();
         }
      } while (interval.start != interval.end);
   }
   else
   {
      fatal_error("Unsupported Non-loop task");
   }
   return NULL;
}


hidden static int init_computing_param(computing_param* param, PSTask* task)
{
   param->sctl.ps_task = malloc(sizeof (PSTask));
   memcpy(param->sctl.ps_task, task, sizeof (PSTask));
   pthread_spin_init(&param->sctl.lock, 0);
   param->sctl.current = task->interval.start;
   param->sctl.n = 0;
   return 0;
}

hidden static int destroy_computing_param(computing_param * param)
{
   pthread_spin_destroy(&param->sctl.lock);
   free(param->sctl.ps_task);
   return 0;
}

hidden static int create_scheduler(computing_param* param)
{
   switch (param->sctl.ps_task->sche_type) {
      case STATIC:
         param->next = static_scheduler;
         break;
      case DYNAMIC:
         param->next = dynamic_scheduler;
         break;
      case GUIDED:
         param->next = guided_scheduler;
         break;
      case DECAY:
         create_decay_scheduler(param);
         param->next = decay_scheduler;
         break;
      default:
         fatal_error("Unsupported scheduler type");
   }
   return 0;
}

hidden static int destroy_scheduler(computing_param* param)
{
   switch (param->sctl.ps_task->sche_type) {
      case STATIC:
         break;
      case DYNAMIC:
         break;
      case GUIDED:
         break;
      case DECAY:
         destroy_decay_scheduler(param);
         break;
      default:
         fatal_error("Unsupported scheduler type");
   }
   return 0;
}



profile boot(PSTask* task)
{
   PS_Pool* pool = create_pool(task->size);
   
   profile timer;

   computing_param param;
   init_computing_param(&param, task);
   create_scheduler(&param);

   timer.start = PS_get_wall_time();

   for(int i = 0; i < pool->size; ++i)
   {
      pool->threads[i].id = i;
      pool->threads[i].ps_pool = pool;
      pool->threads[i].pool_size = pool->size;
      pool->threads[i].param = &param;
      pool->threads[i].n_next = 0;
      pthread_create(&pool->threads[i].t, NULL, boot_computing, &pool->threads[i]);
   }
   // barrier
   for(int i = 0; i < pool->size; ++i)
   {
      pthread_join(pool->threads[i].t, NULL);
   }
   
   timer.end = PS_get_wall_time();
   
   // release some resources
   destroy_scheduler(&param);
   destroy_computing_param(&param);
   destroy_pool(pool);
   return timer;
}
