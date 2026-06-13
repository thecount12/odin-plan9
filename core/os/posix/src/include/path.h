#ifndef PATH_H
#define PATH_H

#include "sysdeps.h"

#define SYS_PATH_MAX 4096

int sys_getcwd(char *buf, long size);
int sys_chdir(const char *path);

#endif
