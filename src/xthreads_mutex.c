/*
 * xthreads_mutex.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_channelwait.h"
#include "xthreads_threadqueue.h"
#include "xthreads_self.h"
#include "xthreads_spin.h"
#include "xthreads_util.h"
#include "xthreads_mutex.h"
#include <stdio.h>
#include <stdlib.h>

/* This mutex implementation is a fully channel based one. Threads waiting to lock the mutex are put into a queue
 * and when the mutex is released, a signal is sent via channel to the next thread in the queue causing it to
 * unpause and continue after locking the mutex.
 *
 * TODO - implement a hybrid spinlock/channel mutex for minor performance gains (?)
 */

//static int xthreads_mutex_timedlock_inner(xthreads_channel_t chanend, unsigned int time);
void __attribute__ ((noinline)) xthreads_mutex_timedlock_inner2(int currentThreadId, xthreads_mutex_t *mutex, int index, int success);

int xthreads_mutex_init(xthreads_mutex_t *mutex, xthreads_mutexattr_t *attr){
    register xthreads_t x = XTHREADS_NOTHREAD;
    mutex->templocked = XTHREADS_SPINBLOCK;
    xthreads_threadqueue_init(&mutex->queue);
    mutex->activeThread = XTHREADS_NOTHREAD;
    // Functionally the same as calling xthreads_spin_unlock_inner, but more likely to trick the compiler into not optimising it
    __asm__ __volatile__("stw %0, %1[0]" : : "r" (x), "r" (mutex) : );
    return XTHREADS_ENONE;
}

int xthreads_mutex_lock(xthreads_mutex_t *mutex){
    xthreads_t currentThreadId = xthreads_self();
    int index;

    /* Spinlock whilst we add ourselves to the mutex queue */
    /* At worst we should be waiting O(15) (?), so this isn't an absolutely terrible spinlock */
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex, currentThreadId);

    if(mutex->activeThread == XTHREADS_NOTHREAD){
        // Case 1 - the mutex is released
        mutex->activeThread = currentThreadId;
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        return XTHREADS_ENONE;
    }
    else{
        // Case 2 - the mutex is locked
        index = xthreads_threadqueue_insert(&mutex->queue,currentThreadId);
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        xthreads_channelwait_nocancel(currentThreadId);
        xthreads_threadqueue_nullify(&mutex->queue,index);
        mutex->activeThread = currentThreadId;
        return XTHREADS_ENONE;
    }
}

int xthreads_mutex_trylock(xthreads_mutex_t *mutex){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
    if(mutex->activeThread == XTHREADS_NOTHREAD){
        mutex->activeThread = currentThreadId;
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        return XTHREADS_ENONE;
    }
    xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
    return XTHREADS_EBUSY;
}

int xthreads_mutex_unlock(xthreads_mutex_t *mutex){
    int currentThreadId = xthreads_self();

    if(mutex->activeThread != currentThreadId){
        // A thread not holding the mutex is trying to release it
        return XTHREADS_EPERM;
    }

    // Send a close signal on the channel activating the next thread in the process
    xthreads_t next_thread = xthreads_threadqueue_read(&mutex->queue);

    int i = 0;
    while(next_thread == XTHREADS_TIMEREMOVEDTHREAD){
        i++;
        next_thread = xthreads_threadqueue_read(&mutex->queue);
    }

    if(next_thread == XTHREADS_NOTHREAD){
        mutex->activeThread = XTHREADS_NOTHREAD;
        return XTHREADS_ENONE;
    }

    // Get the channel ends
    xthreads_channel_t chanend = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    xthreads_channel_t chandest = xthreads_globals.stacks[next_thread].data.threadChannel;

    xthreads_printBytes(chandest);
    // Send a close token to wake the next thread up
    __asm__ __volatile__(
            "setd res[%0], %1\n"
            "outct res[%0], 1\n"
            : : "r" (chanend), "r" (chandest): );
    return XTHREADS_ENONE;
}

int xthreads_mutex_timedlock(xthreads_mutex_t *mutex, unsigned int time){

    xthreads_t currentThreadId = xthreads_self();
    register int success asm("r4") = 0;
    int index;

    /* Spinlock whilst we add ourselves to the mutex queue */
    /* At worst we should be waiting O(15) (?), so this isn't an absolutely terrible spinlock */
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex, currentThreadId);

    if(mutex->activeThread == XTHREADS_NOTHREAD){
        // Case 1 - the mutex is released
        mutex->activeThread = currentThreadId;
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        return XTHREADS_ENONE;
    }
    else{
        // Case 2 - the mutex is locked
        index = xthreads_threadqueue_insert(&mutex->queue,currentThreadId);
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);

        success = xthreads_channelwait_nocancel_timed(currentThreadId,time);

        if(success == XTHREADS_ENONE){
            xthreads_threadqueue_nullify(&mutex->queue,index);
            mutex->activeThread = currentThreadId;
        }
        else{
            xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
            xthreads_threadqueue_skip(&mutex->queue,index);
            xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        }
        return success;
    }
}

/*int xthreads_mutex_timedlock_inner(xthreads_channel_t chanend, unsigned int time){


    register volatile xthreads_resource_t timer asm("r5");
    volatile int value = 0;
    __asm__ __volatile__(
          "getr %0, 1\n" // Get a timer"
          : "=r" (timer) : : );

    if(timer == 0){
        printf("xthreads exception: unable to get timer for timed mutex lock\n");
        exit(1);
    }

    __asm__ __volatile__(
            "setd res[%0], %1\n" // Set the timer's time
            "setc res[%0], 0x9\n"
            : : "r" (timer), "r" (time) :
    );

    __asm__ __volatile__(
          "clre\n" // Clear all enabled events (there shouldn't be many)
          "ldap r11, 0x6\n"
          "setv res[%1], r11\n" //Mark where the channel end sends us
          "eeu res[%1]\n" //Mark the channel end as giving us an interrupt
          "ldap r11, 0x6\n"
          "setv res[%2], r11\n"
          "eeu res[%2]\n"
          "waiteu\n"
          "chkct res[%1], 1\n"
          "ldc %0, %3\n"
          "bu 0x3\n"
          "in r10, res[%1]\n"
          "ldc %0, %4\n"
          "edu res[%1]\n"
          "freer res[%2]\n"
    : "=r" (value) : "r" (chanend), "r" (timer), "i" (XTHREADS_ENONE), "i" (XTHREADS_ETIMEDOUT): "r11","r10");
    return value;
}*/


