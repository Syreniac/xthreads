/*
 * xthreads_cleanup.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_self.h"
#include <stdlib.h>

void xthreads_cleanup_push(void (*routine)(void*), void *arg){
    int currentThreadId = xthreads_self();
    xthreads_cleanup_function_t *currentStackTop = xthreads_globals.stacks[currentThreadId].data.cleanup;
    xthreads_cleanup_function_t *newStackTop = malloc(sizeof(xthreads_cleanup_function_t));
    newStackTop->function = routine;
    newStackTop->arg = arg;
    newStackTop->next = currentStackTop;

    xthreads_globals.stacks[currentThreadId].data.cleanup = newStackTop;
}

void xthreads_cleanup_pop(int execute){
    int currentThreadId = xthreads_self();
    xthreads_cleanup_function_t *currentStackTop = xthreads_globals.stacks[currentThreadId].data.cleanup;

    xthreads_globals.stacks[currentThreadId].data.cleanup = currentStackTop->next;
    if(execute == 1){
        currentStackTop->function(currentStackTop->arg);
    }
    free(currentStackTop);
}

