/*
 * xthreads_joindetach.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_util.h"
#include <stdlib.h>
#include <stdio.h>

void xthreads_detach(xthreads_t threadId){
    // Bitshift magic to get the lowest byte
    xthreads_data_t* threadData = (xthreads_data_t*)(&xthreads_globals.stacks[threadId]);
    int threadType = threadData->resourceId & 0xff;

    /* Synchronised threads are handled through the synchroniser id rather than the thread id
     * which is identified by having a 4 in the lowest byte of the thread id.
     */
    if(threadType == 3){

        if(threadData->detached == XTHREADS_CREATE_UNDETACHED){
            threadData->detached = XTHREADS_LATER_DETACHED;
        }
        else if(threadData->detached == XTHREADS_READY_DETACHED){
            __asm__ __volatile__("msync res[%0]" : : "r" (threadData->threadId) :);
        }
    }
    else{
        printf("XThreads exception: undetachable thread id: ");
        xthreads_printBytes(threadId);
        printf("\n");
    }
    return;
}

void *xthreads_join(xthreads_t threadId){
    // Bitshift magic to get the lowest byte
    xthreads_data_t* threadData = (xthreads_data_t*)(&xthreads_globals.stacks[threadId]);
    int threadType = threadData->resourceId & 0xff;
    void *returnedValue = NULL;

    printf("thread type: %d\n",threadType);

    if(threadType == 3){
        /* When a synchronise thread is created, it waits in a ssync state until
         * a msync command is received.
         *
         * We have to actually start the thread before calling mjoin, because mjoin waits until
         * the thread is ssync-ing before continuing; if the thread is never started,
         * mjoin returns immediately and the thread stops.
         */
        __asm__ __volatile__(
                "mjoin res[%0]\n"
                : : "r" (threadData->resourceId) :
        );

        returnedValue = threadData->returnedValue;
        threadData->threadId = 0;
        threadData->detached = 0;
        return returnedValue;
    }
    else{
        printf("XThreads exception: unjoinable resource id: ");
        xthreads_printBytes(threadType);
        printf("\n");
        exit(1);
    }
    return NULL;
}
