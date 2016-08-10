/*
 * xthreads.h
 *
 *  Created on: 14 Jun 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_H_
#define XTHREADS_H_

#include "xthreads_errors.h"
#include "xthreads_cleanup.h"

#ifndef NULL
#define NULL 0x0
#endif

#define XTHREADS_NOTHREAD -1
#define XTHREADS_SPINBLOCK -2
#define XTHREADS_TIMEREMOVEDTHREAD -3
#define XTHREADS_BARRIER_SERIAL_THREAD 1

#define XTHREADS_CREATE_UNDETACHED 0xA
#define XTHREADS_CREATE_DETACHED 0xB
#define XTHREADS_READY_DETACHED 0xC
#define XTHREADS_LATER_DETACHED 0xD
#define XTHREADS_FINISHED 0xE

#define XTHREADS_CANCELLATION_READY 0
#define XTHREADS_CANCELLATION_WRITING 1
#define XTHREADS_CANCELLATION_ACTIVE 2

#define THREAD_STACK_SIZE 2000
#define NUM_OF_THREADS 16
#define NUM_OF_KEYS 4
#define XTHREADS_THREAD_CAPACITY NUM_OF_THREADS

typedef struct xthreads_stack xthreads_stack_t;
typedef struct xthreads_data xthreads_data_t;

// The identifier for a thread
typedef int xthreads_t;
// A helper type for channels
typedef unsigned int xthreads_channel_t;
typedef unsigned int xthreads_resource_t;
typedef int xthreads_lock_t;
typedef unsigned int xthreads_time_t;

struct xthreads_data{
    char detached;
    xthreads_t threadId;
    int resourceId;
    xthreads_t parentThreadID;
    xthreads_channel_t threadChannel;
    xthreads_channel_t cancelChannel;
    void *returnedValue;
    xthreads_cleanup_function_t *cleanup;
    char cancelState;
    char cancelType;
    void *keys[NUM_OF_KEYS];
};

struct xthreads_stack{
    xthreads_data_t data;
    char bytes[THREAD_STACK_SIZE + 1];
    char safety_buffer[10];
};

typedef struct xthreads_global{
    xthreads_lock_t lock;
    xthreads_stack_t stacks[NUM_OF_THREADS];
    void* keys[NUM_OF_KEYS];
} xthreads_global_t;


// Globals

extern xthreads_global_t xthreads_globals;


#endif /* XTHREADS_H_ */
