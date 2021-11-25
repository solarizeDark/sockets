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
#include "../include/chat.h"
#include "../include/complex.h"
#include "../../fds_array/include/fds_array.h"
#include "../../queue/include/queue.h"

#define PORT 		"3490"
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
	AF_INET		Address family inet(ipv4)

	AI_PASSIVE flags and NULL node set socket to accepting connections
*/
struct addrinfo * network_setting() {

	struct addrinfo hint, *res;

	hint.ai_family		= AF_INET;
	hint.ai_socktype	= SOCK_STREAM;
	hint.ai_flags		= AI_PASSIVE;

	int info_status;
	if ((info_status = getaddrinfo(NULL, PORT, &hint, &res)) != 0) {
		printf("Getting info: %s, line: %d\n", gai_strerror(info_status), __LINE__);
		exit(EXIT_FAILURE);
	}	

	return res;
}

void accept_new_client(int socket_d) {

	// sockaddr_storage will fit both ipv4 and ipv6
	struct sockaddr_storage client_address;
	socklen_t addrsize = sizeof(client_address);

	int client_fd;

	if ((client_fd = accept(socket_d, (struct sockaddr *) &client_address, &addrsize)) != -1) {
		fds_add(fds_start, &fdsa, client_fd);
	}
}

void main_loop (int main_fd, void (*message_reciever)(int *) ) {

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
						(*message_reciever)(&temp->fd);
					}
				}
			}

		}	
	}	

}

int option(int argc, char **argv) {

	// chat default
	if (argc < 2) {
		return 0;
	}
	// 0 for chat 1 for complex modules
	return strcmp(argv[1], "-c") == 0 ? 0 : 1;

}

int socket_setting() {

	struct addrinfo *res = (struct addrinfo *) network_setting();  

	int socket_d;
	struct addrinfo * temp = res;
	for (; temp != NULL; temp = temp->ai_next) {
		if ((socket_d = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol)) == -1) continue;

		int on = 1;
		// SO_REUSEADDR allows other sockets bind to port, unless there is an active listening socket
		if (setsockopt(socket_d, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
			printf("setsockopt error\n");
			exit(EXIT_FAILURE);
		}

		if (bind(socket_d, temp->ai_addr, temp->ai_addrlen) == -1) {
			close(socket_d);
			continue;
		}
		
		break;
	}

	if (temp == NULL) {
		printf("binding error\n");
		exit(EXIT_FAILURE);
	}

	if (listen(socket_d, 10) == -1) {
		printf("listening error\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);
	return socket_d;
}

int main (int argc, char **argv) {

	int mode = option(argc, argv);

	set_fdsa();
	int socket_d = socket_setting();

	main_loop(socket_d, (void (*)(int*)) (mode == 0 ? get_message_chat : get_message_complex ));

}
