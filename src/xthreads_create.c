/*
 * xthreads_create.c
 *
 *  Created on: 4 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_util.h"
#include "xthreads_create.h"
#include "xthreads_task.h"
#include "xthreads_self.h"
#include "xthreads_access.h"
#include <stdio.h>
#include <stdlib.h>

static xthreads_stack_t *xthreads_create_findStackSpace(int currentThreadId, int *stackSpaceId);
static void xthreads_create_acquireThread(char detachState, int *threadId);
static xthreads_t xthreads_create_startThread(char detachState, int threadId, int currentThreadId, xthreads_data_t *threadData);


static xthreads_stack_t *xthreads_create_findStackSpace(int currentThreadId, int *stackSpaceId){
    int i = 0;
    xthreads_lockThreadHeaders();

    while(i < NUM_OF_THREADS){
        if(xthreads_globals.stacks[i].data.resourceId == 0){
            // Mark that we've claimed it because there is a potential race condition here.
            xthreads_globals.stacks[i].data.resourceId = -1;
            xthreads_unlockThreadHeaders();
            *stackSpaceId = i;
            return &xthreads_globals.stacks[i];
        }
        else{
            i = (i+1);
        }
    }
    // Handle the case where there's no stacks available
    printf("xthreads exception: unable to find thread stack space.\n");
    exit(1);
}

static void xthreads_create_acquireThread(char detachState, int *threadId){
    int thread = 0;

    __asm__ __volatile__(
      "getr %0, 4\n"
      : "=r" (thread) : :
    );
    // Handle the case where we can't acquire an unsynchronised thread
    if(thread == 0){
        printf("xthreads exception: unable to create unsynchronised thread.\n");
        exit(1);
    }
    *threadId = thread;
}

xthreads_t xthreads_create_startThread(char detachState, int threadId, int currentThreadId, xthreads_data_t *threadData){
    // thread is actually a 32bit resource id. We'll need to do some bitshiftery to get it into a usable form
    // for xthreads_self() to work with

    xthreads_t shiftedThreadId = (threadId << 16);
    shiftedThreadId = shiftedThreadId >> 24;

    threadData->cleanup = NULL;
    // Write data into the stack structure
    threadData->resourceId = threadId;
    threadData->threadId = shiftedThreadId;
    threadData->parentThreadID = currentThreadId;
    if(detachState == XTHREADS_CREATE_DETACHED){
        threadData->returnChannel = -2;
        threadData->detached = XTHREADS_CREATE_DETACHED;
    }
    else{
        threadData->detached = XTHREADS_CREATE_UNDETACHED;
        threadData->returnChannel = 0;
    }
    __asm__ __volatile__(
      "start t[%0]"
      : : "r" (threadId) :
    );
    return shiftedThreadId;
}


void xthreads_create(xthreads_t *thread, const xthreads_attr_t *attr, void *(start_routine)(void*), void *arg){
    int threadId = 0;
    const void* startPtr = &xthreads_task;
    char* stackPtr;
    xthreads_data_t* threadData;
    xthreads_stack_t* stackSpace;
    int i = 0;
    register int volatile currentThreadId asm("r11") = 0;
    char detachState;
    if(attr == NULL){
        detachState = XTHREADS_CREATE_UNDETACHED;
    }
    else{
        detachState = attr->detachState;
    }


    // Read out the id of the current thread
    __asm__ __volatile__("get r11,id" : : : "r11");

    stackSpace = xthreads_create_findStackSpace(currentThreadId, &i);

    stackPtr = &stackSpace->bytes[THREAD_STACK_SIZE];
    threadData = &stackSpace->data;

    xthreads_create_acquireThread(detachState, &threadId);

    // Initialise the thread with meaningful data.
    // ASSUMPTION: Threads inherit data and constant pointers from the parent implicitly
    asm volatile(
            "init t[%0]:sp, %1\n"
            "init t[%0]:pc, %2\n"
            "set t[%0]:r0, %3\n"
            "set t[%0]:r1, %4\n"
            "set t[%0]:r2, %5\n"
            : : "r" (threadId), "r" (stackPtr), "r" (startPtr), "r" (start_routine), "r" (arg), "r" (threadData):
    );

    /* In theory, because we've marked the thread header when selecting it, there should be
     * no race condition here when writing actual data into it here
     */
    xthreads_create_startThread(detachState, threadId, currentThreadId, threadData);
    if(thread != 0x0){
        *thread = i + xthreads_globals.stacks[i].data.count * NUM_OF_THREADS;
    }
}


