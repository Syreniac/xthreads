/*
 * xthreads_cond.c
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_errors.h"
#include "xthreads_channelwait.h"
#include "xthreads_cancel.h"
#include "xthreads_cond.h"

int xthreads_cond_init(xthreads_cond_t *cond){
    xthreads_t x = XTHREADS_NOTHREAD;
    cond->templocked = XTHREADS_SPINBLOCK;
    xthreads_threadqueue_init(&cond->queue);
    __asm__ __volatile__("stw %0, %1[0]" : : "r" (x), "r" (cond) :);
    return XTHREADS_ENONE;
}

int xthreads_cond_destroy(xthreads_cond_t *cond){return XTHREADS_ENONE;}

// Wake up one thread
int xthreads_cond_signal(xthreads_cond_t *cond){

    int currentThreadId = xthreads_self();

    // Send a close signal on the channel activating the next thread in the process
    xthreads_t next_thread = xthreads_threadqueue_read(&cond->queue);

    int i = 0;
    while(next_thread == XTHREADS_TIMEREMOVEDTHREAD){
        i++;
        next_thread = xthreads_threadqueue_read(&cond->queue);
    }

    if(next_thread == XTHREADS_NOTHREAD){
        return XTHREADS_ENONE;
    }

    // Get the channel ends
    xthreads_channel_t chanend = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    xthreads_channel_t chandest = xthreads_globals.stacks[next_thread].data.threadChannel;

    //printBytes(chandest);
    // Send a close token to wake the next thread up
    __asm__ __volatile__(
            "setd res[%0], %1\n"
            "outct res[%0], 1\n"
            : : "r" (chanend), "r" (chandest): );
    return XTHREADS_ENONE;
}

int xthreads_cond_broadcast(xthreads_cond_t *cond){

    xthreads_t currentThreadId = xthreads_self();
    // Send a close signal on the channel activating the next thread in the process
    xthreads_t next_thread = xthreads_threadqueue_read(&cond->queue);
    xthreads_channel_t chanend = xthreads_globals.stacks[currentThreadId].data.threadChannel;

    // Loop through the condition variable queue and wake up each thread
    while(1){
        while(next_thread == XTHREADS_TIMEREMOVEDTHREAD){
            next_thread = xthreads_threadqueue_read(&cond->queue);
        }
        if(next_thread == XTHREADS_NOTHREAD){
            break;
        }
        else{
            // Get the channel ends
            xthreads_channel_t chandest = xthreads_globals.stacks[next_thread].data.threadChannel;

            //printBytes(chandest);
            // Send a close token to wake the next thread up
            __asm__ __volatile__(
                    "setd res[%0], %1\n"
                    "outct res[%0], 1\n"
                    : : "r" (chanend), "r" (chandest): );
        }
    }
    return XTHREADS_ENONE;
}

int xthreads_cond_wait(xthreads_cond_t *cond, xthreads_mutex_t *mutex){


    xthreads_t currentThreadId = xthreads_self();
    xthreads_spin_lock_inner((xthreads_spin_t*)cond, currentThreadId);

    xthreads_mutex_unlock(mutex);

    int index = xthreads_threadqueue_insert(&cond->queue,currentThreadId);
    xthreads_spin_unlock_inner((xthreads_spin_t*)cond);
    if(xthreads_cancellationPointCheck(currentThreadId)){
        xthreads_channelwait(currentThreadId);
    }
    else{
        xthreads_channelwait_nocancel(currentThreadId);
    }
    xthreads_threadqueue_nullify(&cond->queue,index);
    xthreads_mutex_lock(mutex);
    return XTHREADS_ENONE;
}


