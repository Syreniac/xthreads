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
#include <stdio.h>
#include <stdlib.h>

static xthreads_stack_t *xthreads_create_findStackSpace(int currentThreadId, int *stackSpaceId);
static void xthreads_create_acquireThread(char detachState, int *synchroniserId, int *threadId);
static xthreads_t xthreads_create_startThread(char detachState, int threadId, int synchroniserId, int currentThreadId, xthreads_data_t *threadData);


static xthreads_stack_t *xthreads_create_findStackSpace(int currentThreadId, int *stackSpaceId){
    int i = 0;
    xthreads_lockThreadHeaders();

    /* Because synchronised threads cannot self-terminate, we will need to check when creating new threads
     * whether there are synchronised threads waiting for termination
     */
    while(i < NUM_OF_THREADS){
        if(xthreads_globals.stacks[i].data.detached == XTHREADS_READY_DETACHED && xthreads_globals.stacks[i].data.parentThreadID == currentThreadId){
            // If/when batch synchronised threads are implemented, this will need to be rethought - the mjoin will freeze unexpectedly
            // Join the synchroniser and free it
            __asm__ __volatile__("mjoin res[%0]" : : "r" (xthreads_globals.stacks[i].data.resourceId) : );
            __asm__ __volatile__("freer res[%0]" : : "r" (xthreads_globals.stacks[i].data.resourceId) : );
            // mark the stack as being free.
            xthreads_globals.stacks[i].data.detached = 0;
            xthreads_globals.stacks[i].data.resourceId = 0;
            xthreads_globals.stacks[i].data.threadId = 0;
        }
        if(xthreads_globals.stacks[i].data.resourceId == 0){
            // Mark that we've claimed it because there is a potential race condition here.
            // TODO - Locks (???)
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

static void xthreads_create_acquireThread(char detachState, int *synchroniserId, int *threadId){
    int synchroniser = 0;
    int thread = 0;

    if(detachState == XTHREADS_CREATE_DETACHED){
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
    else{
        // Get a synchroniser
        __asm__ __volatile__("getr %0, 3" : "=r" (synchroniser) : :);
        // Handle the case where we can't acquire a synchroniser
        // TODO - Add options for batch creation of threads on single synchroniser (???)
        if(synchroniserId == 0){
            printf("xthreads exception: unable to get synchroniser.\n");
            printf("Most likely, this is because you are using more synchronisers than are available\n");
            exit(1);
        }
        *synchroniserId = synchroniser;
        // Get a synchronised thread bound to the synchroniser we just acquired
        __asm__ __volatile__(
          "getst %0, res[%1]\n"
          : "=r" (thread) : "r" (synchroniser) :
        );
        // Handle the case where we can't bind a thread to a synchroniser (unlikely, but possible)
        if(thread == 0){
            printf("xthreads exception: unable to create synchronised thread\n");
            exit(1);
        }


        *threadId = thread;
    }
}

xthreads_t xthreads_create_startThread(char detachState, int threadId, int synchroniserId, int currentThreadId, xthreads_data_t *threadData){
    // thread is actually a 32bit resource id. We'll need to do some bitshiftery to get it into a usable form

    xthreads_t shiftedThreadId = (threadId << 16);
    shiftedThreadId = shiftedThreadId >> 24;

    threadData->cleanup = 0x0;
    if(detachState == XTHREADS_CREATE_DETACHED){
        // Write data into the stack structure
        threadData->resourceId = threadId;
        threadData->threadId = shiftedThreadId;
        threadData->detached = XTHREADS_CREATE_DETACHED;
        threadData->parentThreadID = currentThreadId;
        __asm__ __volatile__(
          "start t[%0]"
          : : "r" (threadId) :
        );
    }
    else{
        /* Synchronised threads are interacted with through the synchroniser rather than
         * the thread id directly, so we'll keep that in the data structure for later use.
         * People can tell this apart from a normal thread id because it will have a 4 in
         * the lowest byte.
         */
        threadData->resourceId = synchroniserId;
        threadData->threadId = shiftedThreadId; // Because other threads interact with this through the synchroniser rather than the thread, these are different
        threadData->detached = XTHREADS_CREATE_UNDETACHED;
        threadData->parentThreadID = currentThreadId;
        __asm__ __volatile__(
          "msync res[%0]"
          : : "r" (synchroniserId) :
        );
    }
    return shiftedThreadId;
}


void xthreads_create(xthreads_t *thread, const xthreads_attr_t *attr, void *(start_routine)(void*), void *arg){
    int synchroniserId = 0;
    int threadId = 0;
    void* startPtr = &xthreads_task;
    char* stackPtr;
    xthreads_data_t* threadData;
    xthreads_stack_t* stackSpace;
    int i = 0;
    register int volatile currentThreadId asm("r11") = 0;


    // Read out the id of the current thread
    __asm__ __volatile__("get r11,id" : : : "r11");

    stackSpace = xthreads_create_findStackSpace(currentThreadId, &i);

    stackPtr = &stackSpace->bytes[THREAD_STACK_SIZE];
    threadData = &stackSpace->data;

    xthreads_create_acquireThread(attr->detachState, &synchroniserId, &threadId);

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
    xthreads_create_startThread(attr->detachState, threadId, synchroniserId, currentThreadId, threadData);
    if(thread != 0x0){
        *thread = i;
    }
}


