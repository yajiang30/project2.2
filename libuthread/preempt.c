#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100
#define INTERVAL_USEC (1000000/HZ)

static bool preempt_enabled = false;

static void timer_handler(int signum) {
	if (signum == SIGVTALRM) {
        // Only yield if preemption is enabled
        if (preempt_enabled) {
            uthread_yield();
        }
    }
}

void preempt_disable(void)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGVTALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
}

void preempt_enable(void)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGVTALRM);
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

void preempt_start(bool preempt)
{
    if (!preempt) {
        return;
    }

    // Initialize preemption enabled state
    preempt_enabled = true;

    struct sigaction sa;
    sa.sa_handler = timer_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // Restart system calls if interrupted

    if (sigaction(SIGVTALRM, &sa, NULL)) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Configure the timer to fire every 10ms (100Hz)
    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = INTERVAL_USEC;
    timer.it_value = timer.it_interval;  // Initial expiration same as interval

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL)) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
}

void preempt_stop(void)
{
	// Disable the timer
    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_VIRTUAL, &timer, NULL);

    // Restore default signal handler
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGVTALRM, &sa, NULL);

    // Ensure preemption is disabled
    preempt_enabled = false;
}