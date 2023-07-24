/*
 * ringbuf.h
 *
 * Circular ('ring') buffer.
 */

#ifndef RINGBUF_H_
#define RINGBUF_H_

#define RINGBUF_ASSIGN_DATA
#define RINGBUF_MUTEX
#ifdef RINGBUF_MUTEX
#include <pthread.h>
#define RINGBUF_BLOCKING
#endif /* RINGBUF_MUTEX */

#define BUF_DEEPTH 256
#if 0
#define BUF_SIZE 16
typedef struct{
	int writepos;
	size_t size;
	int readpos;
#ifdef RINGBUF_MUTEX
	pthread_mutex_t mutex;
#endif /* RINGBUF_MUTEX */
#ifdef RINGBUF_BLOCKING
	pthread_cond_t cond_notfull;
	pthread_cond_t cond_notempty;
#endif /* RINGBUF_BLOCKING */

	char buf[BUF_SIZE][BUF_DEEPTH];
}RINGBUF_S;
#endif
struct msg{
	unsigned char len;
	unsigned char data[BUF_DEEPTH-1];
};

struct onebuf{
    union
	{	
		unsigned char buf[BUF_DEEPTH];
		struct msg mBuf;
    };
};

//typedef char datum;
typedef struct onebuf datum;

struct ringbuf {
	datum *writepos;
	size_t size;
	size_t num;
	datum *readpos;
#ifdef RINGBUF_MUTEX
	pthread_mutex_t mutex;
#endif /* RINGBUF_MUTEX */
#ifdef RINGBUF_BLOCKING
	pthread_cond_t cond_notfull;
	pthread_cond_t cond_notempty;
#endif /* RINGBUF_BLOCKING */
	datum *arr;
};
typedef struct ringbuf ringbuf;

void ringbuf_init(ringbuf *r, size_t size, datum *arr);
void ringbuf_fini(ringbuf *r);
int ringbuf_enqueue(ringbuf *r, const datum *d);
#ifdef RINGBUF_BLOCKING
int ringbuf_enqueue_blocking(ringbuf *r, const datum *d);
#endif /* RINGBUF_BLOCKING */
int ringbuf_dequeue(ringbuf *r, datum *d);
#ifdef RINGBUF_BLOCKING
int ringbuf_dequeue_blocking(ringbuf *r, datum *d);
#endif /* RINGBUF_BLOCKING */
//void ringbuf_dump(ringbuf *r);

#endif /* RINGBUF_H_ */
