#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>
#include "display.h"
#include "net.h"
#include "log.h"

#define MSGSIZE 500

int main(int argc, char *argv[])
{
	init_log("z.log");
	init_net();
	init_screen();

	int maxsfd;
	fd_set readfds;
	int index;
	while(1) {
		maxsfd = get_maxsfd();
		readfds = get_readfds();

		if (select(maxsfd + 1, &readfds, 0, 0, 0) < 0) {
			display("select error %d\n", errno);
			return -1;
		}

		/* This abuses conditional short circuiting */
		if (FD_ISSET(fileno(stdin), &readfds) && handle_input() != 0) {
			break;
		}

		for (int i = 1; i <= maxsfd; i++) {
			if(FD_ISSET(i, &readfds)) {
				index = sfd_get_conn_index(i);

				if (index == -1) {
					continue;
				}

				read_conn(index);
			}
		}
	}

	stop_screen();
	stop_net();
	stop_log();
	system("rm z.log");
	return 0;
}
