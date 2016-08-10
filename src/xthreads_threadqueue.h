/*
 * xthreads_threadqueue.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_THREADQUEUE_H_
#define XTHREADS_THREADQUEUE_H_

typedef struct xthreads_threadqueue{
    xthreads_t queue[NUM_OF_THREADS];
    int insert;
    int read;
} xthreads_threadqueue_t;

void xthreads_threadqueue_init(xthreads_threadqueue_t *threadqueue);
int xthreads_threadqueue_insert(xthreads_threadqueue_t *threadqueue, xthreads_t thread);
xthreads_t xthreads_threadqueue_read(xthreads_threadqueue_t *threadqueue);
void xthreads_threadqueue_nullify(xthreads_threadqueue_t *threadqueue, int index);
void xthreads_threadqueue_skip(xthreads_threadqueue_t *threadqueue, int index);

#endif /* XTHREADS_THREADQUEUE_H_ */
