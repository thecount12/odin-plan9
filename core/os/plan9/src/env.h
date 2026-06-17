#ifndef ENV_H
#define ENV_H

char *sys_getenv(char *key);
int sys_setenv(char *key, char *value);
int sys_unsetenv(char *key);

#endif
