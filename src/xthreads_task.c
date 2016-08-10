/*
 * xthreads_task.c
 *
 *  Created on: 4 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_task.h"
#include "xthreads_self.h"
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

    void *returnedValue = 0x0;

    __asm__ __volatile__("getr %0, 2" : "=r" (threadData->threadChannel) : : );
    __asm__ __volatile__("getr %0, 2" : "=r" (threadData->cancelChannel) : : );
    if(threadData->threadChannel == 0 || threadData->cancelChannel == 0){
        printf("xthreads exception: unable to acquire channel for thread\n");
        exit(1);
    }

    returnedValue = target((void*)args);
    xthreads_task_end(threadData, returnedValue);
}

void xthreads_task_endsudden(void){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_data_t *threadData = &xthreads_globals.stacks[currentThreadId].data;
    xthreads_task_end(threadData,NULL);
}

void xthreads_task_end(xthreads_data_t *threadData, void* returnedValue){

    xthreads_task_cleanup(threadData);

    // Free the thread's internal channel
    __asm__ __volatile__(
            "edu res[%0]\n"
            "edu res[%1]\n"
            "freer res[%0]\n"
            "freer res[%1]"
            : : "r" (threadData->threadChannel), "r" (threadData->cancelChannel) :
    );

    // If the thread is a fully detached thread, destroy itself
    if(threadData->detached == XTHREADS_CREATE_DETACHED){
        /* Because the thread header checking in xthreads_create will check threadId == 0
         * if we adjust the detached state before adjusting the threadId, we will not run into
         * race conditions
         */
        threadData->detached = 0;
        threadData->threadId = 0;
        threadData->resourceId = 0;
        threadData->detached = XTHREADS_FINISHED;
        __asm__ __volatile__(
          "freet"
        );
        return;
    }
    threadData->returnedValue = returnedValue;
    // Otherwise mark that it's ready to be cleaned
    threadData->detached = XTHREADS_READY_DETACHED;
    // Wait until joined
    asm("ssync");
    // threadData->threadId will be set to zero by the parent thread when calling xthreads_create() later in time
    // Hopefully.
    return;
}

