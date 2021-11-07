#ifndef	__QUEUE_H__
#define	__QUEUE_H__

#include <stdint.h>

typedef struct message {

	uint32_t opcode;
	uint32_t length;
	char *payload;
	short crc;
	
} message;

typedef struct node {

	message * data;
	struct node * next;
	
} node;

typedef struct list {

	struct node * head;
	struct node * tail;
	
} list;

void	q_init();
void 	q_push(struct message *);
void 	q_memory_release();
struct node* 	q_pop();
struct node*	q_get_head();

#endif
