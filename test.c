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
#include <time.h>
#define HTTP_200 "HTTP/1.0 200 OK\r\nContent-type: text/plain\r\nContent-length: %d\r\n\r\n"


int main(void) {
	char *b;
	time_t times;
	struct tm *newtime;
	times = time(&times);

	printf("%d\n",&times);
	newtime = localtime(&times);
	b = asctime(newtime);
	b[strlen(b)-1] = '\0';
	printf("\"%s\"",b);
	return 0;
}
