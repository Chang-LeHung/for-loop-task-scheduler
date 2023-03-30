
#ifndef PARALLEL_SCHEDULER_SCHEDULER_H
#define PARALLEL_SCHEDULER_SCHEDULER_H

#include "task.h"
#include <pthread.h>

typedef struct scheduler_control {
    pthread_spinlock_t lock;
    long current;
    PSTask* ps_task;
    void* context;
    /* nth split*/
    int n;
}scheduler_control;

// scheduler_control
typedef void (*parallel_scheduler)(PS_interval* itl, scheduler_control* ctl);

typedef struct _computing_param{
    scheduler_control sctl;
    parallel_scheduler next;
}computing_param;


hidden void static_scheduler(PS_interval* itl, scheduler_control* ctl);
hidden void dynamic_scheduler(PS_interval* itl, scheduler_control* ctl);
hidden void guided_scheduler(PS_interval* itl, scheduler_control* ctl);
hidden void decay_scheduler(PS_interval* itl, scheduler_control* ctl);

/*
 * S_{i} = \theta\cdot S_{i - 1} + (1 - \theta)\cdot T_{i}
 * CS_{i} = max(min, \lceil CS_{i - 1}\cdot \frac{S_{i - 1}}{T_{i - 1}} \rceil)
 */
typedef struct _decay{
    /* latest `i` of decay algorithm*/
    long sentinel;
    double * time;
    double Si;
    /* number of intervals*/
    long n_interval;
    int csi; // latest chunk size
    float theta;
    int min_chunk_size;
}decay;

#define PS_itl(param) param->sctl.ps_task->interval
#define PS_task(param) param->sctl.ps_task
#define PS_ctl(param) param->sctl
#endif //PARALLEL_SCHEDULER_SCHEDULER_H
