#+private
package os

import "base:runtime"

_temp_dir :: proc(allocator: runtime.Allocator) -> (string, runtime.Allocator_Error) {
	return clone_string("/tmp", allocator)
}
