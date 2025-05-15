 #include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

typedef struct node* node_t;

struct node {
	void* data;
	node_t prev;
    node_t next;
};

node_t create_node(void* data) {
	node_t node = (node_t)malloc(sizeof(struct node));
	if (!node) {
		return NULL;
	}
	node->data = data;
	node->prev = NULL;
	node->next = NULL;
	return node;
}

node_t destroy_node(node_t node) {
	if (!node) {
		return NULL;
	}
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
	free(node);
	return NULL;
}

struct queue {
	node_t head;
	node_t tail;
	int len;
};

queue_t queue_create(void)
{
	queue_t queue = (queue_t)malloc(sizeof(struct queue));
	if (!queue) {
		return NULL;
	}
	queue->head = NULL;
	queue->tail = NULL;
	queue->len = 0;
	return queue;
}

int queue_destroy(queue_t queue)
{
	if (!queue) {
		return -1;
	}
	while(queue->len) {
		node_t temp = queue->head;
		if (queue->head == queue->tail) {
			queue->tail = NULL;
		}
		queue->head = queue->head->next;
		destroy_node(temp);
		queue->len--;
	}
	free(queue);
	return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
	if (!queue || !data) {
		return -1;
	}
	node_t node = create_node(data);
	if (!node) {
		return -1;
	}
	if (queue->len == 0) {
		queue->head = node;
		queue->tail = node;
	} else {
		queue->tail->next = node;
		node->prev = queue->tail;
		queue->tail = node;
	}
	queue->len++;
	return 0;
}

int queue_dequeue(queue_t queue, void **data)
{
	if (!queue) {
		return -1;
	}
	if (queue->len == 0) {
		return -1;
	}
	node_t temp = queue->head;
	*data = temp->data;
	if (queue->len == 1) {
		queue->head = NULL;
		queue->tail = NULL;
	} else {
		queue->head = queue->head->next;
		queue->head->prev = NULL;
	}
	destroy_node(temp);
	queue->len--;
	return 0;
}

int queue_delete(queue_t queue, void *data)
{
	if (!queue || !data) {
		return -1;
	}
	node_t curr = queue->head;
	while (curr) {
		if (curr->data == data) {
			if (curr == queue->head) {
				queue->head = curr->next;
			}
			if (curr == queue->tail) {
				queue->tail = curr->prev;
			}
			destroy_node(curr);
			queue->len--;
			return 0;
		}
		curr = curr->next;
	}
	return -1;
}

int queue_iterate(queue_t queue, queue_func_t func)
{
	if (!queue || !func) {
		return -1;
	}
	node_t curr = queue->head;
	while (curr) {
		node_t next = curr->next;
		func(queue, curr->data);
		curr = next;
	}
	return 0;
}

int queue_length(queue_t queue)
{
	if (queue == NULL) {
		return -1;
	}
	return queue->len;
}
