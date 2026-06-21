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

_int_to_buf :: proc(n: int, buf: ^[32]u8) -> int {
	if n == 0 {
		buf[0] = '0'
		return 1
	}

	tmp: [32]u8
	negative := n < 0
	v := n
	if negative {
		v = -v
	}

	i := 31
	for v > 0 {
		tmp[i] = u8('0' + (v % 10))
		v /= 10
		i -= 1
	}

	digit_start := i + 1
	digit_count := 32 - digit_start
	out := 0

	if negative {
		buf[0] = '-'
		out = 1
	}

	for j := 0; j < digit_count; j += 1 {
		buf[out] = tmp[digit_start + j]
		out += 1
	}

	return out
}

_write_int :: proc(n: int) -> int {
	buf: [32]u8
	count := _int_to_buf(n, &buf)
	return sys_write(1, cast(^u8)raw_data(buf[0:count]), count)
}

_print_any :: proc(v: any) -> int {
	if v.id == typeid_of(string) {
		return _write_string((cast(^string)v.data)^)
	}
	if v.id == typeid_of(int) {
		return _write_int((cast(^int)v.data)^)
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
