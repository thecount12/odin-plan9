#+private
package os

import "base:runtime"

import "core:strings"

foreign {
	sys_getenv   :: proc(key: cstring) -> cstring ---
	sys_setenv   :: proc(key, value: cstring) -> int ---
	sys_unsetenv :: proc(key: cstring) -> int ---
}

_lookup_env_alloc :: proc(key: string, allocator: runtime.Allocator) -> (value: string, found: bool) {
	if key == "" {
		return
	}

	temp_allocator := TEMP_ALLOCATOR_GUARD({ allocator })

	ckey := strings.clone_to_cstring(key, temp_allocator)
	cval := sys_getenv(ckey)
	if cval == nil {
		return
	}

	found = true
	value = strings.clone(string(cval), allocator)
	return
}

_lookup_env_buf :: proc(buf: []u8, key: string) -> (value: string, error: Error) {
	if key == "" {
		return
	}

	if len(key) + 1 > len(buf) {
		return "", .Buffer_Full
	} else {
		copy(buf, key)
		buf[len(key)] = 0
	}

	cval := sys_getenv(cstring(raw_data(buf)))
	if cval == nil {
		return
	}

	if value = string(cval); value == "" {
		return "", .Env_Var_Not_Found
	} else {
		if len(value) > len(buf) {
			return "", .Buffer_Full
		} else {
			copy(buf, value)
			return string(buf[:len(value)]), nil
		}
	}
}

_lookup_env :: proc{_lookup_env_alloc, _lookup_env_buf}

_set_env :: proc(key, value: string) -> (err: Error) {
	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	ckey := clone_to_cstring(key, temp_allocator) or_return
	cval := clone_to_cstring(value, temp_allocator) or_return
	if sys_setenv(ckey, cval) != 0 {
		return _get_platform_error()
	}
	return nil
}

_unset_env :: proc(key: string) -> bool {
	temp_allocator := TEMP_ALLOCATOR_GUARD({})
	ckey, cerr := clone_to_cstring(key, temp_allocator)
	if cerr != nil {
		return false
	}
	return sys_unsetenv(ckey) == 0
}

_clear_env :: proc() {
	// Plan 9 has no bulk clear in libodin_plan9 yet.
}

_environ :: proc(allocator: runtime.Allocator) -> (environ: []string, err: Error) {
	err = .Unsupported
	return
}
