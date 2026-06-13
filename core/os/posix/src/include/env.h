#ifndef ENV_H
#define ENV_H

#include "sysdeps.h"

const char* sys_getenv(const char *key);
int sys_setenv(const char *key, const char *value);
int sys_unsetenv(const char *key);

#endif
