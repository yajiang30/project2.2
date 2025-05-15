#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "private.h"
#include "sem.h"

struct semaphore {
	size_t count;
	queue_t blocked_queue;
};

sem_t sem_create(size_t count)
{
	sem_t sem = malloc(sizeof(struct semaphore));
	if (!sem) {
		return NULL;
	}
	sem->count = count;
	sem->blocked_queue = queue_create();
	if (!sem->blocked_queue) {
		free(sem);
		return NULL;
	}
	return sem;
}

int sem_destroy(sem_t sem)
{
	if (!sem) {
		return -1;
	}
	if (queue_length(sem->blocked_queue) > 0) {
		return -1;
	}
	queue_destroy(sem->blocked_queue);
	free(sem);
	return 0;
}

int sem_down(sem_t sem)
{
	if (!sem) {
		return -1;
	}
	if (sem->count > 0) {
		sem->count--;
	} else {
		struct uthread_tcb *current_thread = uthread_current();
		queue_enqueue(sem->blocked_queue, current_thread);
		uthread_block();
	}
	return 0;
}

int sem_up(sem_t sem)
{
	if (!sem) {
		return -1;
	}
	if (queue_length(sem->blocked_queue) > 0) {
		struct uthread_tcb *unblocked_thread;
		if (queue_dequeue(sem->blocked_queue, (void **)&unblocked_thread) == -1) {
			return -1;
		}
		uthread_unblock(unblocked_thread);
	}
	sem->count++;
	return 0;
}

