/*
 * Minimal Odin program for Plan 9 codegen bootstrap.
 * cgen hello emits C equivalent; compiler backend will target this next.
 */
package main

// Target shape (not compiled by odin build on Plan 9 yet):
//
//   odin_main(argc, argv) -> int
//   sys_write(1, "hello from odin plan9\n", ...)

main :: proc() {
	// Spec only — see hello.c and `cgen hello` output.
}
