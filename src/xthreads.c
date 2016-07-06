/*
 * xthreads.xc
 *
 *  Created on: 29 May 2016
 *      Author: Matthew
 */
#include <stdio.h>
#include <stdlib.h>
#include "xthreads.h"

xthreads_stack_t xthreads_Stacks[NUM_OF_THREADS];

void xthreads_task(void);

void printBytes(int i){
    printf("%x",(i & 0xff));
    printf("%x",((i >> 8) & 0xff));
    printf("%x",((i >> 16) & 0xff));
    printf("%x",((i >> 24) & 0xff));
}

void xthreads_create(xthreads_t *thread, const xthreads_attr_t *attr, void *(start_routine)(void*), void *arg){
    register volatile int synchroniser asm("r1")  = 0;
    register volatile int res asm("r2") = 0;
    void* startPtr = &xthreads_task;
    char* stackPtr = NULL;
    xthreads_data_t* cleanup = NULL;

    if(attr->detachState == XTHREADS_CREATE_DETACHED){
        printf("case 1\n");
        __asm__ __volatile__(
          "getr %0, 4\n"
          : "=r" (res) : :
        );
        if(res == 0){
            printf("xthreads exception: unable to create unsynchronised thread.\n");
        }
    }
    else{
        printf("case 2\n");
        __asm__ __volatile__(
          "getr %0, 3\n"
          "getst %1, res[%2]\n"
          : "=r" (synchroniser), "=r" (res) : "r" (synchroniser) :
        );

        if(res == 0){
            printf("xthreads exception: unable to create synchronised thread.\n");
            printf("Most likely, this is because you are using more synchronisers than are available\n");
            exit(1);
        }
    }

    int i = 0;
    while(i < XTHREADS_THREAD_CAPACITY && stackPtr == NULL){
        if(xthreads_Stacks[i].threadId == 0){
            xthreads_Stacks[i].threadId = -1;
            stackPtr = &xthreads_Stacks[i].bytes[THREAD_STACK_SIZE];
            xthreads_Stacks[i].detached = attr->detachState;
            cleanup = (xthreads_data_t*)(&xthreads_Stacks[i]);
        }
        else{
            i++;
        }
    }

    if(stackPtr == NULL){
        printf("xthreads exception: maximum thread capacity exceeded\n");
        exit(1);
    }

    asm volatile(
            "init t[%0]:sp, %1\n"
            "init t[%0]:pc, %2\n"
            "set t[%0]:r0, %3\n"
            "set t[%0]:r1, %4\n"
            "set t[%0]:r2, %5\n"
            : : "r" (res), "r" (stackPtr), "r" (startPtr), "r" (start_routine), "r" (arg), "r" (cleanup):
    );

    if(attr->detachState == XTHREADS_CREATE_DETACHED){
        xthreads_Stacks[i].threadId = res;
        __asm__ __volatile__(
          "start t[%0]"
          : : "r" (res) :
        );
    }
    else{
        xthreads_Stacks[i].threadId = synchroniser;
        __asm__ __volatile__(
          "msync res[%0]"
          : : "r" (synchroniser) :
        );
    }

    *thread = i;
}

void xthreads_task_test(void* args){
    printf("subtask\n");
    return;
}

void xthreads_task(void){
    // These are all deliberately left uninitialised
    void *(*target)(void*);
    void* args;
    xthreads_data_t* cleanup;

    __asm__ __volatile__(
      "mov %0, r0\n"
      "mov %1, r1\n"
      "mov %2, r2\n"
      : "=r" (target), "=r" (args), "=r" (cleanup) : : "r1","r2","r3"
    );

    target((void*)args);

    if(cleanup->detached != XTHREADS_CREATE_UNDETACHED){
        cleanup->detached = 0;
        cleanup->threadId = 0;
        __asm__ __volatile__(
          "freet"
        );
        return;
    }
    cleanup->detached = XTHREADS_READY_DETACHED;
    asm("ssync");

}

void xthreads_detach(xthreads_t threadId){
    // Bitshift magic to get the lowest byte
    xthreads_data_t* threadData = (xthreads_data_t*)(&xthreads_Stacks[threadId]);
    int threadType = threadData->threadId & 0xff;

    /* Synchronised threads are handled through the synchroniser id rather than the thread id
     * which is identified by having a 4 in the lowest byte of the thread id.
     */
    if(threadType == 3){
        if(threadData->detached == XTHREADS_CREATE_UNDETACHED){
            threadData->detached = XTHREADS_CREATE_DETACHED;
        }
        else if(threadData->detached == XTHREADS_READY_DETACHED){
            __asm__ __volatile__("msync res[%0]" : : "r" (threadData->threadId) :);
        }
    }
    else{
        printf("XThreads exception: undetachable thread id: ");
        printBytes(threadId);
        printf("\n");
    }
    return;
}

void xthreads_join(xthreads_t threadId){
    // Bitshift magic to get the lowest byte
    xthreads_data_t* threadData = (xthreads_data_t*)(&xthreads_Stacks[threadId]);
    int threadType = threadData->threadId & 0xff;

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
                "freer res[%0]"
                : : "r" (threadData->threadId) :
        );

        threadData->threadId = 0;
        threadData->detached = 0;
    }
    else{
        printf("XThreads exception: unjoinable resource id: ");
        printBytes(threadId);
        printf("\n");
    }
}

/*int main(void){
    xthreads_attr_t attributes;
    xthreads_t thread;
    attributes.detachState = XTHREADS_CREATE_UNDETACHED;

    xthreads_create(&thread, &attributes,&xthreads_task_test,NULL);
    xthreads_join(thread);
    printf("end of main\n");
}*/
