#include "common.h"
#include "sysdeps.h"
#include "dir.h"
#include "filesys.h"
#include "mem.h"

void
main(void)
{
	int failed;
	char *dir_path;
	SysDir *dir;
	DirEnt ent;
	int found_file;
	int result;
	fd_t fd;
	char *name;
	char *text;

	failed = 0;
	dir_path = "/tmp/odin_plan9_dir_test";
	name = "/tmp/odin_plan9_dir_test/entry.txt";
	text = "dir test";

	sys_mkdir(dir_path, 0755);

	fd = sys_open(name, O_WRONLY|O_CREATE|O_TRUNC);
	if(fd >= 0) {
		sys_write(fd, text, sys_strlen(text));
		sys_close(fd);
	}

	dir = sys_opendir(dir_path);
	if(dir == nil) {
		print("FAIL: opendir\n");
		failed = 1;
	} else {
		found_file = 0;
		while((result = sys_readdir(dir, &ent)) == 1) {
			if(sys_memcmp(ent.name, "entry.txt", 10) == 0)
				found_file = 1;
			print("  entry: '%s' is_dir=%d\n", ent.name, ent.is_dir);
		}

		if(result < 0) {
			print("FAIL: readdir error\n");
			failed = 1;
		} else if(!found_file) {
			print("FAIL: entry.txt not found in directory listing\n");
			failed = 1;
		} else {
			print("PASS: directory listing\n");
		}

		sys_closedir(dir);
	}

	sys_unlink(name);
	sys_rmdir(dir_path);

	if(!failed)
		print("\nAll directory tests passed!\n");

	exits(failed ? "fail" : nil);
}
