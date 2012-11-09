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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define HTTP_200 "HTTP/1.0 200 OK\r\nContent-type: text/plain\r\nContent-length: %d\r\n\r\n"
char *test(void){
	char *a = malloc(sizeof(char)*6);
	a = "hello";
	return a;
}
int main(void) {
	
	char *b = test();
	char *c = strdup(b);
	printf("%s\n",b);
	free(b); free(c);
return 0;
	
}
