#include <stdlib.h>

#include "../include/queue.h"

list messages; 
node_p twink_head; // for memory realease

void q_init() {
	messages.head = messages.tail = NULL;
}

node_p q_get_head() {
	return twink_head;
}

void q_push(struct message msg) {

	node_p temp = (node_p) malloc (sizeof(struct node));

	temp->data = msg;
	temp->next = NULL;

	if (messages.head == messages.tail) {
		messages.head 		= temp;
		messages.tail 		= temp;
		twink_head			= temp;
	} else {
		messages.tail->next = temp;
		messages.tail 		= temp;
	}
	
}

node_p q_pop() {

	node_p temp = messages.head;
	messages.head  = temp->next;

	return temp;	
}

void q_memory_release() {

	for (node_p i = twink_head; i != NULL;) {
		node_p temp = i->next;
		free(i);
		i = temp;
	}
	
}
