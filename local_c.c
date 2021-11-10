// Temporary client 

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>

# define PORT 3490

struct message {

	char * 		payload;	// 8
	uint32_t 	 opcode;	// 4
	uint32_t 	 length;	// 4
	short 			crc;	// 2
	char 		  start;	// 1
							// :19
							// 5   slop
							// 24 total
};

int main() {

	struct sockaddr_in server;

	server.sin_addr.s_addr 	= inet_addr("127.0.0.1");
	server.sin_family 		= AF_INET;
	server.sin_port			= htons(PORT);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	printf("%d\n", connect(fd, (struct sockaddr *)&server, sizeof(server)));

	printf("Sizeof message: %lu\n", sizeof(struct message));

	struct message msg = { .start = '\n', .payload = "hello", .opcode = 1, .length = 17, .crc = 2 };

	char * buf = (char *) calloc (50, sizeof(char));

	char * bufi = buf;

	*bufi++ = msg.start;
	for (char * t = msg.payload; t != msg.payload + 6; t++) {
		*bufi++ = *t;
	}

	for (char *t = (char *)&msg + sizeof(char *); t != (char *)&msg + sizeof(struct message) - 7; t++) {
		*bufi++ = *t;
	}
	int i = 0;
	for (char *t = buf; t != buf + 17; t++, i++) {
		printf("%d %d\n", i, *t);
	}

	int sent = send(fd, buf, 50, 0);
	printf("%d\n", sent);
}

