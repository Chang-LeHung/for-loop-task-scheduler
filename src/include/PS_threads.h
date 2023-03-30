
#ifndef PARALLEL_SCHEDULER_THREADS_H
#define PARALLEL_SCHEDULER_THREADS_H

#include "task.h"
#include <pthread.h>
#include <stdlib.h>
#include "scheduler.h"

struct _pool;

typedef struct ps_thread{
    int id;
    int n_next; // times of call function next
    int pool_size; // number of thread
    pthread_t t; // pthread_t of a thread
    computing_param* param; // a shared object by all threads, which for boot_computing
    struct _pool* ps_pool; // a shared object by all threads
    profile timer;
    /* latest chunk size*/
    int lcs;
}ps_thread;


typedef struct _pool{
    int size;
    struct ps_thread* threads;
}PS_Pool;


hidden PS_Pool* create_pool(int size);

hidden int destroy_pool(PS_Pool* pool);

profile boot(struct PSTask* task);

#endif //PARALLEL_SCHEDULER_THREADS_H
