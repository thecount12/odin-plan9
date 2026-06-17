#ifndef DIR_H
#define DIR_H

typedef struct SysDir SysDir;

typedef struct DirEnt {
	char name[256];
	int is_dir;
} DirEnt;

SysDir *sys_opendir(char *path);
int sys_readdir(SysDir *dir, DirEnt *ent);
int sys_closedir(SysDir *dir);

#endif
