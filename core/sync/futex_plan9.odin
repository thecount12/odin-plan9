#+private
package sync

import "core:time"

// Single-threaded stub — enough for fmt and core:mem mutex allocators.
_futex_wait :: proc "contextless" (f: ^Futex, expected: u32) -> bool {
	_ = f
	_ = expected
	return true
}

_futex_wait_with_timeout :: proc "contextless" (f: ^Futex, expected: u32, duration: time.Duration) -> bool {
	_ = f
	_ = expected
	_ = duration
	return true
}

_futex_signal :: proc "contextless" (f: ^Futex) {
	_ = f
}

_futex_broadcast :: proc "contextless" (f: ^Futex) {
	_ = f
}
