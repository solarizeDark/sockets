#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <pthread.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if.h>

#define PORT 		3490
#define NUM_CLIENTS 10		// also threads amount

struct in_addr get_local_ip () {

	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;

	strncpy(ifr.ifr_name, "enp0s3", IFNAMSIZ - 1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	printf("%s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
	return ((struct sockaddr_in*) &ifr.ifr_addr) -> sin_addr;
}

struct sockaddr_in * network_setting() {

	struct sockaddr_in *host = (struct sockaddr_in*) malloc (sizeof(struct sockaddr_in));
	  
	host->sin_family 		= AF_INET;
	host->sin_port 			= htons(PORT);
	host->sin_addr.s_addr 	= get_local_ip().s_addr;
	
	memset(host->sin_zero, '\0', sizeof host);

	return host;
}

void *clients_handling (int socket_d) {

	struct sockaddr_storage client_addr;
	socklen_t addrsize = sizeof client_addr;

	int new_client_fd = accept(socket_d, (struct sockaddr *) &client_addr, &addrsize);
	
	struct sockaddr	*client_sockaddr = (struct sockaddr *) &client_addr;
	struct sockaddr_in *client_sockaddr_in = (struct sockaddr_in *) client_sockaddr; 

	char *ip = inet_ntoa(client_sockaddr_in->sin_addr);
	
	printf("%s\n", ip);

	char income[2000];
	int recieved;
	while ((recieved = recv(new_fd, income, 2000, 0)) > 0 && strcmp(income, "\n") != 0) {
		printf("%s", income);
		send(new_fd, income, recieved, 0);
	}
	
}

int main (int argc, char *argv[]) {

	
	int socket_d = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr *socket_addr = (struct sockaddr *) network_setting();  
	int b_res = bind(socket_d, socket_addr, sizeof *socket_addr);
	printf("%d\n", b_res);

	listen(socket_d, NUM_CLIENTS);

	pthread_t threads[NUM_CLIENTS];

		

/*
	

	struct sockaddr_in my_addr;
	
	
	bind(socket_descr, (struct sockaddr *)&my_addr, sizeof my_addr);

	listen(socket_d, 10);
	
	struct sockaddr_storage incoming_addr;
	socklen_t addrsize = sizeof incoming_addr;

	int new_fd = accept(socket_descr, (struct sockaddr *)&incoming_addr, &addrsize);

	struct sockaddr	*client_sockaddr = (struct sockaddr *)&incoming_addr;
	struct sockaddr_in *client_sockaddr_in = (struct sockaddr_in *)client_sockaddr; 
	char *ip = inet_ntoa(client_sockaddr_in->sin_addr);
	printf("%s\n", ip);

	char income[2000];
	int recieved;
	while ((recieved = recv(new_fd, income, 2000, 0)) > 0 && strcmp(income, "\n") != 0) {
		printf("%s", income);
		send(new_fd, income, recieved, 0);
	}
*/

}
