#+private
package os

import "base:runtime"

foreign {
	sys_getcwd :: proc(buf: ^u8, size: int) -> int ---
	sys_chdir  :: proc(path: cstring) -> int ---
	sys_mkdir  :: proc(path: cstring, mode: u32) -> int ---
	sys_rmdir  :: proc(path: cstring) -> int ---
}

SYS_PATH_MAX :: 4096

_Path_Separator        :: '/'
_Path_Separator_String :: "/"
_Path_List_Separator   :: ':'

_is_path_separator :: proc(c: byte) -> bool {
	return c == _Path_Separator
}

_mkdir :: proc(name: string, perm: Permissions) -> Error {
	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	cname := clone_to_cstring(name, temp_allocator) or_return
	if sys_mkdir(cname, transmute(u32)perm) != 0 {
		return _get_platform_error()
	}
	return nil
}

_mkdir_all :: proc(path: string, perm: Permissions) -> Error {
	if path == "" {
		return .Invalid_Path
	}

	temp_allocator := TEMP_ALLOCATOR_GUARD({})

	if exists(path) {
		return .Exist
	}

	clean_path := clean_path(path, temp_allocator) or_return
	return internal_mkdir_all(clean_path, perm)

	internal_mkdir_all :: proc(path: string, perm: Permissions) -> Error {
		dir, file := split_path(path)
		if file != path && dir != "/" {
			if len(dir) > 1 && dir[len(dir) - 1] == '/' {
				dir = dir[:len(dir) - 1]
			}
			internal_mkdir_all(dir, perm) or_return
		}

		err := _mkdir(path, perm)
		if err == .Exist { err = nil }
		return err
	}
}

_remove_all :: proc(path: string) -> (err: Error) {
	dir := open(path) or_return
	defer close(dir)

	iter := read_directory_iterator_create(dir)
	defer read_directory_iterator_destroy(&iter)

	for fi in read_directory_iterator(&iter) {
		_ = read_directory_iterator_error(&iter) or_break

		if fi.type == .Directory {
			_remove_all(fi.fullpath) or_return
		} else {
			remove(fi.fullpath) or_return
		}
	}

	_ = read_directory_iterator_error(&iter) or_return
	return remove(path)
}

_get_working_directory :: proc(allocator: runtime.Allocator) -> (dir: string, err: Error) {
	temp_allocator := TEMP_ALLOCATOR_GUARD({ allocator })

	buf: [dynamic]u8
	buf.allocator = temp_allocator
	resize(&buf, SYS_PATH_MAX) or_return

	if sys_getcwd(raw_data(buf), len(buf)) != 0 {
		return "", _get_platform_error()
	}

	return clone_string(string(cstring(raw_data(buf))), allocator)
}

_set_working_directory :: proc(dir: string) -> (err: Error) {
	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	cdir := clone_to_cstring(dir, temp_allocator) or_return
	if sys_chdir(cdir) != 0 {
		return _get_platform_error()
	}
	return nil
}

_get_executable_path :: proc(allocator: runtime.Allocator) -> (path: string, err: Error) {
	if len(args) <= 0 {
		return clone_string("/", allocator)
	}
	return clone_string(args[0], allocator)
}

_get_absolute_path :: proc(path: string, allocator: runtime.Allocator) -> (absolute_path: string, err: Error) {
	if path == "" {
		return "", .Invalid_Path
	}
	if _is_absolute_path(path) {
		return clone_string(path, allocator)
	}
	wd, e := _get_working_directory(allocator)
	if e != nil {
		return "", e
	}
	defer delete(wd, allocator)
	return concatenate({wd, _Path_Separator_String, path}, allocator)
}
