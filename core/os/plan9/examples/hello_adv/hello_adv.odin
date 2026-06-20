/*
 * Plan 9 codegen smoke test: structs, switch, helpers.
 */
package main

foreign {
	sys_write  :: proc(fd: int, buf: ^u8, count: int) -> int ---
	sys_strlen :: proc(s: cstring) -> int ---
}

Point :: struct {
	x: int,
	y: int,
}

write_cstr :: proc(s: cstring) {
	sys_write(1, cast(^u8)s, sys_strlen(s))
}

point_sum :: proc(p: Point) -> int {
	return p.x + p.y
}

main :: proc() {
	p := Point{x = 1, y = 2}
	n: int = 0

	switch point_sum(p) {
	case 0:
		n = 0
	case 3:
		n = 1
	case:
		n = -1
	}

	if n == 1 {
		write_cstr("hello adv plan9\n")
	}

	for i := 0; i < 2; i += 1 {
		write_cstr("!\n")
	}
}
