/*
 * xthreads.xc
 *
 *  Created on: 29 May 2016
 *      Author: Matthew
 */
#include <stdio.h>
#include <stdlib.h>
#include "xthreads.h"
#include "xthreads_rwlock2.h"
#include "xthreads_mutex2.h"
#include "xthreads_barrier.h"
#include "xthreads_self.h"
#include "xthreads_create.h"
#include "xthreads_joindetach.h"
#include "xthreads_cancel.h"
#include "xthreads_once.h"

xthreads_global_t xthreads_globals;

void xthreads_task(void *(*target)(void*), void* args, xthreads_data_t* cleanup);

/* These two functions *must* be called in pairs or BAD THINGS will happen
 * Try to minimise the time spent inside the lock - most things should only need it when searching the thread headers
 * and should select and mark things in such a way that there is no need
 *
 */

// ---------------------------------------------------------------------------------------------------


/*
void xthreads_cleanup_test(void *args){
    printf("testing cleanup %p\n", args);
    return;
}

typedef struct xthreads_task_test_struct{
    xthreads_rwlock2_t *rwlock2;
    xthreads_barrier_t *barrier;
    xthreads_mutex2_t *mutex2;
    xthreads_once_t *once;
    xthreads_resource_t resource;
    xthreads_spin_t *spin;
} xthreads_task_test_struct_t;

void xthreads_task_test_sub(){
    register int i = 0;
    while(i < 500){
        __asm__ __volatile__("add %0, %1, 1" : "=r" (i) : "r" (i) : );
    }
}


__attribute__ ((optimize("-O0"))) void *xthreads_task_test(void* args){
    xthreads_setcanceltype(DEFERRED, NULL);
    xthreads_setcancelstate(ENABLED, NULL);
    xthreads_task_test_struct_t *str = (xthreads_task_test_struct_t*) args;
    xthreads_mutex2_t *mutex2 = str->mutex2;
    xthreads_barrier_t *barrier = str->barrier;

    xthreads_mutex2_timedlock(mutex2,0);
    xthreads_task_test_sub();
    xthreads_mutex2_unlock(mutex2);
    xthreads_barrier_wait(barrier);

    return (void*)(long)0xb00b;
}

void *xthreads_task_test2(void *args){
    xthreads_setcanceltype(DEFERRED, NULL);
    xthreads_setcancelstate(ENABLED, NULL);
    xthreads_task_test_struct_t *str = (xthreads_task_test_struct_t*) args;
    xthreads_barrier_t *barrier = str->barrier;
    xthreads_resource_t resource = str->resource;
    //xthreads_t currentThreadId = xthreads_self();

    __asm__ __volatile__("in r10, res[%0]\n" : : "r" (resource) : "r10");
    xthreads_task_test_sub();
    __asm__ __volatile__("out res[%0], r10\n": : "r" (resource) : "r10");

    xthreads_barrier_wait(barrier);

    return (void*)(long)0xb00b;
}

__attribute__ ((optimize("-O0"))) void *xthreads_task_test3(void *args){
    xthreads_setcanceltype(DEFERRED, NULL);
    xthreads_setcancelstate(ENABLED, NULL);
    xthreads_task_test_struct_t *str = (xthreads_task_test_struct_t*) args;
    xthreads_barrier_t *barrier = str->barrier;

    xthreads_task_test_sub();
    xthreads_barrier_wait(barrier);

    return (void*)(long)0xb00b;

}

__attribute__ ((optimize("-O0"))) void *xthreads_task_test4(void *args){
    xthreads_setcanceltype(DEFERRED, NULL);
    xthreads_setcancelstate(ENABLED, NULL);
    xthreads_task_test_struct_t *str = (xthreads_task_test_struct_t*) args;
    xthreads_barrier_t *barrier = str->barrier;

    xthreads_spin_lock(str->spin);
    xthreads_task_test_sub();
    xthreads_spin_unlock(str->spin);
    xthreads_barrier_wait(barrier);

    return (void*)(long)0xb00b;

}

__attribute__ ((constructor)) void xthreads_configureMain(){
    printf("configuring main\n");
    xthreads_t mainThreadId = 0;
    __asm__ __volatile__(
            "get r11, id\n"
            "mov %0, r11\n"
            : "=r" (mainThreadId) : : "r11"
    );
    xthreads_globals.stacks[0].data.threadId = mainThreadId;
    xthreads_globals.stacks[0].data.resourceId = 0xffffffff;
    xthreads_channel_t threadChannel = 0;
    xthreads_channel_t cancelChannel = 0;
    __asm__ __volatile__(
            "getr %0, 2\n"
            "getr %1, 2\n"
            : "=r" (threadChannel), "=r" (cancelChannel) : :
    );
#if SELF_USING_KEP==1
    __asm__ __volatile__("ldc r11, 0x0");
    __asm__ __volatile__("set kep, r11\n");
#endif
    xthreads_globals.stacks[0].data.threadChannel = threadChannel;
    xthreads_globals.stacks[0].data.cancelChannel = cancelChannel;
}

#define XTHREADS_LOCK_TEST_COUNT 7
int main(void){

    xthreads_attr_t attributes;
    xthreads_attr_t attributes2;
    attributes.detachState = XTHREADS_CREATE_UNDETACHED;
    attributes2.detachState = XTHREADS_CREATE_DETACHED;
    xthreads_t thread;
    xthreads_barrier_t *barrier = malloc(sizeof(xthreads_barrier_t));
    xthreads_mutex2_t *mutex2 = malloc(sizeof(xthreads_mutex2_t));
    xthreads_task_test_struct_t *arg = malloc(sizeof(xthreads_task_test_struct_t));
    xthreads_spin_t *spin = malloc(sizeof(xthreads_spin_t));
    int i = 0;

    arg->barrier = barrier;
    arg->mutex2 = mutex2;
    arg->spin = spin;
    register xthreads_resource_t timer asm("r1");

    volatile int dummyInt = 0;

    __asm__ __volatile__("getr %0, 1\n" : "=r" (timer) : :);

    __asm__ __volatile__("getr %0, 5\n" : "=r" (arg->resource) : : );


    xthreads_time_t time;
    xthreads_time_t time2;

    __asm__ __volatile__("in %0, res[%1]" : "=r" (time) : "r" (timer) :);
    xthreads_task_test_sub();
    __asm__ __volatile__("in %0, res[%1]" : "=r" (time2) : "r" (timer) :);

    printf("subtask done %d\n",time2-time);


    for(int j = 1; j <= XTHREADS_LOCK_TEST_COUNT; j++){
        printf("testing %d threads\n",j);
        xthreads_barrierattr_t count = j;
        xthreads_barrier_init(barrier,&count);
        printf("%d\n",barrier->desiredCount);
        xthreads_spin_init(spin);
        xthreads_spin_lock(spin);

        for(i = 0; i < j - 1; i++){
            xthreads_create(NULL, &attributes2, &xthreads_task_test3, arg);
        }
        xthreads_create(&thread, &attributes, &xthreads_task_test3, arg);

        __asm__ __volatile__("in %0, res[%1]" : "=r" (time) : "r" (timer) :);
        xthreads_join(thread, NULL);
        __asm__ __volatile__("in %0, res[%1]" : "=r" (time2) : "r" (timer) :);

        printf("barrier done %d\n",time2-time);


        xthreads_mutex2_init(mutex2,NULL);
        xthreads_mutex2_lock(mutex2);

        for(i = 0; i < j - 1; i++){
            xthreads_create(NULL, &attributes2, &xthreads_task_test, arg);
        }
        xthreads_create(&thread, &attributes,&xthreads_task_test,arg);
        //xthreads_cancel(thread);
        xthreads_task_test_sub();
        __asm__ __volatile__("in %0, res[%1]\n" : "=r" (time) : "r" (timer) :);
        xthreads_mutex2_unlock(mutex2);
        xthreads_join(thread,NULL);
        __asm__ __volatile__(
                "in %0, res[%1]\n"
                : "=r" (time2) : "r" (timer) :
        );

        printf("mutex done %d\n",time2-time);

        __asm__ __volatile__(
                "in %0, res[%1]\n"
                : "=r" (time) : "r" (arg->resource):
        );

        for(i = 0; i < j - 1; i++){
            xthreads_create(NULL, &attributes2, &xthreads_task_test2, arg);
        }
        xthreads_create(&thread, &attributes, &xthreads_task_test2, arg);
        xthreads_task_test_sub();
        __asm__ __volatile__("in %0, res[%1]\n" : "=r" (time) : "r" (timer) :);
        __asm__ __volatile__("out res[%0], r0" : : "r" (arg->resource) :);
        xthreads_join(thread,NULL);
        __asm__ __volatile__(
                "in %0, res[%1]\n"
                : "=r" (time2) : "r" (timer) :
        );

        printf("hwlock done %d\n",time2-time);

        for(i = 0; i < j - 1; i++){
            xthreads_create(NULL, &attributes2, &xthreads_task_test4, arg);
        }
        xthreads_create(&thread, &attributes, &xthreads_task_test4, arg);
        xthreads_task_test_sub();
        __asm__ __volatile__("in %0, res[%1]\n" : "=r" (time) : "r" (timer) :);
        xthreads_spin_unlock(spin);
        xthreads_join(thread,NULL);
        __asm__ __volatile__(
                "in %0, res[%1]\n"
                : "=r" (time2) : "r" (timer) :
        );

        printf("spin done %d\n",time2-time);
        printf("-------------------------\n");
    }

    printf("done!\n");


}*/
