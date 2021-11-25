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

#include "../include/complex.h"
#include "../../queue/include/queue.h"

#define BUF_SIZE 	200
#define LABEL_START '\n'

void copy(char * f, char * s, int to_add) {

	char *tf, *ts;
	tf = f;
	ts = s;
	
	for (int i = 0; i < to_add; i++) *ts++ = *tf++;

}

bool one_chunk_message(char * buf, char * huge, int rc) {

	struct message * msg = (struct message *) buf;

	if (*buf++ != LABEL_START) {
		printf("Start error\n%d\n", *(buf - 1));
	}
	rc--;

	if (msg->length == rc) {	
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

bool get_message_complex(int *fd) {
	
	char * buf  = (char *) calloc(BUF_SIZE, sizeof(char));
	char * huge; 
	bool state = true;
	// huge iterator
	int current_i = 0;

	ssize_t rc = recv(*fd, buf, BUF_SIZE, 0);

	if (rc <= 0) {
		return false;
	}

	if (state) {
		return one_chunk_message(buf, huge, rc);
	} else {
		return multi_chunk_message(buf, huge, rc, false, current_i);
	}

}

