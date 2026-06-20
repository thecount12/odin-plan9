#+build plan9
package fmt

foreign {
	@(link_name="sys_write")
	sys_write :: proc(fd: int, buf: ^u8, count: int) -> int ---
}

_write_string :: proc(s: string) -> int {
	if len(s) == 0 {
		return 0
	}
	return sys_write(1, cast(^u8)raw_data(s), len(s))
}

println :: proc(s: string) -> int {
	n := _write_string(s)
	n += _write_string("\n")
	return n
}

print :: proc(s: string) -> int {
	return _write_string(s)
}
