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

#include "server.h"

#define PORT 		3490
#define NUM_CLIENTS 10		// also threads amount

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

char* array_concat(char *str, char *arr, int recieved) {

	int str_size = 0;
	while (*str++ != '\0') str_size++;

	int res_size = str_size + recieved;
	char *res = (char*) malloc (sizeof(char) * res_size); 

	while((*res++ = *str++) != '\0') 
		;

	int i = 0;
	for (char *p = res; p != res + res_size - str_size; p++, i++) {
		*res = arr[i];
	}

	return res;
	
}

struct sockaddr_in * network_setting() {

	struct sockaddr_in *host = (struct sockaddr_in*) malloc (sizeof(struct sockaddr_in));
	  
	host->sin_family 		= AF_INET;
	host->sin_port 			= htons(PORT);
	host->sin_addr.s_addr 	= get_local_ip().s_addr;
	
	memset(host->sin_zero, '\0', sizeof(host));

	return host;
}

void* client_handler(void *args) {

	thread_args_p args_p = (thread_args_p) args;
	
	struct sockaddr_storage client_addr;
	socklen_t addrsize = sizeof client_addr;

	int new_client_fd = accept(args_p->socket_d, (struct sockaddr *) &client_addr, &addrsize);

	char *ip;
	{	
		// evil casting sockaddr_storage to sockaddr_in
		struct sockaddr	*client_sockaddr = (struct sockaddr *) &client_addr;
		struct sockaddr_in *client_sockaddr_in = (struct sockaddr_in *) client_sockaddr; 
		ip = inet_ntoa(client_sockaddr_in->sin_addr);
	}
	
	printf("%s\n", ip);

	message_p local;

	pthread_mutex_lock(&mutex);
	{
		// local pointer to list for checking for new messages
		 local = args_p->msg_p; 	
	}
	pthread_mutex_unlock(&mutex);

	
	while (true) {

		char income[2000];
		int recieved;
		
		if ((recieved = recv(args_p->socket_d, income, 2000, 0)) > 0 && strcmp(income, "\n") != 0) {

			pthread_mutex_lock(&mutex);

			{
				// adding new message to list
				message_p new_msg 	= (message_p) malloc (sizeof (message));
				new_msg->in_address = ip;
				new_msg->message 	= income;

				args_p->msg_p->message = new_msg->message;
				local++;

				printf("%s\n", new_msg->message);
			}
			 
			pthread_mutex_unlock(&mutex);
		}	

		pthread_mutex_lock(&mutex);
		// getting new messages from other
		if (local != args_p->msg_p) {

			while (local != args_p->msg_p) {
				// actual message is ip + message text
				char *msg = array_concat(local->message, ip, recieved);
				send(new_client_fd, msg, strlen(msg), 0);
				local++;
			}
		}
		pthread_mutex_unlock(&mutex);

	}
	
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

void clients_handling (int socket_d) {

	// messages list for all clients
	message_p messages = (message_p) malloc (10000 * sizeof (message));

	// messages + file descriptor as arguments to each client thread
	thread_args_p args_p = (thread_args_p) malloc (sizeof (thread_arguments));
	args_p->msg_p = messages;

	pthread_mutex_init(&mutex, NULL);
	
	pthread_t *threads = malloc_threads(threads, false, 0);
	pthread_t *start = threads; // for memory allocation moment check
	
	struct sockaddr_storage client_addr;
	socklen_t addrsize = sizeof(client_addr);
	
	int status;
	int new_client_fd;

	int cur_size = NUM_CLIENTS;

	while (true) {

		if (new_client_fd = accept(socket_d, (struct sockaddr *) &client_addr, &addrsize) != -1) {

			if (start + cur_size == threads) {
				threads = malloc_threads(threads, true, &cur_size);	
			}
			
			args_p->socket_d = new_client_fd;
			
			status = pthread_create(threads++, NULL, client_handler, (void*) args_p);
			
		}	
	}	

}

int main (int argc, char *argv[]) {

	
	int socket_d = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr *socket_addr = (struct sockaddr *) network_setting();  
	int b_res = bind(socket_d, socket_addr, sizeof *socket_addr);

	printf("%d\n", b_res);

	int l_res = listen(socket_d, NUM_CLIENTS * 100);

	printf("%d\n", l_res);

	clients_handling(socket_d);

/*
	

	struct sockaddr_in my_addr;
	
	
	bind(socket_descr, (struct sockaddr *)&my_addr, sizeof my_addr);

	listen(socket_d, 10);
	
	struct sockaddr_storage incoming_addr;
	socklen_t addrsize = sizeof incoming_addr;

	*/
}
