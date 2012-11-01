#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "network.h"


// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t jobready = PTHREAD_COND_INITIALIZER;
pthread_cond_t jobdone = PTHREAD_COND_INITIALIZER;

void signal_handler(int sig) {
    still_running = FALSE;
}


void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

void runserver(int numthreads, unsigned short serverport) {
	pthread_t *cid;
    // create your pool of threads here
	int i = 0;
	for(;i<numthreads;i++) {
		pthread_create(cid[i], NULL, consumer, NULL);
	}
	

    
    int main_socket = prepare_server_socket(serverport);
    if (main_socket < 0) {
        exit(-1);
    }
    signal(SIGINT, signal_handler);

    struct sockaddr_in client_address;
    socklen_t addr_len;

    fprintf(stderr, "Server listening on port %d.  Going into request loop.\n", serverport);
    while (still_running) {
        struct pollfd pfd = {main_socket, POLLIN};
        int prv = poll(&pfd, 1, 10000);

        if (prv == 0) {
            continue;
        } else if (prv < 0) {
            PRINT_ERROR("poll");
            still_running = FALSE;
            continue;
        }
        
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);

        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
            
            fprintf(stderr, "Got connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

           ////////////////////////////////////////////////////////
           /* You got a new connection.  Hand the connection off
            * to one of the threads in the pool to process the
            * request.
            *
            * Don't forget to close the socket (in the worker thread)
            * when you're done.
            */
           ////////////////////////////////////////////////////////


        }
    }
    fprintf(stderr, "Server shutting down.\n");
        
    close(main_socket);
}


void log(struct sockaddr_in *client_address, char *filename, char *response_status, char *client_response) {
	char* log_response, *date;
	int totsize = sizeof(char)*client_response;
	time_t time;
	time = time(&time);
	struct tm *timestamp;
	timestamp = localtime(&time);
	date = asctime(timestamp);
	date[strlen(date)-1] = '\0'; //get rid of extra return symbol
	sprintf(log_response, "%s:%d %s \"GET %s\" %s %d\n", inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port),filename,response_status,totsize);
	//MAYBE I NEED TO PARSE THE FILE EVERY TIME TO APPEND INSTEAD OF OVERWRITE?
	int fd = open("weblog.txt");
	int write_status = write(fd, log_response, sizeof(char) * strlen(log_response));
	int close_status = close(fd);
}

char* createresponse(char *status, char *filepath, int filesize, struct sockaddr_in *client_address) {
	char *client_response, *fileread, *HTTP;
	int fd = open(filepath), sizeread = 0;
	sizeread = read(fd, fileread, (size_t) filesize);
	close(fd);
	if(!strcmp(status, "404")) {
		sprintf(HTTP,HTTP_404,sizeread);
	} else {
		sprintf(HTTP,HTTP_200,sizeread);
	}

	sprintf(client_response,"%s%s\n\0",HTTP,fileread);
	client_response = strdup(client_response);
	return client_response;
}


char *handlerequest(char *address, struct sockaddr_in *client_address) {
	if(address == NULL) { 
		return NULL;
	}
	char cwd[1024], filepath[2048], *response, *client_response;
	struct stat filestat;
	int size = 0;
	getcwd(cwd,sizeof(cwd));
	if(address[0] != '/') {
		sprintf(filepath, "%s/%s",cwd,address);
	} else {
		sprintf(filepath, "%s%s",cwd,address);
	}

	if(stat(&filepath, &filestat) == 0) {
		response = "404";
	} else {
		response = "200";
		size = (int) filestat.st_size;
	}
	client_response = strdup(createresponse(response, filepath, size, client_address));
	log(client_address, address, response, client_response);
	
	
	

void *consumer(void *arg) {
	while(still_running) {
		pthread_mutex_lock(&mutex);
		while(numjobs == 0) {
			cond_wait(&jobready,&mutex);
		}
		char filename[1024];
		int request = getrequest(new_sock, filename, 1024);
		if(request != 0) {
			fprintf(stderr, "Could not process request.\n");
			continue;
		}
		char *response = handlerequest(filename, &client_address);


int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;

    int c;
    while (-1 != (c = getopt(argc, argv, "hp:"))) {
        switch(c) {
            case 'p':
                port = atoi(optarg);
                if (port < 1024) {
                    usage(argv[0]);
                }
                break;

            case 't':
                num_threads = atoi(optarg);
                if (num_threads < 1) {
                    usage(argv[0]);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }

    runserver(num_threads, port);
    
    fprintf(stderr, "Server done.\n");
    exit(0);
}
