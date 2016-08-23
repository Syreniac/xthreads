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


void xthreads_cleanup_test(void *args){
    printf("testing cleanup %p\n", args);
    return;
}

typedef struct xthreads_task_test_struct{
    xthreads_rwlock2_t *rwlock2;
    xthreads_barrier_t *barrier;
    xthreads_mutex2_t *mutex2;
    xthreads_once_t *once;
} xthreads_task_test_struct_t;


void xthreads_task_test_sub(void* args){
    printf("once\n");
}


void *xthreads_task_test(void* args){
    xthreads_setcanceltype(DEFERRED, NULL);
    xthreads_setcancelstate(ENABLED, NULL);
    xthreads_task_test_struct_t *str = (xthreads_task_test_struct_t*) args;
    //xthreads_rwlock2_t *rwlock2 = str->rwlock2;
    xthreads_mutex2_t *mutex2 = str->mutex2;
    xthreads_barrier_t *barrier = str->barrier;
    xthreads_t currentThreadId = xthreads_self();

    //xthreads_once(str->once,xthreads_task_test_sub);

    if(currentThreadId % 1 == 0){
        int i = xthreads_mutex2_timedlock(mutex2,500000);
        if(i != XTHREADS_ETIMEDOUT){xthreads_mutex2_unlock(mutex2);}
    }
    else{
        xthreads_mutex2_lock(mutex2);
        xthreads_mutex2_unlock(mutex2);
    }

    xthreads_testcancel();
    //printf("%d\n",currentThreadId);
    xthreads_barrier_wait(barrier);

    return (void*)(long)0xb00b;
}

void *xthreads_task_test2(void *args){
    xthreads_task_test_struct_t *str = (xthreads_task_test_struct_t*) args;
    xthreads_mutex2_t *mutex2 = str->mutex2;
    xthreads_barrier_t *barrier = str->barrier;
    //xthreads_t currentThreadId = xthreads_self();

    xthreads_mutex2_lock(mutex2);
    xthreads_mutex2_unlock(mutex2);
    xthreads_barrier_wait(barrier);

    return &xthreads_task;

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
    printf("self: %d\n",xthreads_self());
}

int main(void){
    xthreads_barrierattr_t count = 5;
    xthreads_barrier_t *barrier = malloc(sizeof(xthreads_barrier_t));
    xthreads_attr_t attributes;
    xthreads_t thread;
    xthreads_t thread2;
    attributes.detachState = XTHREADS_CREATE_UNDETACHED;
    xthreads_rwlock2_t *rwlock2 = malloc(sizeof(xthreads_rwlock2_t));
    xthreads_mutex2_t *mutex2 = malloc(sizeof(xthreads_mutex2_t));
    xthreads_task_test_struct_t *arg = malloc(sizeof(xthreads_task_test_struct_t));
    xthreads_once_t once = XTHREADS_ONCE_INIT;
    arg->rwlock2 = rwlock2;
    arg->barrier = barrier;
    arg->mutex2 = mutex2;
    arg->once = &once;

    xthreads_rwlock2_init(rwlock2,NULL);
    xthreads_barrier_init(barrier,&count);
    xthreads_mutex2_init(mutex2,NULL);
    xthreads_mutex2_lock(mutex2);

    xthreads_create(&thread, &attributes,&xthreads_task_test,arg);
    xthreads_create(&thread2, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    xthreads_create(NULL, &attributes,&xthreads_task_test,arg);
    register xthreads_resource_t timer asm("r1");
    xthreads_time_t time;
    __asm__ __volatile__(
            "getr %0, 1\n"
            "in %1, res[%2]\n"
            : "=r" (timer), "=r" (time) : "r" (timer) :
    );
    xthreads_cancel(thread);
    xthreads_mutex2_unlock(mutex2);
    void *retu = NULL;
    printf("%p\n",xthreads_join(thread,&retu));
    printf("%p\n",retu);
    xthreads_time_t time2;
    __asm__ __volatile__(
            "in %0, res[%1]\n"
            : "=r" (time2) : "r" (timer) :
    );
    printf("done %d\n",time2-time);
}
