#include "common.h"
#include "sysdeps.h"
#include "mmap.h"

void
main(void)
{
	void *mem;
	int *word;
	int failed;

	failed = 0;

	mem = sys_mmap(nil, 4096,
		ODIN_PROT_READ | ODIN_PROT_WRITE,
		ODIN_MAP_PRIVATE | ODIN_MAP_ANONYMOUS,
		-1, 0);
	if(mem == nil) {
		print("FAIL: mmap\n");
		exits("fail");
	}

	word = (int *)mem;
	*word = 0x12345678;
	if(*word != 0x12345678) {
		print("FAIL: mmap read/write\n");
		failed = 1;
	} else {
		print("PASS: anonymous mmap read/write\n");
	}

	if(sys_mprotect(mem, 4096, ODIN_PROT_READ) != 0) {
		print("FAIL: mprotect\n");
		failed = 1;
	} else {
		print("PASS: mprotect (read-only)\n");
	}

	if(sys_munmap(mem, 4096) != 0) {
		print("FAIL: munmap\n");
		failed = 1;
	} else {
		print("PASS: munmap\n");
	}

	if(!failed)
		print("\nAll mmap tests passed!\n");

	exits(failed ? "fail" : nil);
}
