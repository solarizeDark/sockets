#include <stdio.h>

#define BUF_SIZE 200

void get_message(int *fd) {

	char *buf = (char *) malloc(sizeof(char) * BUF_SIZE);
	ssize_t rc = recv(*fd, buf, BUF_SIZE, 0);
	if (rc > 0) printf("%s\n", buf); 	
	
}

void send_message(int *fd) {
	
}
