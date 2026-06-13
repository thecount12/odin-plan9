#include <stdio.h>
#include "include/dir.h"
#include "include/filesys.h"
#include "include/mem.h"

int main(void) {
    int failed = 0;
    const char *dir_path = "/tmp/odin_posix_dir_test";
    SysDir *dir;
    DirEnt ent;
    int found_file;
    int result;

    sys_mkdir(dir_path, 0755);

    {
        fd_t fd;
        const char *name = "/tmp/odin_posix_dir_test/entry.txt";
        const char *text = "dir test";

        fd = sys_open(name, O_WRONLY | O_CREATE | O_TRUNC);
        if (fd >= 0) {
            sys_write(fd, text, (long)sys_strlen(text));
            sys_close(fd);
        }
    }

    dir = sys_opendir(dir_path);
    if (dir == NULL) {
        printf("FAIL: opendir\n");
        failed = 1;
    } else {
        found_file = 0;
        while ((result = sys_readdir(dir, &ent)) == 1) {
            if (sys_memcmp(ent.name, "entry.txt", 10) == 0) {
                found_file = 1;
            }
            printf("  entry: '%s' is_dir=%d\n", ent.name, ent.is_dir);
        }

        if (result < 0) {
            printf("FAIL: readdir error\n");
            failed = 1;
        } else if (!found_file) {
            printf("FAIL: entry.txt not found in directory listing\n");
            failed = 1;
        } else {
            printf("PASS: directory listing\n");
        }

        sys_closedir(dir);
    }

    sys_unlink("/tmp/odin_posix_dir_test/entry.txt");
    sys_rmdir(dir_path);

    return failed ? 1 : 0;
}
