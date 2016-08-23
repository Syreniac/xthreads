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
    /* Because the standard requires that the barrier is reset to the state that it was initialised to, and we
     * can't directly set cont in the reset function (discussed below), we'll have to not set it here to be compliant
     */
    //barrier->cont = XTHREADS_BARRIER_WAIT;
    __asm__ __volatile__("stw %0, %1[0]" : : "r" (x), "r" (barrier) : );
    return XTHREADS_ENONE;
}

void xthreads_barrier_reset(volatile xthreads_barrier_t *barrier){
    int x = XTHREADS_NOTHREAD;
    barrier->templocked = XTHREADS_SPINBLOCK;
    barrier->currentCount = 0;
    xthreads_channelstack_init(&barrier->channelStack);
    // We don't set this here because it's impossible to ensure that it doesn't cause threads to get stuck when reset is called
    // Instead, we set it when the first thread hits the barrier - that way, we ensure that it takes up the minimal amount of 'real' processing time
    //barrier->cont = XTHREADS_BARRIER_WAIT;
    // If you do this in normal C, the compiler will remove the previous lock!
    __asm__ __volatile__("stw %0, %1[0]" : : "r" (x), "r" (barrier) : );
}

int xthreads_barrier_wait(volatile xthreads_barrier_t *barrier){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_spin_lock_inner((xthreads_spin_t*)barrier,currentThreadId);
    barrier->currentCount++;
    // Don't know why you'd ever have a barrier for <=1 thread, but we'll catch it just in case
    // TODO - make the barrier not have to check these ifs every time
    if(barrier->desiredCount <= 1){
        return xthreads_barrier_wait_eqless_than_1(barrier);
    }

    if(barrier->desiredCount == 2){
        return xthreads_barrier_eq_to_2(barrier);
    }

    // Otherwise, we get to do some channel shenanigans

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

static int xthreads_barrier_gt_2(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){

    if(barrier->currentCount == 1){
        // Case 1 - the first thread hits the barrier
        return xthreads_barrier_gt_2_first_thread(barrier,currentThreadId);
    }
    else if(barrier->currentCount == barrier->desiredCount){
        // Case 2 - the final thread hits the barrier
        return xthreads_barrier_gt_2_final_thread(barrier,currentThreadId);
    }
#if USING_HYBRID == 1
    else if(barrier->currentCount == barrier->desiredCount - 1){
        // Case 3 - the penultimate thread hits the barrier
        return xthreads_barrier_gt_2_penultimate_thread(barrier,currentThreadId);
    }
#endif
    else{
        // Case 4 - any other thread hits the barrier
        return xthreads_barrier_gt_2_generic_thread(barrier,currentThreadId);
    }
}

static int xthreads_barrier_gt_2_first_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){
    xthreads_channel_t threadChannel = xthreads_access_threadChannel(currentThreadId);
    // Case 1 - First thread to hit the barrier
    //barrier->targetChannel = threadChannel;
    barrier->cont = XTHREADS_BARRIER_WAIT;
    /*__asm__ __volatile__(
            "chkct res[%0],1\n"
            : : "r" (threadChannel) : "r10");*/
    xthreads_channelstack_start(&barrier->channelStack,threadChannel,(xthreads_spin_t*)barrier);
    // Spin whilst waiting for the final thread to minimise delays
#if USING_HYBRID == 1
    while(barrier->cont != XTHREADS_BARRIER_CONTINUE){}
#endif
    return XTHREADS_BARRIER_SERIAL_THREAD;
}

static int xthreads_barrier_gt_2_final_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){

#if USING_HYBRID == 1
    while(barrier->cont != XTHREADS_BARRIER_SPINREADY){} // Just in case, wait until we have all the threads spinning
    barrier->currentCount = 0;
    barrier->cont = XTHREADS_BARRIER_CONTINUE;
    xthreads_spin_unlock_inner((xthreads_spin_t*)barrier);
#else
    xthreads_channel_t threadChannel = xthreads_access_threadChannel(currentThreadId);
    xthreads_channelstack_cascade(&barrier->channelStack, threadChannel,(xthreads_spin_t*)barrier);
    /*__asm__ __volatile__(
            "setd res[%0], %1\n"
            "outct res[%0], 1"
            : : "r" (threadChannel), "r" (barrier->targetChannel) :
    );*/
    xthreads_spin_unlock_inner((xthreads_spin_t*)barrier);
#endif
    return XTHREADS_ENONE;
}

#if USING_HYBRID == 1
static int xthreads_barrier_gt_2_penultimate_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){
    xthreads_channel_t threadChannel = xthreads_access_threadChannel(currentThreadId);

    // Message the threads telling them to wake and start spinning before the final thread hits
    /*__asm__ __volatile__(
            "setd res[%0], %1\n"
            "outct res[%0], 1"
            : : "r" (threadChannel), "r" (barrier->targetChannel) :
    );*/
    xthreads_channelstack_cascade(&barrier->channelStack,threadChannel,(xthreads_spin_t*)barrier);
    barrier->cont = XTHREADS_BARRIER_SPINREADY;
    xthreads_spin_unlock_inner((xthreads_spin_t*)barrier);
    while(barrier->cont != XTHREADS_BARRIER_CONTINUE){}
    return XTHREADS_ENONE;
}
#endif

static int xthreads_barrier_gt_2_generic_thread(volatile xthreads_barrier_t *barrier, xthreads_t currentThreadId){
    xthreads_channel_t threadChannel = xthreads_access_threadChannel(currentThreadId);

    // Case 4 - other threads hit barrier
    // Wait for the next thread in the stack to message us, then pass the message on to the next
    /*__asm__ __volatile__(
            "setd res[%0], %1\n"
            "chkct res[%0], 1\n"
            "outct res[%0], 1"
            : : "r" (threadChannel), "r" (destChannel) :
    );*/
    xthreads_channelstack_pushwait(&barrier->channelStack,threadChannel,(xthreads_spin_t*)barrier);
#if USING_HYBRID == 1
    while(barrier->cont != XTHREADS_BARRIER_CONTINUE){}
#endif
    return XTHREADS_ENONE;
}
