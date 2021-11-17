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

#include "../include/server.h"
#include "../../fds_array/include/fds_array.h"
#include "../../queue/include/queue.h"

#define PORT 		3490
#define NUM_CLIENTS 10		// also threads amount
#define BUF_SIZE 	200
#define LABEL_START '\n'


// file descriptor will be monitored if it is ready to recieve data
struct pollfd *fdsa;
struct pollfd *fds_start;

void set_fdsa() {

	fdsa 		= (struct pollfd *) malloc (sizeof(struct pollfd) * CAPACITY);
	fds_start	= fdsa;

}

void _ip(struct sockaddr_storage sas) {
	
	char *ip;
	{	
		// evil casting sockaddr_storage to sockaddr_in
		struct sockaddr	   *client_sockaddr 	= (struct sockaddr *) 	 	&sas;
		struct sockaddr_in *client_sockaddr_in 	= (struct sockaddr_in *) 	client_sockaddr; 
		ip = inet_ntoa(client_sockaddr_in->sin_addr);
	}	
	printf("%s\n", ip);

}

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

/*
	IP socket address = IP address + 16bit port number
	AF_INET		Address family inet(ip)

*/
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
		// queue_print();
		return true;
	} else if (msg->length < rc) {

		q_push(msg);
		
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
		huge = realloc(huge, length * sizeof(char));
		return false; // message havent been got fully 

	} else { // Nth chunk
		length = ((struct message *) huge)->length;

		// if buffer fully will go to huge, then current += recieved
		// else left space have to be filled
		int to_add = (length - current_i > rc) ? rc : length - current_i;

		copy(buf, huge + current_i, to_add);
		current_i += to_add; 
		
		if (to_add == length - current_i) { // full message

			q_push((struct message *) huge);

			char *n_huge;
			return one_chunk_message(buf, n_huge, rc - to_add);
		} else {
			return false;
		}
			
	}
	
}

bool get_message(int fd) {
	
	char * buf  = (char *) calloc(BUF_SIZE, sizeof(char));
	char * huge; 
	bool state = true;
	// huge iterator
	int current_i = 0;

	ssize_t rc = recv(fd, buf, BUF_SIZE, 0);

	if (rc <= 0) {
		return false;
	}

	if (state) {
		return one_chunk_message(buf, huge, rc);
	} else {
		return multi_chunk_message(buf, huge, rc, false, current_i);
	}

}

void accept_new_client(int socket_d) {
	struct sockaddr_storage client_address;
	socklen_t addrsize = sizeof(client_address);

	int client_fd;

	if ((client_fd = accept(socket_d, (struct sockaddr *) &client_address, &addrsize)) != -1) {
		fds_add(fds_start, &fdsa, client_fd);
	}
}

void main_loop (int main_fd) {

	// setting to non blocking
	// F_SETFL sets file flags specified by 3rd arg 
	if (fcntl(main_fd, F_SETFL, O_NONBLOCK) == -1) printf("Error while setting to nonblocking\n");

	// first socket is server main listener
	fdsa->fd 		= main_fd; 
	fdsa->events	= POLLIN;

	fdsa++;	

	while (1) {

		int events = poll(fds_start, fdsa - fds_start, 1000);

		// on one of fd event has occurred
		if (events != 0) {
			// going through descriptors
			for (struct pollfd *temp = fds_start; temp != fdsa; temp++) {
				// found exact fd
				if (temp->revents && POLLIN) {
					
					// main server listener, means new client occurred
					if (temp->fd == main_fd) {
						printf("accept\n");
						accept_new_client(main_fd);
					} else {
						get_message(temp->fd);
						queue_print();
					}
				}
			}

		}	
	}	

}

void queue_print() {

	struct node * temp;
	while ((temp = q_pop()) != NULL) {
		printf("%d\t%d\n", temp->data->opcode, temp->data->length);
	}
	
}

int main (int argc, char *argv[]) {

	set_fdsa();

	int socket_d = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_storage client_address;
	socklen_t addrsize = sizeof(client_address);

	struct sockaddr *socket_addr = (struct sockaddr *) network_setting();  

	int b_s = bind(socket_d, socket_addr, sizeof(*socket_addr));

	printf("Bind status: %d\n", b_s);

	int l_s = listen(socket_d, NUM_CLIENTS);

	printf("Listen status: %d\n", l_s);

	q_init();
	main_loop(socket_d);

}
