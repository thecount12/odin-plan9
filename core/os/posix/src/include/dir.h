#ifndef DIR_H
#define DIR_H

#include "sysdeps.h"

typedef struct SysDir SysDir;

typedef struct DirEnt {
    char name[256];
    int is_dir;
} DirEnt;

SysDir* sys_opendir(const char *path);
int sys_readdir(SysDir *dir, DirEnt *ent);
int sys_closedir(SysDir *dir);

#endif
