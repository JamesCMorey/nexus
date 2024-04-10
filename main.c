#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include "display.h"
#include "net.h"
#include "log.h"

int main(int argc, char *argv[])
{
	init_log("z");
	init_net();
	init_screen();

	while(1) {

		/*
		if (select(1, &tmp, 0, 0, 0) < 0) {
			fprintf( stderr, "select error %d\n", errno);
			return -1;
		}
		*/
		if (handle_input() < 0) {
			break;
		}

	}

	stop_screen();
	stop_net();
	stop_log();
	return 0;
}
