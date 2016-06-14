/*
 * xthreads.xc
 *
 *  Created on: 29 May 2016
 *      Author: Matthew
 */
#include <stdio.h>
#include <stdlib.h>
#include "xthreads.h"

#define NUM_OF_THREADS 8
#define THREAD_STACK_SIZE 2000

typedef struct xthreadsThreadStack_t{
    char bytes[THREAD_STACK_SIZE];
} xthreadsThreadStack_t;

int G_INT = 0;
xthreadsThreadStack_t xthreads_Stacks[NUM_OF_THREADS];
int synchroniserIds[NUM_OF_THREADS];

void __attribute__((noinline)) theoreticalThreadTask(void){
    int volatile i = 0;
    while(G_INT == 0){
        printf(">>>> %d\n",G_INT);
        i++;
    }
    printf("hopefully this works %d|",i);
}

void __attribute__ ((noinline)) theoreticalThreadStart(void){
    // Code that the thread will need to call before being able to do anything!
    register volatile void* target = &theoreticalThreadTask;
    printf("start thread\n");
    asm volatile(
            "bla %0\n"
            "ssync"
            : : "r" (target) :
    );
    printf("thread done");

}

void __attribute__ ((noinline)) theoreticalCreateThread(xthreads_t *thread){
    register volatile int synchroniser = 0;
    register volatile int res = 0;
    register volatile void* startPtr asm("r7") = &theoreticalThreadStart;
    register volatile void* funcPtr asm("r8") = &theoreticalThreadTask;
    register volatile char* stackPtr asm("r9") = NULL;//&xthreads_Stacks[i].bytes[0];
    register volatile int threadId asm("r10") = 0;

    asm volatile(
            "getr %0, 3\n"
            "getst %1, res[%3]\n"
            "mov %2, %4\n"
            : "=r" (synchroniser), "=r" (res), "=r" (threadId) : "r" (synchroniser), "r" (res) : "r1","r2","r3"
    );


    printf("debug: %d\n",threadId);
    threadId = threadId >> 8;
    printf("ddebug: %d\n",threadId);
    printf("synchronisers: %d\n",synchroniser);
    synchroniserIds[threadId] = synchroniser;

    if(threadId == 0){
        printf("xthreads exception: unable to create a synchronised thread");
        exit(1);
    }
    else if(threadId >= NUM_OF_THREADS){
        printf("xthreads exception: maximum thread count exceeded");
        exit(1);
    }
    /*
     * Horrible assumption here: that the hardware automatically reuses threadIds and I don't need to manually
     * ensure non-overlapping stacks
     */
    stackPtr = &xthreads_Stacks[threadId].bytes[0];
    /*
     * I'm not entirely sure why, but the program breaks if the stack pointer doesn't have 2000 added to it here
     * This is rather bizarre behaviour and I'm pretty sure it has unintended consequences.
     * The problem seems to be that the program returns from main before the thread can do anything if this is removed
     * which is so utterly unrelated I'm not sure it's even actual behaviour of the Xcore or just a simulator bug of some
     * kind.
     */
    stackPtr += THREAD_STACK_SIZE;
    asm volatile(
           "init t[%0]:sp, %1"
           : : "r" (res), "r" (stackPtr) :
    );

    asm volatile(
            "init t[%0]:pc, %1\n"
            "set t[%0]:r0, %2"
            : : "r" (res), "r" (startPtr), "r" (funcPtr) :
    );
    *thread = threadId;
    /* At this point the thread simply needs to be started by doing asm("msync res[threadId]");*/
}

void __attribute__ ((noinline)) theoreticalDetachThread(xthreads_t threadId){
    int synchroniser = synchroniserIds[threadId];
    threadId = threadId << 8;
    threadId += 4;
    asm volatile(
            "msync res[%0]"
            : : "r" (synchroniser) :
    );
    printf("threadId: %d\n",threadId);
    printf("sync: %d\n",synchroniser);

    /*asm volatile(
            "start t[%0]"
            : : "r" (threadId) :
    );*/
}

void __attribute__ ((noinline)) theoreticalJoinThread(xthreads_t threadId){
    int synchroniser = synchroniserIds[threadId];
    threadId = threadId << 8;
    threadId += 4;
    asm volatile(
            "mjoin res[%0]"
            : : "r" (synchroniser) :
    );
    printf("threadId: %d\n",threadId);
    printf("sync: %d\n",synchroniser);
}

/*
    getr      r4,        3        // get a thread synchroniser, store in r4
    getst     r11,       res[r4]  // get synchronised thread, bound on the thread synchroniser in r4
    init      t[r11]:sp, r1       // set stack pointer of new thread
    init      t[r11]:pc, r0       // set PC of new thread
    set       t[r11]:r0, r2       // set r0, argument of new thread
    set       t[r11]:r1, r3       // set r1, chanend of new thread
    ldaw      r0,        dp[0]    // load value of data pointer in r0
    init      t[r11]:dp, r0       // set data pointer of new thread
    add       r1,        r11,  0  // copy thread resource id to r1
    ldaw      r11,       cp[0]    // load value of constant pointer in r11
    init      t[r1]:cp,  r11      // set constant pointer of new thread
    msync     res[r4]             // start new thread
 */

int main(int argc, char* argv[]){
    printf("A\n");
    xthreads_t exDebug = -1;
    theoreticalCreateThread(&exDebug);
    theoreticalJoinThread(exDebug);
    printf("-------------");
    G_INT = 1;
    while(1){
        G_INT = 1;
    }
    return 0;
}
