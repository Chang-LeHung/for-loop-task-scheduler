
#include <time.h>
#include "utils.h"


hidden double PS_get_wall_time()
{
   struct timespec ts;
# ifdef CLOCK_MONOTONIC
   if (clock_gettime (CLOCK_MONOTONIC, &ts) < 0)
# endif
      clock_gettime (CLOCK_REALTIME, &ts);
   return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}