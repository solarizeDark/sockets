#include <stdlib.h>
#include <stdio.h>

#include "../include/queue.h"

list messages; 
struct node * twink_head; // for memory realease

void q_init() {
	messages.head = messages.tail = NULL;
}

struct node * q_get_head() {
	return twink_head;
}

void q_push(struct message * msg) {

	struct node * temp = (struct node *) malloc(sizeof(struct node *));

	temp->data = msg;
	temp->next = NULL;

	if (messages.head == NULL) {
		messages.head 		= temp;
		messages.tail 		= temp;
		twink_head			= temp;
	} else {
		messages.tail->next = temp;
		messages.tail 		= temp;
	}
	
}

struct node * q_pop() {

	if (messages.head == NULL) return NULL;

	struct node * temp = messages.head;
	messages.head = temp->next;

	return temp;	
}

void q_memory_release() {

	for (struct node * i = twink_head; i != NULL;) {
		struct node * temp = i->next;
		free(i);
		i = temp;
	}
	
}
