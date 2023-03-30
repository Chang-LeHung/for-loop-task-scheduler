
#include "ps.h"
#include <stdlib.h>
#include <stdio.h>

//typedef void (*task)(void *context, PS_interval * interval);

int fib(int n)
{
   if(n <= 1)
      return n;
   return fib(n - 1) + fib(n - 2);
}

void routine(void *context, PS_interval * interval)
{
   int* arr = (int*)context;
   for(long i = interval->start; i < interval->end; i += interval->step)
   {
      arr[i] = fib((int)i);
   }
}

int main()
{
#define N 50
   PSTask task;
   ps_init_ps_task(&task);
   task.size = 4;
   task.sche_type = GUIDED;
   task.interval.start = 0;
   task.interval.end = N;
   task.interval.step = 1;
   int* arr = malloc(sizeof (int) * N);
   task.context = arr;
   task.chunk_size = 2;
   task.routine = routine;
   profile timer;
   timer = boot(&task);
   for(int i = 0; i < N; ++i)
   {
      printf("arr[%d] = %d\n", i, arr[i]);
   }
   printf("start = %lf end = %lf\n", timer.start, timer.end);
   printf("cost = %lf\n", timer.end - timer.start);
   return 0;
}