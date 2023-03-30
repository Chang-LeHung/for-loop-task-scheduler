

#ifndef PARALLEL_SCHEDULER_TASK_H
#define PARALLEL_SCHEDULER_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _time{
    double start;
    double end;
}profile;

#include <stdbool.h>
#define hidden __attribute__((visibility("hidden")))

typedef struct interval {
    long start;
    long end;
    int step;
    // nth interval
    long n;
} PS_interval;

typedef enum scheduler_type {
    STATIC,
    DYNAMIC,
    GUIDED,
    DECAY
}scheduler_type;


typedef void (*task)(void *context, PS_interval * interval);

typedef struct PSTask {
    enum scheduler_type sche_type;

    int size; // number of threads
    /* chunk size filed is used by static scheduler & dynamic scheduler as fixed chunk size
     * decay scheduler use it as initial chunk size
     */
    int chunk_size;

    /*
     * this filed only used by decay scheduler
     */
    int min_chunk_size;

    task routine;

    /* a object shared by all threads
     * you can pass variable to your task routine by this variable*/
    void *context;

    PS_interval interval;

    float theta;

    bool is_loop;
} PSTask;

void ps_init_ps_task(struct PSTask* task);

#ifdef __cplusplus
}
#endif

#endif //PARALLEL_SCHEDULER_TASK_H
