#include "include/dir.h"
#include "include/mem.h"
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>

struct SysDir {
    DIR *dir;
};

SysDir* sys_opendir(const char *path) {
    SysDir *dir;
    DIR *posix_dir;

    if (path == NULL) {
        sys_seterr(ERR_IO);
        return NULL;
    }

    posix_dir = opendir(path);
    if (posix_dir == NULL) {
        sys_seterr_posix();
        return NULL;
    }

    dir = (SysDir*)sys_malloc(sizeof(SysDir));
    if (dir == NULL) {
        closedir(posix_dir);
        sys_seterr(ERR_IO);
        return NULL;
    }

    dir->dir = posix_dir;
    return dir;
}

int sys_readdir(SysDir *dir, DirEnt *ent) {
    struct dirent *entry;

    if (dir == NULL || ent == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    entry = readdir(dir->dir);
    if (entry == NULL) {
        if (errno != 0) {
            return sys_seterr_posix();
        }
        return 0;
    }

    sys_memset(ent, 0, sizeof(DirEnt));
    sys_memcpy(ent->name, entry->d_name, sizeof(ent->name) - 1);
    ent->name[sizeof(ent->name) - 1] = '\0';

#if defined(DT_DIR)
    ent->is_dir = (entry->d_type == DT_DIR) ? 1 : 0;
#else
    ent->is_dir = 0;
#endif

    return 1;
}

int sys_closedir(SysDir *dir) {
    int result;

    if (dir == NULL) {
        sys_seterr(ERR_IO);
        return -1;
    }

    result = closedir(dir->dir);
    sys_free(dir);

    if (result < 0) {
        return sys_seterr_posix();
    }

    return 0;
}
