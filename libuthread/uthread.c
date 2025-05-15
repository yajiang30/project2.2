#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"

// running, ready, blocked
typedef enum {
	STATE_RUNNING,
	STATE_READY,
	STATE_BLOCKED,
	STATE_ZOMBIE
} state_t;

struct uthread_tcb {
	uthread_ctx_t context;
	uthread_func_t function;
	void *stack;
	void *arg;
	state_t state;
};

queue_t ready_queue;
queue_t zombie_queue; // used to deallocate resources of finished threads
queue_t blocked_queue;
struct uthread_tcb *current_thread;

struct uthread_tcb *uthread_current(void)
{
	return current_thread;
}

void uthread_yield(void)
{
	preempt_disable();
	if (queue_length(ready_queue) == 0) {
		preempt_enable();
		return;
	}
	struct uthread_tcb *next_thread;
	if (queue_dequeue(ready_queue, (void **)&next_thread) == -1) {
		preempt_enable();
		return;
	}
	if (current_thread->state != STATE_BLOCKED) {
		queue_enqueue(ready_queue, (void *)current_thread);
		current_thread->state = STATE_READY;
	}

	struct uthread_tcb *prev_thread = current_thread;

	current_thread = next_thread;
	current_thread->state = STATE_RUNNING;
	uthread_ctx_switch(&prev_thread->context, &next_thread->context);
	preempt_enable();
}

void uthread_exit(void)
{
	preempt_disable();
	if (queue_enqueue(zombie_queue, (void *)current_thread) == -1) {
		preempt_enable();
		return;
	}
	struct uthread_tcb *prev_thread = current_thread;
	prev_thread->state = STATE_ZOMBIE;

	struct uthread_tcb *next_thread;
	if (queue_dequeue(ready_queue, (void **)&next_thread) == -1) {
		preempt_enable();
		return;
	}

	current_thread = next_thread;
	current_thread->state = STATE_RUNNING;
	uthread_ctx_switch(&prev_thread->context, &next_thread->context);
	preempt_enable();
}

int uthread_create(uthread_func_t func, void *arg)
{
	preempt_disable();
	struct uthread_tcb *new_thread = malloc(sizeof(struct uthread_tcb));
	if (!new_thread) {
		return -1;
	}
	new_thread->function = func;
	new_thread->arg = arg;
	new_thread->state = STATE_READY;
	new_thread->stack = uthread_ctx_alloc_stack();
	if (!new_thread->stack) {
		free(new_thread);
		preempt_enable();
		return -1;
	}
	if (uthread_ctx_init(&new_thread->context, new_thread->stack, func, arg) == -1) {
		free(new_thread->stack);
		free(new_thread);
		preempt_enable();
		return -1;
	}
	if (queue_enqueue(ready_queue, new_thread) == -1) {
		free(new_thread->stack);
		free(new_thread);
		preempt_enable();
		return -1;
	}
	preempt_enable();
	return 0;
}

int uthread_run(bool preempt, uthread_func_t func, void *arg)
{
	preempt_start(preempt);

	preempt_disable();
	ready_queue = queue_create();
	zombie_queue = queue_create();
	blocked_queue = queue_create();
	if (!ready_queue || !zombie_queue || !blocked_queue) {
		if (ready_queue) {
			queue_destroy(ready_queue);
		}
		if (zombie_queue) {
			queue_destroy(zombie_queue);
		}
		if (blocked_queue) {
			queue_destroy(blocked_queue);
		}
		return -1;
	}
	struct uthread_tcb *idle_thread = malloc(sizeof(struct uthread_tcb));
	if (!idle_thread) {
		queue_destroy(ready_queue);
		queue_destroy(zombie_queue);
		queue_destroy(blocked_queue);
		return -1;
	}
	idle_thread->state = STATE_RUNNING;
	current_thread = idle_thread;

	if (uthread_create(func, arg) == -1) {
		free(idle_thread);
		queue_destroy(ready_queue);
		queue_destroy(zombie_queue);
		queue_destroy(blocked_queue);
		return -1;
	}
	preempt_enable();

	while (queue_length(ready_queue) > 0) {
		uthread_yield();

		preempt_disable();
		while(queue_length(zombie_queue) > 0) {
			struct uthread_tcb *zombie_thread;
			if (queue_dequeue(zombie_queue, (void **)&zombie_thread) == -1) {
				break;
			}
			free(zombie_thread->stack);
			free(zombie_thread);
		}
		preempt_enable();
	}

	preempt_stop();
	free(idle_thread);
	queue_destroy(ready_queue);
	queue_destroy(zombie_queue);
	queue_destroy(blocked_queue);

	return 0;
}

void uthread_block(void)
{
	preempt_disable();
	current_thread->state = STATE_BLOCKED;
	if (queue_enqueue(blocked_queue, (void *)current_thread) == -1) {
		return;
	}
	preempt_enable();
	uthread_yield();
}

void uthread_unblock(struct uthread_tcb *uthread)
{
	preempt_disable();
	if (queue_delete(blocked_queue, (void *)uthread) == -1) {
		return;
	}
	uthread->state = STATE_RUNNING;
	struct uthread_tcb *prev_thread = current_thread;
	prev_thread->state = STATE_READY;
	if (queue_enqueue(ready_queue, (void *)prev_thread) == -1) {
		return;
	}
	current_thread = uthread;
	uthread_ctx_switch(&prev_thread->context, &current_thread->context);
	preempt_enable();
}

