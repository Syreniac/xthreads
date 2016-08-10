/*
 * xthreads_keys.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_errors.h"
#include <stdlib.h>

#define XTHREADS_KEY_INUSE 0xb
#define XTHREADS_KEY_NOTINUSE 0xe

typedef int xthreads_key_t;

/*static int xthreads_countActiveKeys(void){
    int count = 0;
    for(int i = 0; i < NUM_OF_KEYS; i++){
        if(xthreads_globals.keys[i] != NULL){
            count++;
        }
    }
    return count;
}*/

static int xthreads_findOpenKey(void){
    for(int i = 0; i < NUM_OF_KEYS; i++){
        if(xthreads_globals.keys[i] != NULL){
            return i;
        }
    }
    return -1;
}

void xthreads_key_blank(void* pointer){
    return;
}

void lock(){
    // Experimentally, I'm going to have a lock for the thread headers
    if(xthreads_globals.lock == 0){
        __asm__ __volatile__("getr %0, 5" : "=r" (xthreads_globals.lock) : :);
    }
    __asm__ __volatile__("in r0, res[%0]" : : "r" (xthreads_globals.lock) : "r0");
    return;
}

void unlock(){
    __asm__ __volatile__("out res[%0], r0" : : "r" (xthreads_globals.lock) : "r0" );
}


int xthreads_key_create(xthreads_key_t *key, void (*destructor)(void*)){

    lock();
    int openKey = xthreads_findOpenKey();
    if(openKey != -1){
        // Maximum key count exceeded
        unlock();
        return XTHREADS_EAGAIN;
    }
    xthreads_globals.keys[openKey] = &xthreads_key_blank;
    unlock();

    if(destructor != NULL){
        xthreads_globals.keys[openKey] = destructor;
    }

    *key = openKey;
    return XTHREADS_ENONE;
}

int xthreads_key_delete(xthreads_key_t *key){
    xthreads_globals.keys[*key] = NULL;
    return XTHREADS_ENONE;
}

void *xthreads_getspecfic(xthreads_key_t *key){
    xthreads_t currentThreadId = xthreads_self();
    // If this isn't initialised properly, who even knows what you're going to get. Probably 0, but it'll run onto the thread's stack.
    // The standard says that there is no specified behavior for what would happen in this situation
    return xthreads_globals.stacks[currentThreadId].data.keys[*key];
}

void xthreads_setspecific(xthreads_key_t *key, const void* value){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_globals.stacks[currentThreadId].data.keys[*key] = (void*)value;
}
