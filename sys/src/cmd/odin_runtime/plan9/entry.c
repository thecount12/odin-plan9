/*
 * Program entry for Odin Plan 9 targets.
 */
#include "common.h"

int odin_main(int argc, char **argv);

void
main(int argc, char **argv)
{
	int status;

	status = odin_main(argc, argv);
	if(status != 0)
		exits("fail");
	exits(nil);
}
