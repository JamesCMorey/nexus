#include <unistd.h>
#include <sys/select.h>
#include "display.h"
#include "net.h"

int main(int argc, char *argv[])
{
	char buffer[1024] = "";
	struct winfo wins;

	wins = init_display();

	while(1) {
		if (handle_input(wins, buffer) < 0) {
			break;
		}

	}

	stop_display(wins);
	return 0;
}
