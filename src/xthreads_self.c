/*
 * xthreads_self.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_self.h"

/* Really gets the index of the stack space of the calling thread, but that functions as the id
 * in the current structure
 */
xthreads_t xthreads_self(void){
    int i = 0;
    register int volatile currentThreadId = 0;
    __asm__ __volatile__("get r11, id" : : : "r11");
    __asm__ __volatile__("mov %0, r11" : "=r" (currentThreadId) : : );

    while(i < NUM_OF_THREADS){
        if(xthreads_globals.stacks[i].data.threadId == currentThreadId){
            return i;
        }
        i++;
    }

    return XTHREADS_NOTHREAD;
}

xthreads_t xthreads_self_outer(void){
    return xthreads_self() % NUM_OF_THREADS;
}
// TODO - update this to work with numerous threads
