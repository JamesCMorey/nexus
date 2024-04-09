#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "log.h"

enum type {
	NOARG, STR, INT
};

FILE *logfd;

void init_log(char *filename)
{
	char *mode = "a+";
	logfd = fopen(filename, mode);
}

void wlog(char *text)
{
	fprintf(logfd, "%s: ", timestamp());
	fprintf(logfd, "%s", text);
	fflush(logfd);
}

void walog(int type, char *text, void *arg)
{
	switch(type) {
	case NOARG:
		fprintf(logfd, "%s: ", timestamp());
		fprintf(logfd, "%s", text);
		break;

	case INT:
		fprintf(logfd, "%s: ", timestamp());
		fprintf(logfd, text, *(int *)arg);
		break;

	case STR:
		fprintf(logfd, "%s: ", timestamp());
		fprintf(logfd, text, arg);
		break;

	default:
		fprintf(logfd, "%s: ", timestamp());
		fprintf(logfd, "Invalid log type (%s:%d)", __FILE__, __LINE__);
	}

	fflush(logfd);
}

char *timestamp()
{
	char *ts;
	time_t t;
	struct tm *tminfo;

	ts = malloc(sizeof(char) * 9);
	time(&t);
	tminfo = localtime(&t);

	sprintf(ts, "%d:%d:%d", tminfo->tm_hour, tminfo->tm_min,
		tminfo->tm_sec);

	return ts;
}
