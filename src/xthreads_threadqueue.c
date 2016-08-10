/*
 * xthreads_threadqueue.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_threadqueue.h"

/* Every function in here is dangerously unthreadsafe. Using it outside of the other xthreads structures is almost
 * guaranteed to cause something, somewhere to break. So don't.
 */

void xthreads_threadqueue_init(xthreads_threadqueue_t *threadqueue){
    threadqueue->insert = 0;
    threadqueue->read = 0;
    for(int i = 0; i < NUM_OF_THREADS; i++){
        threadqueue->queue[i] = XTHREADS_NOTHREAD;
    }
}

// Returns the index in the array that we just inserted at.
int xthreads_threadqueue_insert(xthreads_threadqueue_t *threadqueue, xthreads_t thread){
    int index = threadqueue->insert;
    threadqueue->queue[threadqueue->insert] = thread;
    threadqueue->insert = (threadqueue->insert + 1) % NUM_OF_THREADS;
    return index;
}

xthreads_t xthreads_threadqueue_read(xthreads_threadqueue_t *threadqueue){
    xthreads_t thread;
    // Same result if I do threadqueue->read == threadqueue->insert?
    if(threadqueue->queue[threadqueue->read] != 0){
        thread = threadqueue->queue[threadqueue->read];
        threadqueue->read = (threadqueue->read + 1) % NUM_OF_THREADS;
        return thread;
    }
    return XTHREADS_NOTHREAD;
}

void xthreads_threadqueue_nullify(xthreads_threadqueue_t *threadqueue, int index){
    threadqueue->queue[index] = XTHREADS_NOTHREAD;
}

void xthreads_threadqueue_skip(xthreads_threadqueue_t *threadqueue, int index){
    threadqueue->queue[index] = XTHREADS_TIMEREMOVEDTHREAD;
}
