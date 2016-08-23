/*
 * xthreads_rwlock.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_channelwait.h"
#include "xthreads_rwlock.h"
#include "xthreads_threadqueue.h"
#include "xthreads_spin.h"
#include "xthreads_self.h"

#define XTHREADS_RWLOCK_CANCELLATION_POINT 1

static void xthreads_rwlock_lock(xthreads_rwlock_t *rwlock, xthreads_threadqueue_t *queue);


void xthreads_rwlock_init(xthreads_rwlock_t *rwlock, xthreads_rwlockattr_t *attr){
    int x = XTHREADS_NOTHREAD;
    rwlock->templocked = XTHREADS_SPINBLOCK;
    xthreads_threadqueue_init(&rwlock->write_queue);
    xthreads_threadqueue_init(&rwlock->read_queue);
    rwlock->activeThread = XTHREADS_NOTHREAD;
    __asm__ __volatile__("stw %0, %1[0]" : : "r" (x), "r" (rwlock) : );

}

void xthreads_rwlock_rdlock(xthreads_rwlock_t *rwlock){
    xthreads_rwlock_lock(rwlock,&rwlock->read_queue);
}

void xthreads_rwlock_wrlock(xthreads_rwlock_t *rwlock){
    xthreads_rwlock_lock(rwlock,&rwlock->write_queue);
}

static void xthreads_rwlock_lock(xthreads_rwlock_t *rwlock, xthreads_threadqueue_t *queue){
    xthreads_t currentThreadId = xthreads_self();
    int index;
    xthreads_spin_lock_inner((xthreads_spin_t*)rwlock,currentThreadId);

    if(rwlock->activeThread == XTHREADS_NOTHREAD){
        // Case 1 - rwlock is unlocked
        rwlock->activeThread = currentThreadId;
        xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
        return;
    }
    else{
        // Case 2 - rwlock is locked
        index = xthreads_threadqueue_insert(queue,currentThreadId);
        xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
#if XTHREADS_RWLOCK_CANCELLATION_POINT == 1
        xthreads_channelwait(currentThreadId);
#else
        // We don't want to reset the event watching on the cancel channel if we can avoid it
        // So we'll check here whether we should
        if(xthreads_cancellationPointCheck(currentThreadId)){
            xthreads_channelwait(currentThreadId);
        }
        else{
            xthread_channelwait_nocancel(currentThreadId);
        }
#endif
        // Whilst these two changes are not protected by the spinlock, they should not be accidentally done
        // The queue change should be protected by the queue index looping
        // The active thread change should be protected as the activeThread never hits zero unless the queue is empty
        // In which case this code will never be called
        // In theory.
        queue->queue[index] = XTHREADS_NOTHREAD;
        rwlock->activeThread = currentThreadId;
    }
}

void xthreads_rwlock_unlock(xthreads_rwlock_t *rwlock){
        int currentThreadId = xthreads_self();

        if(rwlock->activeThread != currentThreadId){
            // A thread not holding the mutex is trying to release it
            // UNDEFINED BEHAVIOR?
            return;
        }
        // Get our channel to send with
        xthreads_channel_t chanend = xthreads_globals.stacks[currentThreadId].data.threadChannel;

        // Check what the next thread is
        xthreads_t next_thread = xthreads_threadqueue_read(&rwlock->write_queue);
        // Deciding what thread to wake up next
        if(next_thread == -1){
            next_thread = xthreads_threadqueue_read(&rwlock->read_queue);
            if(next_thread == -1){
                rwlock->activeThread = XTHREADS_NOTHREAD;
                // No thread waiting on lock, restore to empty state
                return;
            }
        }
        // Get the destination channel
        xthreads_channel_t chandest = xthreads_globals.stacks[next_thread].data.threadChannel;
        __asm__ __volatile__(
                "setd res[%0], %1\n"
                "outct res[%0], 1\n"
                : : "r" (chanend), "r" (chandest) : );
        return;
}


