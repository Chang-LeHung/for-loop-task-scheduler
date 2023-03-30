

#include "scheduler.h"
#include "task.h"
#include "PS_threads.h"
#include <string.h>

#ifdef Debug
   #include <stdio.h>
#endif

extern struct ps_thread *get_ps_thread (void);

#define ACQUIRE pthread_spin_lock(&ctl->lock);
#define RELEASE pthread_spin_unlock(&ctl->lock);

#define EnsureInBoundary if (itl->step > 0)                 \
                           {                                \
                              if (itl->end > end)           \
                                 itl->end = end;            \
                              if (itl->start > itl->end)    \
                                 itl->start = itl->end;     \
                           }                                \
                           else                             \
                           {                                \
                              if (itl->end < end)           \
                                 itl->end = end;            \
                              if (itl->start < itl->end)    \
                                 itl->start = itl->end;     \
                           }

#define get_interval(ctl) long start = ctl->ps_task->interval.start; \
                     long end = ctl->ps_task->interval.end;     \
                     int step = ctl->ps_task->interval.step;

#define increase_next(t) t->n_next++;

#define N_step(itl) (itl->end - itl->start) / itl->step

#define print_interval(itl)   if(itl->start != itl->end) \
                              {printf("tid = %d start = %ld end = %ld step_size= %d n_step = %ld\n", \
                              get_ps_thread()->id, itl->start, itl->end, itl->step, N_step(itl));}

#define execution_time(t) t->timer.end - t->timer.start

/*
 * this scheduler do not need a lock for interval splitting
 */
hidden void static_scheduler(PS_interval* itl, scheduler_control* ctl)
{
   struct ps_thread* t = get_ps_thread();
   get_interval(ctl)
   if (ctl->ps_task->chunk_size == -1)
   {
      if (t->n_next > 0)
      {
         itl->start = itl->end;
         return;
      }
      long nstep = (long)(((double)(end - start) / step) + 1);
      long step_per_thread = nstep / ctl->ps_task->size + 1;
      long step_size = step_per_thread * step;
      itl->start = step_size * t->id;
      itl->end = step_size * (t->id + 1);
      itl->step = ctl->ps_task->interval.step;
      if (t->id == ctl->ps_task->size - 1)
      {
         itl->end = end;
      }
      EnsureInBoundary
      t->n_next++;
   }
   else
   {
      long offset = ctl->ps_task->chunk_size * ctl->ps_task->interval.step;
      long base = (t->n_next++) * (ctl->ps_task->chunk_size * ctl->ps_task->size * ctl->ps_task->interval.step);
      itl->start = ctl->ps_task->interval.start + base + offset * t->id;
      itl->end = itl->start + offset;
      itl->step = ctl->ps_task->interval.step;
      EnsureInBoundary
   }
#ifdef Debug
   print_interval(itl);
#endif
}

hidden void dynamic_scheduler(PS_interval* itl, scheduler_control* ctl)
{
   struct ps_thread* t = get_ps_thread();
   get_interval(ctl)
   itl->step = step;
   ACQUIRE
   // critical area
   long current = ctl->current;
   itl->start = current;
   itl->end = current + ctl->ps_task->chunk_size * step;
   ctl->current = itl->end;
   RELEASE
   EnsureInBoundary
   increase_next(t)
#ifdef Debug
   print_interval(itl)
#endif
}

/*
function `guided_scheduler` is a recurrence of the function `gomp_iter_guided_next_locked`
in libgomp (GNU openmp runtime library):

```c
bool
gomp_iter_guided_next_locked (long *pstart, long *pend)
{
  struct gomp_thread *thr = gomp_thread ();
  struct gomp_work_share *ws = thr->ts.work_share;
  struct gomp_team *team = thr->ts.team;
  unsigned long nthreads = team ? team->nthreads : 1;
  unsigned long n, q;
  long start, end;

  if (ws->next == ws->end)
    return false;

  start = ws->next;
  n = (ws->end - start) / ws->incr;
  q = (n + nthreads - 1) / nthreads;

  if (q < ws->chunk_size)
    q = ws->chunk_size;
  if (q <= n)
    end = start + q * ws->incr;
  else
    end = ws->end;

  ws->next = end;
  *pstart = start;
  *pend = end;
  return true;
}

```
 */
hidden void guided_scheduler(PS_interval* itl, scheduler_control* ctl)
{
   get_interval(ctl)
   itl->step = step;
   ACQUIRE
   long current = ctl->current;
   itl->start = current;
   long n = (end - current) / step;
   long q = (n + ctl->ps_task->size - 1) / ctl->ps_task->size;
   if (q < ctl->ps_task->chunk_size)
      q = ctl->ps_task->chunk_size;
   long t;
   if (q <= n)
      t = current + q * step;
   else
      t = end;
   ctl->current = t;
   itl->end = t;
   RELEASE
   EnsureInBoundary
#ifdef Debug
   print_interval(itl)
#endif
}

hidden void create_decay_scheduler(computing_param* param)
{
   struct _decay* p = malloc(sizeof (decay));
   p->theta = param->sctl.ps_task->theta;
   long n = ((PS_itl(param).end - PS_itl(param).start)) /PS_itl(param).step + 1;
   long size = sizeof (double) * n;
   p->time = malloc(size);
   memset(p->time, 0, size);
   p->n_interval = n;
   p->csi = PS_task(param)->chunk_size;
   p->min_chunk_size = PS_task(param)->min_chunk_size;
   PS_ctl(param).context = p;
   p->sentinel = 0;
}

hidden void destroy_decay_scheduler(computing_param* param)
{
   if (param == NULL)
      return;
   struct _decay* p = PS_ctl(param).context;
   free(p->time);
   free(p);
}


hidden void decay_scheduler(PS_interval* itl, scheduler_control* ctl)
{
   get_interval(ctl)
   itl->step = step;
   struct _decay* p =  ctl->context;
   struct ps_thread* t = get_ps_thread();
   ACQUIRE

   if (t->timer.start == 0)
   {
      goto next;
   }
   else
   {
      double time = execution_time(t);
      if(p->sentinel == 0)
      {
         p->time[p->sentinel] = time;
         p->Si = time;
         // update latest chunk size
         p->csi = t->lcs;
         goto next;
      }
      else
      {
         p->Si *= (1 - p->theta);
         p->Si += p->theta * time;
         int cs = (int)(p->Si / time) * t->lcs;
         t->lcs = cs < p->min_chunk_size ? p->min_chunk_size : cs;
         long cur = ctl->current;
         itl->start = cur;
         itl->end = itl->start + t->lcs;
         ctl->current = itl->end;
         itl->n = ctl->n++;
         goto end;
      }
   }
   next:{
         long cur = ctl->current;
         itl->start = cur;
         itl->end = itl->start + itl->step * ctl->ps_task->chunk_size;
         ctl->current = itl->end;
         // save chunk size
         t->lcs = ctl->ps_task->chunk_size;
         itl->n = ctl->n++;
   };
   end:;
   RELEASE
   EnsureInBoundary
#ifdef Debug
   print_interval(itl)
#endif
}