# Target library
lib := libuthread.a
CC = gcc
CFLAGS = -Wall -Werror

all: $(lib)

$(lib): queue.o uthread.o context.o preempt.o sem.o
	ar rcs $@ $^

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c -o $@ $<

uthread.o: uthread.c uthread.h queue.h
	$(CC) $(CFLAGS) -c -o $@ $<

context.o: context.c
	$(CC) $(CFLAGS) -c -o $@ $<

sem.o: sem.c sem.h
	$(CC) $(CFLAGS) -c -o $@ $<

preempt.o: preempt.c private.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f queue.o uthread.o context.o preempt.o sem.o libuthread.a