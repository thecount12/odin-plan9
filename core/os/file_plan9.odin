#+private
package os

import "base:runtime"

import "core:io"
import "core:sync"
import "core:time"

foreign {
	sys_open   :: proc(path: cstring, omode: int) -> int ---
	sys_read   :: proc(fd: int, buf: rawptr, count: int) -> int ---
	sys_write  :: proc(fd: int, buf: rawptr, count: int) -> int ---
	sys_close  :: proc(fd: int) -> int ---
	sys_seek   :: proc(fd: int, offset: uint, whence: int) -> uint ---
	sys_unlink :: proc(path: cstring) -> int ---
	sys_rename :: proc(oldpath, newpath: cstring) -> int ---
}

ODIN_O_RDONLY :: 0x000
ODIN_O_WRONLY :: 0x001
ODIN_O_RDWR   :: 0x002
ODIN_O_CREATE :: 0x040
ODIN_O_TRUNC  :: 0x200
ODIN_O_EXCL   :: 0x080

MAX_RW :: 1 << 30

File_Impl :: struct {
	file:      File,
	name:      string,
	fd:        int,
	allocator: runtime.Allocator,
	rw_mutex:  sync.RW_Mutex,
}

@(private="file")
_to_sys_omode :: proc(flags: File_Flags) -> int {
	omode: int
	switch flags & (O_RDONLY|O_WRONLY|O_RDWR) {
	case O_RDONLY:
	case O_WRONLY: omode |= ODIN_O_WRONLY
	case O_RDWR:   omode |= ODIN_O_RDWR
	}
	if .Create in flags { omode |= ODIN_O_CREATE }
	if .Trunc  in flags { omode |= ODIN_O_TRUNC }
	if .Excl   in flags { omode |= ODIN_O_EXCL }
	return omode
}

@(init)
init_std_files :: proc "contextless" () {
	new_std :: proc "contextless" (impl: ^File_Impl, fd: int, name: cstring) -> ^File {
		impl.file.impl = impl
		impl.fd = fd
		impl.allocator = runtime.nil_allocator()
		impl.name = string(name)
		impl.file.stream = {
			data = impl,
			procedure = _file_stream_proc,
		}
		return &impl.file
	}

	@(static) files: [3]File_Impl
	stdin  = new_std(&files[0], 0, "/dev/stdin")
	stdout = new_std(&files[1], 1, "/dev/stdout")
	stderr = new_std(&files[2], 2, "/dev/stderr")
}

_open :: proc(name: string, flags: File_Flags, perm: Permissions) -> (f: ^File, err: Error) {
	if name == "" {
		return nil, .Invalid_Path
	}

	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	cname := clone_to_cstring(name, temp_allocator) or_return

	fd := sys_open(cname, _to_sys_omode(flags))
	if fd < 0 {
		return nil, _get_platform_error()
	}

	return _new_file(uintptr(fd), name, file_allocator())
}

_new_file :: proc(handle: uintptr, name: string, allocator: runtime.Allocator) -> (f: ^File, err: Error) {
	if name == "" {
		err = .Invalid_Path
		return
	}

	impl := new(File_Impl, allocator) or_return
	defer if err != nil { free(impl, allocator) }

	impl.allocator = allocator
	impl.name = clone_string(name, allocator) or_return
	impl.fd = int(handle)
	impl.file.impl = impl
	impl.file.stream = {
		data = impl,
		procedure = _file_stream_proc,
	}

	return &impl.file, nil
}

_clone :: proc(f: ^File) -> (clone: ^File, err: Error) {
	if f == nil || f.impl == nil {
		return
	}
	return _new_file(uintptr((^File_Impl)(f.impl).fd), name(f), file_allocator())
}

_close :: proc(f: ^File_Impl) -> (err: Error) {
	if sys_close(f.fd) != 0 {
		err = _get_platform_error()
	}
	delete(f.name, f.allocator)
	free(f, f.allocator)
	return
}

_fd :: proc(f: ^File) -> uintptr {
	return uintptr(__fd(f))
}

__fd :: proc(f: ^File) -> int {
	if f != nil && f.impl != nil {
		return (^File_Impl)(f.impl).fd
	}
	return -1
}

_is_tty :: proc "contextless" (f: ^File) -> bool {
	return false
}

_name :: proc(f: ^File) -> string {
	if f != nil && f.impl != nil {
		return (^File_Impl)(f.impl).name
	}
	return ""
}

_sync :: proc(f: ^File) -> Error {
	_ = f
	return nil
}

_truncate :: proc(f: ^File, size: i64) -> Error {
	_ = f
	_ = size
	return .Unsupported
}

_remove :: proc(name: string) -> Error {
	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	cname := clone_to_cstring(name, temp_allocator) or_return
	if sys_unlink(cname) != 0 {
		return _get_platform_error()
	}
	return nil
}

_rename :: proc(old_path, new_path: string) -> Error {
	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	cold := clone_to_cstring(old_path, temp_allocator) or_return
	cnew := clone_to_cstring(new_path, temp_allocator) or_return
	if sys_rename(cold, cnew) != 0 {
		return _get_platform_error()
	}
	return nil
}

_link :: proc(old_name, new_name: string) -> Error {
	_ = old_name
	_ = new_name
	return .Unsupported
}

_symlink :: proc(old_name, new_name: string) -> Error {
	_ = old_name
	_ = new_name
	return .Unsupported
}

_read_link :: proc(name: string, allocator: runtime.Allocator) -> (s: string, err: Error) {
	_ = name
	_ = allocator
	return "", .Unsupported
}

_chdir :: proc(name: string) -> Error {
	return _set_working_directory(name)
}

_fchdir :: proc(f: ^File) -> Error {
	if f == nil || f.impl == nil {
		return .Invalid_File
	}
	return _set_working_directory(name(f))
}

_fchmod :: proc(f: ^File, mode: Permissions) -> Error {
	_ = f
	_ = mode
	return .Unsupported
}

_chmod :: proc(name: string, mode: Permissions) -> Error {
	_ = name
	_ = mode
	return .Unsupported
}

_fchown :: proc(f: ^File, uid, gid: int) -> Error {
	_ = f
	_ = uid
	_ = gid
	return .Unsupported
}

_chown :: proc(name: string, uid, gid: int) -> Error {
	_ = name
	_ = uid
	_ = gid
	return .Unsupported
}

_lchown :: proc(name: string, uid, gid: int) -> Error {
	_ = name
	_ = uid
	_ = gid
	return .Unsupported
}

_chtimes :: proc(name: string, atime, mtime: time.Time) -> Error {
	_ = name
	_ = atime
	_ = mtime
	return .Unsupported
}

_fchtimes :: proc(f: ^File, atime, mtime: time.Time) -> Error {
	_ = f
	_ = atime
	_ = mtime
	return .Unsupported
}

_exists :: proc(path: string) -> bool {
	_, err := _stat(path, file_allocator())
	return err == nil
}

_file_stream_proc :: proc(stream_data: rawptr, mode: File_Stream_Mode, p: []byte, offset: i64, whence: io.Seek_From, allocator: runtime.Allocator) -> (n: i64, err: Error) {
	f := (^File_Impl)(stream_data)
	fd := f.fd

	switch mode {
	case .Read:
		if len(p) <= 0 {
			return
		}
		to_read := min(len(p), MAX_RW)
		_n := sys_read(fd, raw_data(p[:to_read]), to_read)
		if _n < 0 {
			return 0, _get_platform_error()
		}
		n = i64(_n)
		if n == 0 {
			err = .EOF
		}
		return

	case .Read_At:
		if len(p) <= 0 {
			return
		}
		if offset < 0 {
			return 0, .Invalid_Offset
		}
		cur := sys_seek(fd, 0, 1)
		if int(cur) < 0 {
			return 0, _get_platform_error()
		}
		defer sys_seek(fd, cur, 0)

		if int(sys_seek(fd, uint(offset), 0)) < 0 {
			return 0, _get_platform_error()
		}
		to_read := min(len(p), MAX_RW)
		_n := sys_read(fd, raw_data(p[:to_read]), to_read)
		if _n < 0 {
			return 0, _get_platform_error()
		}
		n = i64(_n)
		if n == 0 {
			err = .EOF
		}
		return

	case .Write:
		if len(p) <= 0 {
			return
		}
		to_write := min(len(p), MAX_RW)
		_n := sys_write(fd, raw_data(p[:to_write]), to_write)
		if _n < 0 {
			return 0, _get_platform_error()
		}
		return i64(_n), nil

	case .Write_At:
		if len(p) <= 0 {
			return
		}
		if offset < 0 {
			return 0, .Invalid_Offset
		}
		cur := sys_seek(fd, 0, 1)
		if int(cur) < 0 {
			return 0, _get_platform_error()
		}
		defer sys_seek(fd, cur, 0)

		if int(sys_seek(fd, uint(offset), 0)) < 0 {
			return 0, _get_platform_error()
		}
		to_write := min(len(p), MAX_RW)
		_n := sys_write(fd, raw_data(p[:to_write]), to_write)
		if _n < 0 {
			return 0, _get_platform_error()
		}
		return i64(_n), nil

	case .Flush:
		return 0, nil

	case .Seek:
		w: int
		switch whence {
		case .Start:   w = 0
		case .Current: w = 1
		case .End:     w = 2
		}
		pos := sys_seek(fd, uint(offset), w)
		if int(pos) < 0 {
			return 0, _get_platform_error()
		}
		return i64(pos), nil

	case .Size:
		cur := sys_seek(fd, 0, 1)
		if int(cur) < 0 {
			return 0, _get_platform_error()
		}
		end := sys_seek(fd, 0, 2)
		if int(end) < 0 {
			return 0, _get_platform_error()
		}
		sys_seek(fd, cur, 0)
		return i64(end), nil

	case .Close, .Destroy:
		return 0, _close(f)

	case .Fstat:
		return 0, file_stream_fstat_utility(f, p, allocator)

	case .Query:
		return 0, nil
	}

	return 0, .Unsupported
}
