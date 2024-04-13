#pragma once

#include <sys/select.h>

#define NUMCONNTYPES 4

enum ConnType {
	TCP, IRC, HTTP, FTP
};

const char *CONNTYPES[NUMCONNTYPES];

struct conn;

void delconn(int index);
struct conn *sfd_get_conn(int sfd);
int mkconn(enum ConnType type, char *hostname, char *port);
int get_conntype(int index);
int send_text(char *text);
fd_set get_readfds(void);
fd_set get_writefds(void);
void init_net();
void stop_net();
int mksfd(char *hostname, char *port);
void closeAllConns();
