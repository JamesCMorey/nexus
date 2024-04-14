#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"

FILE *logfd;

void init_log(char *filename)
{
	char *mode = "a+";
	logfd = fopen(filename, mode);
	wlog("-------------------------------------------------------------");
}

void stop_log()
{
	wlog("-------------------------------------------------------------");
	fclose(logfd);
}
void wlog(char *text, ...)
{
	va_list args;

	fprintf(logfd, "%s: ", timestamp());

	va_start(args, text);
	vfprintf(logfd, text, args);
	va_end(args);

	fprintf(logfd, "\r\n");

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
