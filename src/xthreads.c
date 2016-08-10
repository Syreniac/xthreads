/*
 * xthreads.xc
 *
 *  Created on: 29 May 2016
 *      Author: Matthew
 */
#include <stdio.h>
#include <stdlib.h>
#include "xthreads.h"
#include "xthreads_mutex2.h"
#include "xthreads_barrier.h"
#include "xthreads_self.h"
#include "xthreads_create.h"
#include "xthreads_joindetach.h"

xthreads_global_t xthreads_globals;


void xthreads_task(void *(*target)(void*), void* args, xthreads_data_t* cleanup);

/* These two functions *must* be called in pairs or BAD THINGS will happen
 * Try to minimise the time spent inside the lock - most things should only need it when searching the thread headers
 * and should select and mark things in such a way that there is no need
 *
 */

// ---------------------------------------------------------------------------------------------------


void xthreads_cleanup_test(void *args){
    printf("testing cleanup %p\n", args);
    return;
}

typedef struct xthreads_task_test_struct{
    xthreads_mutex2_t *mutex;
    xthreads_barrier_t *barrier;
} xthreads_task_test_struct_t;

void *xthreads_task_test(void* args){
    xthreads_task_test_struct_t *str = (xthreads_task_test_struct_t*) args;
    xthreads_mutex2_t *mutex = str->mutex;
    xthreads_barrier_t *barrier = str->barrier;
    xthreads_t currentThreadId = xthreads_self();
    char *a = NULL;
    xthreads_cleanup_push(&xthreads_cleanup_test,a+xthreads_self());

    if(xthreads_self() % 2 != 1){
        xthreads_mutex2_lock(mutex);
        for(int i = 0; i < 500; i++){
            if(i % 100 == 0){
                printf("\n<<<<%d\n", i + currentThreadId);
            }
        }
        xthreads_mutex2_unlock(mutex);
    }
    else{
        // Even threads 0,2,4
        int locked = xthreads_mutex2_timedlock(mutex, 50000);
        if(locked == XTHREADS_ENONE){
            for(int i = 0; i < 500; i++){
                if(i % 100 == 0){
                    printf("\n-----%d\n", i + currentThreadId);
                }
            }
            xthreads_mutex2_unlock(mutex);
        }
        //printf("timed test %d\n",locked);
    }
    xthreads_barrier_wait(barrier);

    return &xthreads_task;
}

int main(void){
    xthreads_barrierattr_t count = 6;
    xthreads_barrier_t *barrier = malloc(sizeof(xthreads_barrier_t));
    xthreads_attr_t attributes;
    xthreads_t thread;
    attributes.detachState = XTHREADS_CREATE_UNDETACHED;
    xthreads_mutex2_t *mutex = malloc(sizeof(xthreads_mutex2_t));

    xthreads_mutex2_init(mutex,NULL);
    xthreads_barrier_init(barrier,&count);

    xthreads_task_test_struct_t *arg = malloc(sizeof(xthreads_task_test_struct_t));
    arg->mutex = mutex;
    arg->barrier = barrier;

    xthreads_create(&thread, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_join(thread);
    printf("done\n");
}
