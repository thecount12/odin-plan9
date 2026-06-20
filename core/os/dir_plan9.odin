#+private
package os

import "base:runtime"

foreign {
	sys_opendir   :: proc(path: cstring) -> Sys_Dir ---
	sys_readdir   :: proc(dir: Sys_Dir, ent: ^Dir_Ent) -> int ---
	sys_closedir  :: proc(dir: Sys_Dir) -> int ---
}

Sys_Dir :: distinct rawptr

Dir_Ent :: struct {
	name:   [256]u8,
	is_dir: int,
}

Read_Directory_Iterator_Impl :: struct {
	dir:      Sys_Dir,
	fullpath: [dynamic]byte,
}

@(require_results)
_read_directory_iterator :: proc(it: ^Read_Directory_Iterator) -> (fi: File_Info, index: int, ok: bool) {
	fimpl := (^File_Impl)(it.f.impl)

	index = it.index
	it.index += 1

	for {
		ent: Dir_Ent
		res := sys_readdir(it.impl.dir, &ent)
		if res == 0 {
			return
		}
		if res < 0 {
			read_directory_iterator_set_error(it, name(it.f), _get_platform_error())
			return
		}

		cname := cstring(raw_data(ent.name[:]))
		if cname == "." || cname == ".." {
			continue
		}
		sname := string(cname)

		n := len(fimpl.name)+1
		if err := non_zero_resize(&it.impl.fullpath, n+len(sname)); err != nil {
			read_directory_iterator_set_error(it, sname, err)
			ok = true
			return
		}
		copy(it.impl.fullpath[n:], sname)

		fullpath := string(it.impl.fullpath[:])
		fi, err := _stat(fullpath, file_allocator())
		if err != nil {
			read_directory_iterator_set_error(it, fullpath, err)
			ok = true
			return
		}

		ok = true
		return
	}
}

_read_directory_iterator_init :: proc(it: ^Read_Directory_Iterator, f: ^File) {
	if f == nil || f.impl == nil {
		read_directory_iterator_set_error(it, "", .Invalid_File)
		return
	}

	impl := (^File_Impl)(f.impl)

	it.impl.fullpath.allocator = file_allocator()
	clear(&it.impl.fullpath)
	if err := reserve(&it.impl.fullpath, len(impl.name)+128); err != nil {
		read_directory_iterator_set_error(it, name(f), err)
		return
	}

	append(&it.impl.fullpath, impl.name)
	append(&it.impl.fullpath, "/")

	if it.impl.dir != nil {
		sys_closedir(it.impl.dir)
		it.impl.dir = nil
	}

	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	cpath, cerr := clone_to_cstring(impl.name, temp_allocator)
	if cerr != nil {
		read_directory_iterator_set_error(it, name(f), cerr)
		return
	}

	it.impl.dir = sys_opendir(cpath)
	if it.impl.dir == nil {
		read_directory_iterator_set_error(it, name(f), _get_platform_error())
		return
	}
}

_read_directory_iterator_destroy :: proc(it: ^Read_Directory_Iterator) {
	if it.impl.dir == nil {
		return
	}

	sys_closedir(it.impl.dir)
	it.impl.dir = nil
	delete(it.impl.fullpath)
}
