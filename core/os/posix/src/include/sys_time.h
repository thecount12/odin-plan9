#ifndef SYS_TIME_H
#define SYS_TIME_H

#include "sysdeps.h"
#include <time.h>

time_t sys_time(void);
void sys_sleep(int ms);
ulonglong sys_nanotime(void);

#endif
