/*
 * xthreads_cleanup.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_self.h"
#include "xthreads_access.h"
#include "xthreads_update.h"
#include <stdlib.h>

void xthreads_cleanup_push(void (*routine)(void*), void *arg){
    xthreads_t self = xthreads_self();
    xthreads_cleanup_function_t *currentStackTop = xthreads_access_cleanup(self);
    xthreads_cleanup_function_t *newStackTop = malloc(sizeof(xthreads_cleanup_function_t));
    newStackTop->function = routine;
    newStackTop->arg = arg;
    newStackTop->next = currentStackTop;

    xthreads_update_cleanup(self,newStackTop);
    //return currentStackTop;
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

void xthreads_cleanup_drop(xthreads_cleanup_function_t *predrop){
    xthreads_cleanup_function_t *drop = predrop->next;
    xthreads_cleanup_function_t *postdrop = drop->next;
    predrop->next = postdrop;
    free(drop);
}

