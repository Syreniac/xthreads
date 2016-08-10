/*
 * xthreads_self.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"

/* Really gets the index of the stack space of the calling thread, but that functions as the id
 * in the current structure
 */

int xthreads_self(void){
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

    return -1;
}
