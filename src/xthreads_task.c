/*
 * xthreads_task.c
 *
 *  Created on: 4 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_task.h"
#include "xthreads_util.h"
#include "xthreads_self.h"
#include "xthreads_access.h"
#include <stdlib.h>
#include <stdio.h>

void xthreads_task_cleanup(xthreads_data_t *threadData){

    xthreads_cleanup_function_t *cleanup = threadData->cleanup;
    xthreads_cleanup_function_t *next = 0x0;
    // Get the top of the cancellation stack

    // Loop until the stack hits NULL
    while(cleanup != 0x0){
        cleanup->function(cleanup->arg);
        next = cleanup->next;
        free(cleanup);
        cleanup = next;
    }
}
void xthreads_task(void *(*target)(void*), void* args, xthreads_data_t* threadData){

    __asm__ __volatile__("clre");
    void *returnedValue = 0x0;

    __asm__ __volatile__("getr %0, 2" : "=r" (threadData->threadChannel) : : );
    __asm__ __volatile__("getr %0, 2" : "=r" (threadData->cancelChannel) : : );
    if(threadData->threadChannel == 0 || threadData->cancelChannel == 0){
        printf("xthreads exception: unable to acquire channel for thread\n");
        exit(1);
    }
    // Because we need to spoof cancel signals to ourself at certain points in event handling
    // we'll set the destination here tautologously.
    // It's not a major problem because this channel is *never* used to send signals to other channels
    // Don't do this in real code, it'll cause bad things to happen!
    __asm__ __volatile__("setd res[%0], %0" : : "r" (threadData->cancelChannel) : );

    returnedValue = target((void*)args);
    xthreads_task_exit(returnedValue);
}

__attribute__((noinline)) xthreads_channel_t dummyFunction(xthreads_t self){
    return xthreads_self_returnChannel(self);
}

void __attribute__((noinline,noreturn)) xthreads_task_exit(void* retval){
    __asm__ __volatile("clre");
    xthreads_t self = xthreads_self_outer();
    if(xthreads_self_detached(self) != XTHREADS_CREATE_DETACHED){
        const register xthreads_channel_t selfChannel asm("r10") = xthreads_self_threadChannel(self);
        register xthreads_channel_t returnChannel asm("r9") = xthreads_self_returnChannel(self);
        if(returnChannel == 0){
            // No thread waiting on us
            // Sleep until woken
            xthreads_globals.stacks[self].data.returnChannel = -1;
            __asm__ __volatile__("chkct res[%0], 1\n" : : "r" (selfChannel) :);
            returnChannel = dummyFunction(self);
        }
        __asm__ __volatile(
                "setd res[%0], %1\n"
                "out res[%0], %2\n"
                "outct res[%0], 1\n"
                : : "r" (selfChannel), "r" (returnChannel), "r" (retval) :);
    }
    xthreads_data_t *threadData = &xthreads_globals.stacks[self].data;
    xthreads_task_end(threadData);
}
#define XTHREADS_CANCELLED 0xffffffff
void xthreads_task_endsudden(void){
    __asm__ __volatile__("clre");
    printf("sudden ending\n");
    xthreads_task_exit((void*)(long)XTHREADS_CANCELLED);
}

void __attribute__ ((noinline,noreturn)) xthreads_task_end(xthreads_data_t *threadData){

    xthreads_task_cleanup(threadData);

    // Free the thread's internal channel
    __asm__ __volatile__(
            "freer res[%0]\n"
            "freer res[%1]"
            : : "r" (threadData->threadChannel), "r" (threadData->cancelChannel) :
    );

    // If the thread is a fully detached thread, destroy itself
    //if(threadData->detached == XTHREADS_CREATE_DETACHED){
        /* Because the thread header checking in xthreads_create will check threadId == 0
         * if we adjust the detached state before adjusting the threadId, we will not run into
         * race conditions
         */
    //printf("freeting\n");
    threadData->detached = 0;
    threadData->threadId = 0;
    threadData->resourceId = 0;
    __asm__ __volatile__("freet");
    //}
    /*
    // Otherwise mark that it's ready to be cleaned
    threadData->detached = XTHREADS_READY_DETACHED;
    // Wait until joined
    asm("ssync");
    // threadData->threadId will be set to zero by the parent thread when calling xthreads_create() later in time
    // Hopefully.
    return;*/
}

