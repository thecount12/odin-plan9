#+private
package os

import "base:runtime"

import "core:time"

foreign {
	sys_stat :: proc(path: cstring, st: ^Sys_Stat) -> int ---
}

Sys_ULL :: struct {
	lo: u32,
	hi: u32,
}

Sys_Stat :: struct {
	type:   u32,
	dev:    u32,
	ino:    Sys_ULL,
	mode:   u32,
	nlink:  u32,
	uid:    u32,
	gid:    u32,
	rdev:   u32,
	length: Sys_ULL,
	atime:  int,
	mtime:  int,
	ctime:  int,
	name:   [256]u8,
}

ODIN_S_IFREG :: 0o100000
ODIN_S_IFDIR :: 0o040000

@(private="file")
_sys_ull_to_i64 :: proc(u: Sys_ULL) -> i64 {
	return i64(u.lo) | (i64(u.hi) << 32)
}

internal_stat :: proc(stat: Sys_Stat, fullpath: string) -> (fi: File_Info) {
	fi.fullpath = fullpath
	_, fi.name = split_path(fi.fullpath)

	fi.inode = u128(_sys_ull_to_i64(stat.ino))
	fi.size  = _sys_ull_to_i64(stat.length)
	fi.mode  = transmute(Permissions)u32(stat.mode)

	fi.type = .Undetermined
	switch stat.type {
	case ODIN_S_IFDIR:
		fi.type = .Directory
	case ODIN_S_IFREG:
		fi.type = .Regular
	}

	fi.creation_time     = time.Time{_nsec = i64(stat.ctime) * 1_000_000_000}
	fi.modification_time = time.Time{_nsec = i64(stat.mtime) * 1_000_000_000}
	fi.access_time       = time.Time{_nsec = i64(stat.atime) * 1_000_000_000}

	return
}

_fstat :: proc(f: ^File, allocator: runtime.Allocator) -> (fi: File_Info, err: Error) {
	if f == nil || f.impl == nil {
		err = .Invalid_File
		return
	}

	impl := (^File_Impl)(f.impl)
	return _stat(impl.name, allocator)
}

_stat :: proc(name: string, allocator: runtime.Allocator) -> (fi: File_Info, err: Error) {
	if name == "" {
		err = .Invalid_Path
		return
	}

	temp_allocator := TEMP_ALLOCATOR_GUARD({ allocator })
	cname := clone_to_cstring(name, temp_allocator) or_return

	st: Sys_Stat
	if sys_stat(cname, &st) != 0 {
		err = _get_platform_error()
		return
	}

	fullpath := clone_string(name, allocator) or_return
	return internal_stat(st, fullpath), nil
}

_lstat :: _stat

_same_file :: proc(fi1, fi2: File_Info) -> bool {
	return fi1.fullpath == fi2.fullpath
}

_is_reserved_name :: proc(path: string) -> bool {
	return false
}
