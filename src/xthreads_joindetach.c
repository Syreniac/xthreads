/*
 * xthreads_joindetach.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_util.h"
#include "xthreads_access.h"
#include "xthreads_update.h"
#include "xthreads_errors.h"
#include "xthreads_self.h"
#include "xthreads_cancel.h"
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

void xthreads_join_tempcleanup(void* arg){
    xthreads_data_t* data = (xthreads_data_t*)arg;
    data->returnChannel = 0;
}

int xthreads_join(xthreads_t threadId, void** retval){
    xthreads_testcancel();
    volatile xthreads_data_t* data;
    const xthreads_t self = xthreads_self_outer();
    const register xthreads_channel_t selfChannel asm("r10") = xthreads_self_threadChannel(self);
    void* returned;

    if(xthreads_check_access(threadId) == 0){
        return XTHREADS_EINVAL;
    }
    threadId = threadId % NUM_OF_THREADS;
    data = &xthreads_globals.stacks[threadId].data;
    if(data->resourceId == 0){
        return XTHREADS_EINVAL;
    }
    if(data->detached == XTHREADS_CREATE_DETACHED){
        return XTHREADS_EINVAL;
    }

    register xthreads_channel_t returnChannel asm("r9") = data->returnChannel;
    register xthreads_channel_t otherChannel asm("r8") = data->threadChannel;

    switch(returnChannel){
        case -1:
            // The thread is paused waiting for a join
            // We'll try to put ourself in as the return channel
            data->returnChannel = selfChannel;
            __asm__ __volatile__("nop\n");
            returnChannel = data->returnChannel;
            if(returnChannel == selfChannel){
                // We've got in first!
                // Send a message to wake the joinable thread up
                __asm__("setd res[%0], %1\n"
                        "outct res[%0], 1\n"
                        : : "r" (selfChannel), "r" (otherChannel) :);

                __asm__ __volatile__(
                        "in %0, res[%1]\n"
                        "chkct res[%1], 1"
                        : "=r" (returned) : "r" (selfChannel) :
                );

                if(retval != NULL){
                    *retval = returned;
                }

                return XTHREADS_ENONE;
            }
            break;
        case 0:
            data->returnChannel = selfChannel;
            asm("nop\n");
            returnChannel = data->returnChannel;
            if(returnChannel == selfChannel){
                printf("%d\n",data->returnChannel);
                __asm__ __volatile__(
                        "in %0, res[%1]\n"
                        "chkct res[%1], 1"
                        : "=r" (returned) : "r" (selfChannel) :
                );

                if(retval != NULL){
                    *retval = returned;
                }

                return XTHREADS_ENONE;
            }
        default:
            *retval = (void*)(long)XTHREADS_ECANCELLED;
            return XTHREADS_EINVAL;
    }
    return XTHREADS_EINVAL;
}
