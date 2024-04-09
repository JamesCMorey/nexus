#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

#include "display.h"
#include "net.h"


int main(int argc, char *argv[])
{
	init_display();
	/*
	while(1) {
		tmp = getReadFDs();

		if (select(1, &tmp, 0, 0, 0) < 0) {
			fprintf( stderr, "select error %d\n", errno);
			return -1;
		}

		if (handle_input() < 0) {
			break;
		}

	}

	*/
	stop_display();
	return 0;
}
