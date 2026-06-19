/*
 * Minimal Odin program for Plan 9 codegen bootstrap.
 */
package main

foreign {
	sys_write  :: proc(fd: int, buf: ^u8, count: int) -> int ---
	sys_strlen :: proc(s: cstring) -> int ---
}

main :: proc() {
	msg: cstring = "hello from odin plan9\n"
	sys_write(1, cast(^u8)msg, sys_strlen(msg))
}
