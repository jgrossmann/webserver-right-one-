#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>



int prepare_server_socket(unsigned short);
int senddata(int, const char *, int);
int getrequest(int, char *, int);

struct socks {
	struct sock *socklist_head;
	pthread_mutex_t *joblist_lock;
	int numjobs;
};

struct sock {
	int socket;
	struct sock *next;
	struct sock *previous;
	struct sockaddr_in address;
	char status;
};

#define HTTP_404 "HTTP/1.0 404 Not found\r\n\r\n"
#define HTTP_200 "HTTP/1.0 200 OK\r\nContent-type: text/plain\r\nContent-length: %d\r\n\r\n"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define PRINT_ERROR(thecall)  perror(thecall)

#ifndef MIN
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#endif // __NETWORK_H__
