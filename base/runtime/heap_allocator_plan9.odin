#+build plan9
#+private
package runtime

foreign {
	sys_malloc  :: proc(size: int) -> rawptr ---
	sys_calloc  :: proc(nmemb, size: int) -> rawptr ---
	sys_realloc :: proc(ptr: rawptr, size: int) -> rawptr ---
	sys_free    :: proc(ptr: rawptr) ---
}

_heap_alloc :: proc "contextless" (size: int, zero_memory := true) -> rawptr {
	if size <= 0 {
		return nil
	}
	if zero_memory {
		return sys_calloc(1, size)
	}
	return sys_malloc(size)
}

_heap_resize :: proc "contextless" (ptr: rawptr, new_size: int) -> rawptr {
	return sys_realloc(ptr, new_size)
}

_heap_free :: proc "contextless" (ptr: rawptr) {
	sys_free(ptr)
}
