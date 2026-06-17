#include "common.h"
#include "sysdeps.h"
#include "path.h"
#include "filesys.h"

void
main(void)
{
	int failed;
	char cwd[SYS_PATH_MAX];
	char inside[SYS_PATH_MAX];
	char *subdir;

	failed = 0;
	subdir = "/tmp/odin_plan9_path_test";

	if(sys_getcwd(cwd, sizeof(cwd)) != 0) {
		print("FAIL: getcwd\n");
		failed = 1;
	} else {
		print("PASS: getcwd = '%s'\n", cwd);
	}

	sys_mkdir(subdir, 0755);

	if(sys_chdir(subdir) != 0) {
		print("FAIL: chdir into test dir\n");
		failed = 1;
	} else {
		if(sys_getcwd(inside, sizeof(inside)) != 0) {
			print("FAIL: getcwd after chdir\n");
			failed = 1;
		} else {
			print("PASS: chdir (now in '%s')\n", inside);
		}
		sys_chdir(cwd);
	}

	sys_rmdir(subdir);

	if(!failed)
		print("\nAll path tests passed!\n");

	exits(failed ? "fail" : nil);
}
