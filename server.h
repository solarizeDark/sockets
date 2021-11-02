#include <sys/socket.h>

#ifndef __SERVER_H__
#define __SERVER_H__

typedef struct msg *message_p;

typedef struct msg {

	char *in_address;
	char *message;
	
} message;

typedef struct thread_args *thread_args_p;

typedef struct thread_args {

	message_p msg_p;
	int socket_d;
	struct sockaddr_storage client_addr;
	
} thread_arguments;

#endif 
