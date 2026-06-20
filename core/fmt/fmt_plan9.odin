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

_print_any :: proc(v: any) -> int {
	if v.id == typeid_of(string) {
		return _write_string((cast(^string)v.data)^)
	}
	return 0
}

println :: proc(args: ..any) -> int {
	n := 0
	for _, i in args {
		if i > 0 {
			n += _write_string(" ")
		}
		n += _print_any(args[i])
	}
	n += _write_string("\n")
	return n
}

print :: proc(args: ..any) -> int {
	n := 0
	for _, i in args {
		if i > 0 {
			n += _write_string(" ")
		}
		n += _print_any(args[i])
	}
	return n
}
