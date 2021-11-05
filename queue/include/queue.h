#ifndef	__QUEUE_H__
#define	__QUEUE_H__

typedef struct node * node_p;

typedef struct node {

	message data;
	node_p next;
	
} node;

typedef struct list {

	node_p 	head;
	node_p 	tail;
	
} list;

typedef struct message {

	uint32_t opcode;
	uint32_t length;
	char *payload;
	short crc;
	
} message;

void	q_init();
void 	q_push(char*);
void 	q_memory_release();
node_p 	q_pop();
node_p	q_get_head();

#endif
