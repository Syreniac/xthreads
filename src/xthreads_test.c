/*
 * xthreads_test.c
 *
 *  Created on: 31 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_create.h"
#include "xthreads_joindetach.h"

#define TEST_THREAD_COUNT 7

void *xthreads_test_task(void){
    __asm__(
            "ldc r0, 0x0\n"
            "ldc r2, 0xffff\n"
            "eq r1, r0, r2\n"
            "bt r1, 0x3\n"
            "add r0, r0, 1\n"
            "bu -0x4"
            : : : "r0", "r1", "r2"
    );
    return NULL;
}

void *xthreads_test_task2(int i){
    int a = 0;
    int b = 0;
    __asm__(
            "ldc %1, 0x0\n"
            "eq %2, %1, %0\n"
            "bt %2, 0x3\n"
            "add %1, %1, 1\n"
            "bu -0x4"
            : : "r" (i), "r" (a), "r" (b) :
    );
    return NULL;
}

int main(void){
    xthreads_attr_t attributes;
    xthreads_attr_t attributes2;
    attributes.detachState = XTHREADS_CREATE_UNDETACHED;
    attributes2.detachState = XTHREADS_CREATE_DETACHED;
    xthreads_t thread;
    unsigned int timeA = 0;
    unsigned int timeB = 0;
    unsigned int timeC = 0;
    xthreads_resource_t timer = 0;

    __asm__ __volatile__("getr %0, 1\n" : "=r" (timer) : :);
    __asm__ __volatile__("in %0, res[%1]" : "=r" (timeA) : "r" (timer) :);

    for(int i = 0; i < TEST_THREAD_COUNT - 1; i++){
        xthreads_create(NULL, &attributes2, &xthreads_test_task, NULL);
    }
    xthreads_create(&thread, &attributes, &xthreads_test_task, NULL);
    __asm__ __volatile__("in %0, res[%1]" : "=r" (timeB) : "r" (timer) :);
    //xthreads_test_task();
    xthreads_join(thread,NULL);
    __asm__ __volatile__("in %0, res[%1]" : "=r" (timeC) : "r" (timer) : );
    printf("creation time: %d\n", timeB - timeA);
    printf("join time: %d\n", timeC - timeB);
    printf("done!");
    return 0;
}
/*
int main(void){
    int i = 0;
    while(1){
        printf("loop started\n");
        i = (i + 1) % 5000;
        volatile xthreads_t thread asm("r11");
        xthreads_create(&thread, NULL, xthreads_test_task, NULL);
        xthreads_test_task2(i);
        printf("thread? %d\n", thread);
        xthreads_join(thread,NULL);
        printf("done\n");
    }
}
*/
