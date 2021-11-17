#include <sys/socket.h>
#ifndef __SERVER_H__
#define __SERVER_H__

typedef struct thread_args {

	int socket_d;
	struct sockaddr_storage client_addr;
	
} thread_args;

#endif 
