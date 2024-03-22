#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: tclient hostname port\n");
		return -1;
	}

	printf("Hostname: %s, Port: %s\n", argv[1], argv[2]);
	return 0;
}
