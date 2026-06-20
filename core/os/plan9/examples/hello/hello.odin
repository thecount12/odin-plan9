/*
 * Minimal Odin program for Plan 9 codegen bootstrap.
 */
package main

foreign {
	sys_write  :: proc(fd: int, buf: ^u8, count: int) -> int ---
	sys_strlen :: proc(s: cstring) -> int ---
}

write_cstr :: proc(s: cstring) {
	sys_write(1, cast(^u8)s, sys_strlen(s))
}

main :: proc() {
	hello: cstring = "hello from odin plan9\n"
	extra: cstring = "!\n"
	n: int = sys_strlen(hello)

	if n > 0 {
		write_cstr(hello)
	}

	for i := 0; i < 1; i += 1 {
		write_cstr(extra)
	}
}
