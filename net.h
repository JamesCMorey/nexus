#pragma once

#include <sys/select.h>

#define NUMCONNTYPES 4

enum ConnType {
	TCP, IRC, HTTP, FTP
};

extern const char *CONNTYPES[NUMCONNTYPES];

struct conn;

int recv_text();
int read_conn(int index);
int get_maxsfd(void);
int sfd_get_conn_index(int sfd);
void switch_conn(int index);
void delconn(int index);
int mkconn(enum ConnType type, char *hostname, char *port);
int get_conntype(int index);
int send_text(char *text);
fd_set get_readfds(void);
fd_set get_writefds(void);
void init_net();
void stop_net();
int mksfd(char *hostname, char *port);
void closeAllConns();
