#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

#include "display.h"
#include "net.h"

//# define SET_FLAG(x, arr) x | arr
// # define ISSET(x, arr)

int main(int argc, char *argv[])
{
	char buffer[1024] = "";
	struct fds fds = {0};

	struct winfo wins;

	wins = init_display();

	FD_SET(fileno(stdin), &fds.master);
	fd_set tmp;
	while(1) {
		tmp = fds.master;

		if (select(1, &tmp, 0, 0, 0) < 0) {
			fprintf( stderr, "select error %d\n", errno);
			return -1;
		}

		if (handle_input(wins, &fds, buffer) < 0) {
			break;
		}

	}

	stop_display(wins);
	return 0;
}
