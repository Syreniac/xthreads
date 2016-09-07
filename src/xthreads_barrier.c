/*
 * xthreads_barrier.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_channelstack.h"
#include "xthreads_barrier.h"
#include "xthreads_access.h"
#include <assert.h>

/* The current barrier implementation is a hybridisation of channel and spinlocking.
 * Theoretically, this means that all the threads should resume as close as possible to the same time,
 * whilst also having inactive threads paused for minimal overhead.
 */

#define XTHREADS_BARRIER_WAIT 0xAA
#define XTHREADS_BARRIER_SPINREADY 0xDD
#define XTHREADS_BARRIER_CONTINUE 0xFF

#define USING_HYBRID 0

static int xthreads_barrier_wait_eqless_than_1(volatile xthreads_barrier_t *barrier);
static int xthreads_barrier_eq_to_2(volatile xthreads_barrier_t *barrier);
static int xthreads_barrier_gt_2(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId);
static int xthreads_barrier_gt_2_first_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId);
static int xthreads_barrier_gt_2_final_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId);
#if USING_HYBRID == 1
static int xthreads_barrier_gt_2_penultimate_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId);
#endif
static int xthreads_barrier_gt_2_generic_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId);

int xthreads_barrier_init(xthreads_barrier_t *barrier, xthreads_barrierattr_t *attr){
    int x = XTHREADS_NOTHREAD;
    barrier->templocked = XTHREADS_SPINBLOCK;
    barrier->desiredCount = *attr;
    barrier->currentCount = 0;
    xthreads_channelstack_init(&barrier->channelStack);
    __asm__ __volatile__("stw %0, %1[0]" : : "r" (x), "r" (barrier) : );
    return XTHREADS_ENONE;
}

void xthreads_barrier_reset(volatile xthreads_barrier_t *barrier){
    int x = XTHREADS_NOTHREAD;
    barrier->templocked = XTHREADS_SPINBLOCK;
    barrier->currentCount = 0;
    xthreads_channelstack_init(&barrier->channelStack);
    // If you do this in normal C, the compiler will remove the previous lock!
    __asm__ __volatile__("stw %0, %1[0]" : : "r" (x), "r" (barrier) : );
}

int xthreads_barrier_wait(volatile xthreads_barrier_t *barrier){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_spin_lock_inner((xthreads_spin_t*)barrier,currentThreadId);
    barrier->currentCount++;
    // Don't know why you'd ever have a barrier for <=1 thread, but we'll catch it just in case
    if(barrier->desiredCount <= 1){
        return xthreads_barrier_wait_eqless_than_1(barrier);
    }

    if(barrier->desiredCount == 2){
        return xthreads_barrier_eq_to_2(barrier);
    }
    // Otherwise, we get to do some channel shenanigans

    //printf("%d v %d\n",barrier->templocked,currentThreadId);

    return xthreads_barrier_gt_2(barrier,currentThreadId);
}

static int xthreads_barrier_wait_eqless_than_1(volatile xthreads_barrier_t *barrier){
    xthreads_barrier_reset(barrier);
    return XTHREADS_BARRIER_SERIAL_THREAD;
}

static int xthreads_barrier_eq_to_2(volatile xthreads_barrier_t *barrier){

    // A two thread barrier is a special case - as there is no need for channel communication
    // and the first thread can be always left spinning
    if(barrier->currentCount == 1){
        // Wait on cont to be flipped
        xthreads_spin_unlock_inner((xthreads_spin_t*)barrier);
        while(barrier->cont != 1){}
        xthreads_barrier_reset(barrier);
        return XTHREADS_BARRIER_SERIAL_THREAD;
    }
    else{
        // flip cont and continue
        barrier->cont = 1;
        xthreads_spin_unlock_inner((xthreads_spin_t*)barrier);
        return XTHREADS_ENONE;
    }
}

static int xthreads_barrier_gt_2_first_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){
    xthreads_channel_t threadChannel = xthreads_access_threadChannel(currentThreadId);
    barrier->cont = XTHREADS_BARRIER_WAIT;
    xthreads_channelstack_start(&barrier->channelStack,threadChannel,(xthreads_spin_t*)barrier);
    return XTHREADS_BARRIER_SERIAL_THREAD;
}

static int xthreads_barrier_gt_2_final_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){

    barrier->currentCount = 0;
    xthreads_channel_t threadChannel = xthreads_access_threadChannel(currentThreadId);
    xthreads_channelstack_cascade(&barrier->channelStack, threadChannel,(xthreads_spin_t*)barrier);
    xthreads_spin_unlock_inner((xthreads_spin_t*)barrier);
    return XTHREADS_ENONE;
}

static int xthreads_barrier_gt_2_generic_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){
    xthreads_channel_t threadChannel = xthreads_access_threadChannel(currentThreadId);
    xthreads_channelstack_pushwait(&barrier->channelStack,threadChannel,(xthreads_spin_t*)barrier);
    return XTHREADS_ENONE;
}

static int xthreads_barrier_gt_2(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){

    if(barrier->currentCount == 1){
        // Case 1 - the first thread hits the barrier
        return xthreads_barrier_gt_2_first_thread(barrier,currentThreadId);
    }
    else if(barrier->currentCount == barrier->desiredCount){
        return xthreads_barrier_gt_2_final_thread(barrier,currentThreadId);
    }
    else{
        // Case 4 - any other thread hits the barrier
        return xthreads_barrier_gt_2_generic_thread(barrier,currentThreadId);
    }
}
