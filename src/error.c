

#include "PS_error.h"
#include <stdlib.h>
#include <stdio.h>

hidden void fatal_error(const char* msg)
{
   fprintf(stderr, "Parallel Scheduler Fatal error: ");
   fprintf(stderr, msg);
   fprintf(stderr, "\n");
   exit(-1);
}