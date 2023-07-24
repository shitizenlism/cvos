/*
 * main.c
 *
 * Test harness for ring buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ringbuf.h"

#define ELEMENTSOF(arr) (sizeof(arr) / sizeof((arr)[0]))
#define tmalloc(count, type) ((type *) malloc((count) * sizeof(type)))

void dequeue_and_print(ringbuf *r) {
	char c;

	if (ringbuf_dequeue(r, &c) < 0) {
		fputs("Underflow\n", stderr);
	} else {
		fprintf(stderr, "Got %c\n", c);
	}
}

unsigned int rand_between(unsigned int lo, unsigned int hi)
{
	return lo + (int) (rand() / ((double) RAND_MAX / (hi - lo + 1) + 1));
}

void test_overflow(const char *test_name)
{
	fprintf(stderr, "Test %s\n", test_name);

	ringbuf r;
	datum ringbuf_arr[5];
	memset(&r, 0, sizeof(r));
	memset(ringbuf_arr, 0, sizeof(ringbuf_arr));

	ringbuf_init(&r, ELEMENTSOF(ringbuf_arr), ringbuf_arr);
	ringbuf_dump(&r);

	ringbuf_enqueue(&r, "0");
	ringbuf_dump(&r);
	ringbuf_enqueue(&r, "1");
	ringbuf_dump(&r);
	ringbuf_enqueue(&r, "2");
	ringbuf_dump(&r);
	ringbuf_enqueue(&r, "3");
	ringbuf_dump(&r);
	ringbuf_enqueue(&r, "4");
	ringbuf_dump(&r);
	ringbuf_enqueue(&r, "5");
	ringbuf_dump(&r);
	ringbuf_enqueue(&r, "6");
	ringbuf_dump(&r);
	ringbuf_enqueue(&r, "7");
	ringbuf_dump(&r);

	dequeue_and_print(&r);
	ringbuf_dump(&r);

	ringbuf_fini(&r);
}

void test_underflow(const char *test_name)
{
	fprintf(stderr, "Test %s\n", test_name);

	ringbuf r;
	datum ringbuf_arr[5];
	memset(&r, 0, sizeof(r));
	memset(ringbuf_arr, 0, sizeof(ringbuf_arr));

	ringbuf_init(&r, ELEMENTSOF(ringbuf_arr), ringbuf_arr);
	ringbuf_dump(&r);

	ringbuf_enqueue(&r, "0");
	ringbuf_enqueue(&r, "1");
	ringbuf_enqueue(&r, "2");
	ringbuf_dump(&r);

	dequeue_and_print(&r);
	dequeue_and_print(&r);
	dequeue_and_print(&r);
#if 1
	dequeue_and_print(&r);
#endif
	ringbuf_dump(&r);

	ringbuf_fini(&r);
}

void *producer_func(void *arg)
{
	ringbuf *r = (ringbuf *) arg;

	unsigned int count = rand_between((int) (r->size * 1.5), r->size * 2);
	while (count--) {
		char c = rand_between('A', 'z');
		fprintf(stderr, "Enqueue %c\n", c);
		ringbuf_enqueue_blocking(r, &c);
		ringbuf_dump(r);
		unsigned int secs = rand_between(0, 5);
		fprintf(stderr, "Producer: sleep %d\n", secs);
		sleep(secs);
	}

	return NULL;
}

typedef struct consumer_arg {
	ringbuf *r;
	unsigned int producer_has_exited;
} consumer_arg;

void *consumer_func(void *arg)
{
	consumer_arg *carg = (consumer_arg *) arg;

	for (;;) {
		char c;
		if (carg->producer_has_exited) {
			if (ringbuf_dequeue(carg->r, &c) < 0) {
				/* EOF */
				break;
			}
		} else {
			ringbuf_dequeue_blocking(carg->r, &c);
		}
		fprintf(stderr, "Dequeued %c\n", c);
		ringbuf_dump(carg->r);
		unsigned int secs = rand_between(0, 5);
		fprintf(stderr, "Consumer: sleep %d\n", secs);
		sleep(secs);
	}

	return NULL;
}

void test_multithreaded(const char *test_name)
{
	fprintf(stderr, "Test %s\n", test_name);

	ringbuf r;
	datum *ringbuf_arr = tmalloc(100, char);
	if (NULL == ringbuf_arr) {
		perror("malloc");
		return;
	}
	memset(&r, 0, sizeof(r));
	memset(ringbuf_arr, 0, 100 * sizeof(char));

	ringbuf_init(&r, 100, ringbuf_arr);
	ringbuf_dump(&r);

	pthread_t producer, consumer;

	if (pthread_create(&producer, NULL, producer_func, &r) < 0) {
		perror("pthread_create");
	}
	consumer_arg carg = { .r = &r, .producer_has_exited = 0 };
	if (pthread_create(&consumer, NULL, consumer_func, &carg) < 0) {
		perror("pthread_create");
	}
	if (pthread_join(producer, NULL) < 0) {
		perror("pthread_join");
	}
	carg.producer_has_exited = 1;
	if (pthread_join(consumer, NULL) < 0) {
		perror("pthread_join");
	}
}

int main(int argc, char **argv)
{
	srand(time(NULL));

#if 0
	test_underflow("underflow");
	test_overflow("overflow");
#endif
	test_multithreaded("multithreaded");
	return EXIT_SUCCESS;
}
