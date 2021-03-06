/* John Grossmann Project-03 Webserver
the first request yields some uninitialized variable messages from valgrind
yet the program works just fine. Could not pinpoint the source of the error and 
I think its just superficial error messaging. */

#include "network.h"


// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //lock for threads before condition variable.
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER; //lock on the log function which edits weblog.txt
pthread_cond_t jobready = PTHREAD_COND_INITIALIZER; //when signaled by producer, 1 thread takes new socket.

void signal_handler(int sig) {
    still_running = FALSE;
}


void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

void free_socklist(struct socks *socklist) { //frees remaining variables and structs when server is ending
	pthread_mutex_lock(socklist->joblist_lock);
	struct sock *cursock = socklist->socklist_head;
	while(cursock->next != NULL) {
		cursock = cursock->next;
	}
	while(cursock->previous != NULL) {
		cursock = cursock->previous;
		free(cursock->next);
	}
	free(cursock->next); free(cursock);
	pthread_mutex_unlock(socklist->joblist_lock);
	pthread_mutex_destroy(socklist->joblist_lock);
	free(socklist->joblist_lock);
	free(socklist);
}
		
// function which writes log entry to weblog.txt
void Log(struct sockaddr_in client_address, char *filename, char *response_status, char *client_response) {
	char *date, cwd[1024], writefile[2048];
	struct stat status;
	char *log_response = malloc(sizeof(char)*1024);
	int totsize = sizeof(char)*strlen(client_response);
	time_t timer;
	time(&timer);
	struct tm *timestamp;
	timestamp = localtime(&timer);
	date = asctime(timestamp);//formatted time string
	date[strlen(date)-1] = '\0'; //get rid of extra return symbol
	sprintf(log_response, "%s:%d %s \"GET %s\" %s %d\n", inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port),date,filename,response_status,totsize); //creates log entry string
	pthread_mutex_lock(&log_mutex);
	getcwd(cwd,sizeof(cwd)); //string for current working directory
	sprintf(writefile,"%s/weblog.txt", cwd);
	if(stat(writefile, &status)==0){ // real file
		FILE *fd = fopen(writefile, "a");
		if(fd == NULL) { //if could not open file
			printf("ERROR with log: could not write to file.");
		}
		fprintf(fd, "%s\n",log_response);
		//int write_status = write(fd, log_response, sizeof(char) * strlen(log_response));
		//int close_status = close(fd);
		fclose(fd);
	}else {// not a real file
		printf("ERROR with log: could not write to file.");
	}
	free(log_response);
	pthread_mutex_unlock(&log_mutex);
}

//creates response to GET request to sent back to requester
char* createresponse(char *status, char *filepath, int filesize, struct sockaddr_in client_address) {
	char *client_response, fileread[9224], HTTP[1024];
	client_response = calloc(10224,sizeof(char)); //idk valgrind was bitching about uninitialized variable and this seemed to calm one of the error messages down
	int fd, sizeread = 0, totsize = 0;
	if(!strcmp(status, "404")) {//if could not find file
		sprintf(HTTP,HTTP_404);
		sprintf(client_response,"%s",HTTP);
		totsize+=sizeof(char)*strlen(HTTP_404);
	} else {//if file was found
		fd = open(filepath,O_RDONLY); //open to read only
		sizeread = read(fd, fileread, (size_t) filesize);
		close(fd);
		totsize+=sizeread; 
		sprintf(HTTP,HTTP_200,sizeread);
		totsize+=sizeof(char)*strlen(HTTP_200);
		sprintf(client_response,"%s%s\n",HTTP,fileread);
	}
	client_response[totsize-1] = '\0'; //apparently sprintf does not add a null terminator
	char *ret = malloc(sizeof(char)*strlen(client_response)+1);//making a cleaner variable
	strncpy(ret,client_response, sizeof(char)*strlen(client_response));
	free(client_response);
	return ret;
}

//once request is recieved, this handles the analization and mangagement of the response and log.
void handlerequest(char *address, struct sock *cursock, struct sockaddr_in client_address) {
	if(address == NULL) { 
		return;
	}
	char cwd[1024], filepath[2048], *response = NULL, *client_response = NULL;
	struct stat filestat;
	int size = 0;
	getcwd(cwd,sizeof(cwd)); //current working directory
//creating full path to file request address
	if(address[0] != '/') {
		sprintf(filepath, "%s/%s",cwd,address);
	} else {
		sprintf(filepath, "%s%s",cwd,address);
	}
	if(stat(filepath, &filestat) != 0) { //if a real file
		response = "404";
	} else { //if not a real file
		response = "200";
		size = (int) filestat.st_size;
	}
	client_response = createresponse(response, filepath, size, client_address); //creates response to GET request
	size = (int) sizeof(char)*strlen(client_response);
	int senddata_ret = senddata(cursock->socket, client_response, size); //sends response made in createresponse
	Log(client_address, address, response, client_response);//logs interaction in weblog.txt
	free(client_response);
}	
	
struct sock *get_socket(struct socks *socklist) {// gets new socket from socket linked list
	pthread_mutex_lock(socklist->joblist_lock);
	struct sock *sock_head = socklist->socklist_head, *temp;
	while(sock_head->status == 'n') {//while socket is/was already used go to next
		sock_head = sock_head->next;
	}
	if(sock_head->previous == NULL && sock_head->status == 'y'){//if head of list and has job available
		temp = sock_head; temp->status = 'n';
	}else if(sock_head->next!=NULL && sock_head->status == 'y') {//if in middle of list and has job available
		temp = sock_head; temp->status = 'n'; temp->previous->next = temp->next;
		temp->next->previous = temp->previous;
	}else { //if at the end of the lest and has job available
		temp = sock_head; temp->status = 'n';
		temp->previous->next = NULL;
	}
	socklist->numjobs--; //decrement numjobs to ensure threads dont access list when there is no jobs
	pthread_mutex_unlock(socklist->joblist_lock);
	return temp;
}
		
void update_socketlist(struct sock *socket_struct, struct socks *socklist) { //slight housekeeping
	if(socket_struct->previous!=NULL){
		free(socket_struct);
	}
	return;
}


void *consumer(void *arg) {   //thread start function
	struct socks *socklist = (struct socks *) arg;
	while(still_running) { //signal that producer uses as basis to keep running
		pthread_mutex_lock(&mutex);
		while(socklist->numjobs == 0 || !still_running) { //thread wait loop
			if(!still_running) { //only happens when producer is ending and want threads to end
				pthread_mutex_unlock(&mutex);
				return NULL;
			}
			pthread_cond_wait(&jobready,&mutex); //wait for signal for job from producer
			if(!still_running) {//only happens when producer is ending and want threads to end
				pthread_mutex_unlock(&mutex);
				return NULL;
			}
		}
		pthread_mutex_unlock(&mutex);
		struct sock *socket_struct = get_socket(socklist); //get new socket from socket linked list
		char* filename = calloc(4096,sizeof(char));
		int request = getrequest(socket_struct->socket, filename, 1024); //get new request from socket
		if(request != 0) { //if request was bad
			fprintf(stderr, "Could not process request.\n");
			close(socket_struct->socket);
			update_socketlist(socket_struct,socklist);
			free(filename);
			continue;
		}
		handlerequest(filename, socket_struct, socket_struct->address); //sends request to be taken care of
		close(socket_struct->socket);
		update_socketlist(socket_struct,socklist);
		free(filename);
	}
	return NULL;
}

void runserver(int numthreads, unsigned short serverport) {
	
	pthread_t cid[numthreads];
	
	//initialize the socket list head.
	struct socks *socklist = malloc(sizeof(struct socks));
	socklist->joblist_lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(socklist->joblist_lock,NULL);
	socklist->socklist_head = malloc(sizeof(struct sock));
	socklist->socklist_head->socket= (int) NULL;
	socklist->socklist_head->next = NULL; socklist->socklist_head->previous = NULL;
	socklist->numjobs = 0; socklist->socklist_head->status = 'n';
    // create your pool of threads here
	int i = 0;
	for(;i<numthreads;i++) {
		pthread_create(&cid[i], NULL, consumer, (void *)socklist); //each thread with socket list passed in
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
        struct sock *sock_head = socklist->socklist_head;
		struct sock *newsocket;
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);
		if(sock_head->next != NULL) { //if there is more than 1 socket on the list
			newsocket = malloc(sizeof(struct sock));
			newsocket->socket = (int) NULL; newsocket->next = NULL; newsocket->previous = NULL;
			newsocket->status = 'n';
		}

        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
            struct sock *temp;
            fprintf(stderr, "Got connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
			pthread_mutex_lock(socklist->joblist_lock);
		// assigns new socket to socket list
			if(sock_head->next == NULL) { 
				sock_head->socket = new_sock;
				sock_head->address = client_address;
				sock_head->status = 'y';
			} else {
				temp = sock_head;
				while(temp->next !=NULL) {
					temp = temp->next;
				}
				temp->next = newsocket;
				newsocket->socket = new_sock;
				newsocket->address = client_address;
				newsocket->previous = temp;
				newsocket->status = 'y';
			}
			socklist->numjobs++; //ensures threads that pass condition variable have a job to recieve
			pthread_mutex_unlock(socklist->joblist_lock);
			pthread_cond_signal(&jobready); //signals to a thread that there is a new socket

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
	pthread_cond_broadcast(&jobready);
	i = 0;
	void *status;
	for(;i<numthreads;i++) {
		pthread_join(cid[i], &status); //collects dead threads
	}
	free_socklist(socklist);
	pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&log_mutex);  
    close(main_socket);
}
	

int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;

    int c;
    while (-1 != (c = getopt(argc, argv, "p:t:h"))) {
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
