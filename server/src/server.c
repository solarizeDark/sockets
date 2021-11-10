#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <pthread.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if.h>
#include <unistd.h>
#include <fcntl.h>

#include <poll.h>
#include <fcntl.h>

#include "../include/server.h"
#include "../../queue/include/queue.h"

#define PORT 		3490
#define NUM_CLIENTS 10		// also threads amount
#define BUF_SIZE 	200
#define LABEL_START '\n'

pthread_mutex_t mutex;

struct in_addr get_local_ip () {

	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;

	strncpy(ifr.ifr_name, "enp0s3", IFNAMSIZ - 1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	printf("%s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
	return ((struct sockaddr_in*) &ifr.ifr_addr) -> sin_addr;
}

// cur_size via ptr with purpose changing cur_size value only once inside this function
// flag will be setted to true if reallocation needed
pthread_t* malloc_threads(pthread_t *threads_p, bool flag, int *cur_size) {
	
	// initial allocation for NUM_CLIENTS
	threads_p = (pthread_t*) malloc(NUM_CLIENTS * sizeof (pthread_t));	

	if (flag) {
		threads_p = (pthread_t*) realloc(threads_p, *cur_size += NUM_CLIENTS);
		// threads_ptr moving to new block of data 
		threads_p += *cur_size - NUM_CLIENTS;  
	}

	return threads_p;
}

struct sockaddr_in * network_setting() {

	struct sockaddr_in *host = (struct sockaddr_in*) malloc (sizeof(struct sockaddr_in));
	  
	host->sin_family 		= AF_INET;
	host->sin_port 			= htons(PORT);
	// host->sin_addr.s_addr 	= get_local_ip().s_addr;
	host->sin_addr.s_addr 	= inet_addr("127.0.0.1");
	 
	memset(host->sin_zero, '\0', sizeof(host));

	return host;
}

void copy(char * f, char * s, int to_add) {

	char *tf, *ts;
	tf = f;
	ts = s;
	
	for (int i = 0; i < to_add; i++) *ts++ = *tf++;

}

bool multi_chunk_message(char * buf, char * huge, int rc, bool state, int current_i);
void queue_print();

bool one_chunk_message(char * buf, char * huge, int rc) {

	struct message * msg = (struct message *) buf;

	if (*buf++ != LABEL_START) {
		printf("Start error\n%d\n", *(buf - 1));
	}
	rc--;

	if (msg->length = rc) {	
		q_push(msg);
		queue_print();
		return true;
	} else if (msg->length < rc) {

		pthread_mutex_lock(&mutex);
		q_push(msg);
		pthread_mutex_unlock(&mutex);
		
		return one_chunk_message(buf + rc, huge, rc - msg->length);
	} else {
		return multi_chunk_message(buf, huge, rc, false, 0);
	}
}

bool multi_chunk_message(char * buf, char * huge, int rc, bool state, int current_i) {

	uint32_t length;

	if (current_i == 0) { // first chunk of new message
		length = ((struct message *) buf)->length;

		huge = buf;
		realloc(huge, length * sizeof(char));
		return false; // message havent been got fully 

	} else { // Nth chunk
		length = ((struct message *) huge)->length;

		// if buffer fully will go to huge, then current += recieved
		// else left space have to be filled
		int to_add = (length - current_i > rc) ? rc : length - current_i;

		copy(buf, huge + current_i, to_add);
		current_i += to_add; 
		
		if (to_add == length - current_i) { // full message

			pthread_mutex_lock(&mutex);
			q_push((struct message *) huge);
			pthread_mutex_unlock(&mutex);

			char *n_huge;
			return one_chunk_message(buf, n_huge, rc - to_add);
		} else {
			return false;
		}
			
	}
	
}

void* get_message(void *args) {

	thread_args_p args_p = (thread_args_p) args;
	
	char *ip;
	{	
		// evil casting sockaddr_storage to sockaddr_in
		struct sockaddr	   *client_sockaddr 	= (struct sockaddr *) 	 	&(args_p->client_addr);
		struct sockaddr_in *client_sockaddr_in 	= (struct sockaddr_in *) 	client_sockaddr; 
		ip = inet_ntoa(client_sockaddr_in->sin_addr);
	}	
			
	// FILE * income = fdopen(args_p->socket_d, "r+"); 
	struct pollfd fds = { .fd = args_p->socket_d, .events = POLLIN };	// ready to recieve data event set on descriptor
	
	char * buf  = (char *) calloc(BUF_SIZE, sizeof(char));
	char * huge; 
	bool state = true;
	int current_i = 0;		// huge iterator
	
	while (1) {

		int ready = poll(&fds, 1, 100);				// 100 milliseconds to wait
			
		if (ready == -1) {
			printf("Poll error\n");
		}

		if (fds.revents != 0) {

			if (fds.revents & POLLIN) {

				ssize_t rc = recv(args_p->socket_d, buf, BUF_SIZE, 0);
				if (rc == -1) {
					printf("Recieve error\n");
				}

				if (state) {
					state = one_chunk_message(buf, huge, rc);
				} else {
					state = multi_chunk_message(buf, huge, rc, false, current_i);
				}
				
			}
			
		}

	}

}

void clients_add (int socket_d) {

	// setting on non blocking 
	if (fcntl(socket_d, F_SETFL, O_NONBLOCK) == -1) printf("Error while setting to nonblocking\n");

	thread_args_p args_p = (thread_args_p) malloc (100 * sizeof(thread_arguments));	// messages + file descriptor as arguments to each client thread

	pthread_mutex_init(&mutex, NULL);
	
	pthread_t *threads = malloc_threads(threads, false, 0);
	pthread_t *start = threads; // for memory allocation moment check
	
	int cur_size = NUM_CLIENTS;

	while (1) {

		int client_fd;

		struct sockaddr_storage client_address;
		socklen_t addrsize = sizeof(client_address);

		if ((client_fd = accept(socket_d, (struct sockaddr *) &client_address, &addrsize)) != -1) {

			// setting on non blocking 
			if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) printf("Error while setting to nonblocking\n");

			if (start + cur_size == threads) {
				threads = malloc_threads(threads, true, &cur_size);		
			}

			int status;
			
			args_p->socket_d 	= client_fd;
			args_p->client_addr	= client_address;

			status = pthread_create(threads++, NULL, get_message, (void*) args_p);
			// pthread_join(*(threads - 1), NULL);
			printf("Thread status: %d\n", status);
		}	
	}	

	pthread_mutex_destroy(&mutex);	
	free(start);
}

void queue_print() {

	struct node * temp;
	while ((temp = q_pop()) != NULL) {
		printf("%d\t%d\t%s\n", temp->data->opcode, temp->data->length, temp->data->payload);
	}
	
}

int main (int argc, char *argv[]) {

	int socket_d = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_storage client_address;
	socklen_t addrsize = sizeof(client_address);

	struct sockaddr *socket_addr = (struct sockaddr *) network_setting();  

	int b_s = bind(socket_d, socket_addr, sizeof(*socket_addr));

	printf("Bind status: %d\n", b_s);

	int l_s = listen(socket_d, NUM_CLIENTS);

	printf("Listen status: %d\n", l_s);

	q_init();
	clients_add(socket_d);

}
