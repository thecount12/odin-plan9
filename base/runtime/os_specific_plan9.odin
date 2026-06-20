#+build plan9
#+private
package runtime

/*
 * Plan 9 runtime OS glue — maps to sys_* in libodin_plan9.a (odin_generated.h).
 */

foreign {
	sys_write :: proc(fd: int, buf: ^u8, count: int) -> int ---
	sys_exit  :: proc(status: int) ---
}

_HAS_RAND_BYTES :: false

_stderr_write :: proc "contextless" (data: []byte) -> (int, _OS_Errno) {
	if len(data) == 0 {
		return 0, 0
	}
	ret := sys_write(2, cast(^u8)raw_data(data), len(data))
	if ret < 0 {
		return 0, _OS_Errno(-ret)
	}
	return ret, 0
}

_exit :: proc "contextless" (code: int) -> ! {
	sys_exit(code)
	unreachable()
}
