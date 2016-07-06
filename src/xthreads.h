/*
 * xthreads.h
 *
 *  Created on: 14 Jun 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_H_
#define XTHREADS_H_

#define XTHREADS_CREATE_UNDETACHED 0
#define XTHREADS_CREATE_DETACHED 1
#define XTHREADS_READY_DETACHED 2

#define THREAD_STACK_SIZE 2000
#define NUM_OF_THREADS 16
#define XTHREADS_THREAD_CAPACITY NUM_OF_THREADS

typedef struct xthreads_attr{
    /* threads that cannot be joined are unsynchronised threads */
    char detachState;
} xthreads_attr_t;

typedef struct xthreads_cleanup_function{
    void (*function)(void*);
    void* arg;
} xthreads_cleanup_function_t;

typedef struct xthreads_stack{
    char detached;
    int threadId;
    char bytes[THREAD_STACK_SIZE + 1];
    char safety_buffer[10];
} xthreads_stack_t;

typedef struct xthreads_data{
    char detached;
    int threadId;
} xthreads_data_t;

// The identifier for a thread
typedef int xthreads_t;

void xthreads_create(xthreads_t *thread, const xthreads_attr_t *attr, void *(start_routine)(void*), void *arg);
void xthreads_detach(xthreads_t threadId);
void xthreads_join(xthreads_t threadId);


#endif /* XTHREADS_H_ */
