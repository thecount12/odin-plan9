#+private
package sync

// Plan 9 user programs are single-threaded for now.
_current_thread_id :: proc "contextless" () -> int {
	return 0
}
