
#include "task.h"

void ps_init_ps_task(struct PSTask* task)
{
   task->size = 1;
   task->sche_type = STATIC;
   task->is_loop = true;
   task->theta = .5;
   task->chunk_size = 1;
   task->min_chunk_size = 1;
}
