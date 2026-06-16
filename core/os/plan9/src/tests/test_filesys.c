#include <u.h>
#include <libc.h>
#include "include/filesys.h"
#include "include/mem.h"
#include "include/sysdeps.h"

void
main(void)
{
	int failed;
	fd_t fd;
	char *test_path;
	char *test_str;
	long written;
	char buf[64];
	long nread;
	Stat st;

	failed = 0;
	test_path = "/tmp/odin_plan9_test.txt";
	test_str = "Hello from Odin Plan 9 backend";

	fd = sys_open(test_path, O_WRONLY|O_CREATE|O_TRUNC);
	if(fd < 0) {
		print("FAIL: Could not create test file\n");
		exits("fail");
	}

	written = sys_write(fd, test_str, sys_strlen(test_str));
	if(written != sys_strlen(test_str)) {
		print("FAIL: Write incorrect length\n");
		failed = 1;
	} else {
		print("PASS: File write operations\n");
	}
	sys_close(fd);

	fd = sys_open(test_path, O_RDONLY);
	if(fd < 0) {
		print("FAIL: Could not open file for reading\n");
		failed = 1;
	} else {
		nread = sys_read(fd, buf, sizeof(buf)-1);
		if(nread <= 0) {
			print("FAIL: Read returned %ld\n", nread);
			failed = 1;
		} else {
			buf[nread] = '\0';
			print("PASS: File read operations (read '%s')\n", buf);
		}
		sys_close(fd);
	}

	if(sys_stat(test_path, &st) != 0) {
		print("FAIL: stat failed\n");
		failed = 1;
	} else {
		print("PASS: stat (size=%lld)\n", st.length);
	}

	sys_unlink(test_path);

	if(!failed)
		print("\nAll file system tests passed!\n");

	exits(failed ? "fail" : nil);
}
