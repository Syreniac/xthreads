/*
 * xthreads_rwlock2.c
 *
 *  Created on: 10 Aug 2016
 *      Author: Matthew
 */

#include <stdio.h>
#include <stdlib.h>
#include "xthreads.h"
#include "xthreads_rwlock2.h"
#include "xthreads_self.h"
#include "xthreads_spin.h"
#include "xthreads_errors.h"
#include "xthreads_util.h"

#define XTHREADS_NOCHANNEL 0x0


#define rwlockSignalRead(channel,targetChannel,position) __asm__ __volatile__(\
                                                  "in %0, res[%2]\n"\
                                                  "int %1, res[%2]\n"\
                                                  "chkct res[%2], 1\n"\
                                                  : "=r" (targetChannel), "=r" (position) : "r" (channel) :)

#define rwlockSignalTransmit(channel,targetChannel,position) __asm__ __volatile__(\
                                                      "out res[%0], %1\n"\
                                                      "outt res[%0], %2\n"\
                                                      "outct res[%0], 1\n"\
                                                      : : "r" (channel), "r" (targetChannel), "r" (position) :)

int xthreads_rwlock2_init(xthreads_rwlock2_t *rwlock, xthreads_rwlock2attr_t *attr){
    rwlock->spinlock = XTHREADS_SPINBLOCK;
    rwlock->rdChannel = XTHREADS_NOCHANNEL;
    rwlock->rdWaitCount = 0;
    rwlock->wrChannel = XTHREADS_NOCHANNEL;
    rwlock->wrWaitCount = 0;
    xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
    return XTHREADS_ENONE;
}

int xthreads_rwlock2_rdlock(volatile xthreads_rwlock2_t *rwlock){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    // Lock the mutex header quickly
    xthreads_spin_lock_inner((xthreads_spin_t*)rwlock,currentThreadId);
    rwlock->rdWaitCount++;
    register char position = rwlock->rdWaitCount; //Our position in the queue
    register xthreads_channel_t targetChannel = rwlock->rdChannel;
    rwlock->rdChannel = ourChannel;
    if(rwlock->wrWaitCount == 0 && position == 1){
        // The rwlock is unlocked, grab it
        xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
        return XTHREADS_ENONE;
    }
    else if(rwlock->wrWaitCount != 0 && position == 1){
        rwlock->rdWaitCount++;
        position = rwlock->rdWaitCount;
    }
    else{
        if(position != 1){
            __asm__ __volatile__(
                    "setd res[%0], %1"
                    : : "r" (ourChannel), "r" (targetChannel) :
            );
        }
    }
    register char newPosition;
    // Otherstuff
    xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
    // Wait for a signal
    while(1){
        rwlockSignalRead(ourChannel,targetChannel,newPosition);
        position--;
        if(position == 1){
            //printf("rgo\n");
            // We've become the thread holding the rwlock
            return XTHREADS_ENONE;
        }
        else if(position == newPosition){
            // The signal is meant for us
            //printf("rping\n");
            __asm__ __volatile__(
                    "setd res[%0], %1\n"
                    : : "r" (ourChannel), "r" (targetChannel) :
            );
        }
        else{
            //printf("recho %d %d\n",targetChannel,newPosition);
            // The signal means nothing for us
            rwlockSignalTransmit(ourChannel,targetChannel,newPosition);
        }
    }
    return XTHREADS_ENONE;
}

int xthreads_rwlock2_wrlock(volatile xthreads_rwlock2_t *rwlock){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    // Lock the rwlock header quickly
    xthreads_spin_lock_inner((xthreads_spin_t*)rwlock,currentThreadId);
    rwlock->wrWaitCount++;
    register char position = rwlock->wrWaitCount; //Our position in the queue
    register xthreads_channel_t targetChannel = rwlock->wrChannel;
    rwlock->wrChannel = ourChannel;
    if(rwlock->rdWaitCount == 0 && position == 1){
        // The rwlock is unlocked, grab it
        xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
        return XTHREADS_ENONE;
    }
    else if(rwlock->rdWaitCount != 0 && position == 1){
        rwlock->wrWaitCount++;
        position = rwlock->wrWaitCount;
    }
    else{
        if(position != 1){
            __asm__ __volatile__(
                    "setd res[%0], %1"
                    : : "r" (ourChannel), "r" (targetChannel) :
            );
        }
    }
    register char newPosition;
    // Otherstuff
    xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
    // Wait for a signal
    while(1){
        rwlockSignalRead(ourChannel,targetChannel,newPosition);
        position--;
        if(position == 1){
            //printf("wgo\n");
            // We've become the thread holding the rwlock
            return XTHREADS_ENONE;
        }
        else if(position == newPosition){
            // The signal is meant for us
            //printf("wping\n");
            __asm__ __volatile__(
                    "setd res[%0], %1\n"
                    : : "r" (ourChannel), "r" (targetChannel) :
            );
        }
        else{
            //printf("wecho %d %d\n",targetChannel,newPosition);
            // The signal means nothing for us
            rwlockSignalTransmit(ourChannel,targetChannel,newPosition);
        }
    }
    return XTHREADS_ENONE;
}

int xthreads_rwlock2_unlock(xthreads_rwlock2_t *rwlock){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    int a = 0;
    char b = 0;
    xthreads_spin_lock_inner((xthreads_spin_t*)rwlock,currentThreadId);
    if(rwlock->wrWaitCount > 1){
        rwlock->wrWaitCount--;
        __asm__ __volatile__(
                "setd res[%0], %1\n"
                : : "r" (ourChannel), "r" (rwlock->wrChannel) :
        );
        rwlockSignalTransmit(ourChannel,a,b);
    }
    else if(rwlock->rdWaitCount > 1){
        rwlock->rdWaitCount--;
        __asm__ __volatile__(
                "setd res[%0], %1\n"
                : : "r" (ourChannel), "r" (rwlock->rdChannel) :
        );
        rwlockSignalTransmit(ourChannel,a,b);
    }
    xthreads_spin_unlock_inner((xthreads_spin_t*)rwlock);
    //printf("unlocked\n");
    return XTHREADS_ENONE;
}
