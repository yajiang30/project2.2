#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <uthread.h>

void hog(void *arg)
{
	(void)arg;

	while (true) {
        printf("hogging thread\n");
    }
}

void other(void *arg)
{
	(void)arg;

    while(true) {
        printf("me too\n");
    }
}

void main_thread(void *arg)
{
	(void)arg;

	uthread_create(hog, NULL);
    uthread_create(other, NULL);
    while (true) {
        printf("hello\n");
    }
}

int main(void)
{
	uthread_run(true, main_thread, NULL);
	return 0;
}
