#+private
package os

_Platform_Error :: enum i32 {
	None = 0,
}

foreign {
	sys_err :: proc() -> i32 ---
}

@(private="file")
_plan9_err_strings := [?]cstring{
	"ok",
	"permission denied",
	"no such file or directory",
	"",
	"",
	"io error",
}

_error_string :: proc(errno: i32) -> string {
	if errno == 0 {
		return ""
	}
	if errno >= 0 && errno < len(_plan9_err_strings) {
		if s := _plan9_err_strings[errno]; s != nil {
			return string(s)
		}
	}
	return "plan9 error"
}

_get_platform_error_from_errno :: proc() -> Error {
	return _get_platform_error_existing(sys_err())
}

_get_platform_error_existing :: proc(errno: i32) -> Error {
	switch errno {
	case 0:
		return nil
	case 1:
		return .Permission_Denied
	case 2:
		return .Not_Exist
	case 5:
		return .Unknown
	case:
		return Platform_Error(Platform_Error(errno))
	}
}

_get_platform_error :: proc {
	_get_platform_error_existing,
	_get_platform_error_from_errno,
}
