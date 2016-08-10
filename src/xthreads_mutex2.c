/*
 * xthreads_mutex2.c
 *
 *  Created on: 9 Aug 2016
 *      Author: Matthew
 */

#include <stdio.h>
#include <stdlib.h>
#include "xthreads.h"
#include "xthreads_mutex2.h"
#include "xthreads_self.h"
#include "xthreads_spin.h"
#include "xthreads_errors.h"
#include "xthreads_util.h"

#define XTHREADS_NOCHANNEL 0x0

#define mutexSignalRead(channel,targetChannel,position) __asm__ __volatile__(\
                                                  "in %0, res[%2]\n"\
                                                  "int %1, res[%2]\n"\
                                                  "chkct res[%2], 1\n"\
                                                  : "=r" (targetChannel), "=r" (position) : "r" (channel) :)

#define mutexSignalTransmit(channel,targetChannel,position) __asm__ __volatile__(\
                                                      "out res[%0], %1\n"\
                                                      "outt res[%0], %2\n"\
                                                      "outct res[%0], 1\n"\
                                                      : : "r" (channel), "r" (targetChannel), "r" (position) :)

int xthreads_mutex2_init(xthreads_mutex2_t *mutex, xthreads_mutex2attr_t *attr){
    mutex->spinlock = XTHREADS_SPINBLOCK;
    mutex->channel = XTHREADS_NOCHANNEL;
    mutex->waitCount = 0;
    xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
    return XTHREADS_ENONE;
}

int xthreads_mutex2_lock(volatile xthreads_mutex2_t *mutex){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    // Lock the mutex header quickly
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
    mutex->waitCount++;
    register char position = mutex->waitCount; //Our position in the queue
    register xthreads_channel_t targetChannel = mutex->channel;
    mutex->channel = ourChannel;
    //printf("mutex lock requeusted by thread %d, queue position %d\n", currentThreadId, position);
    if(position == 1){
        // The mutex is unlocked, grab it
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        return XTHREADS_ENONE;
    }
    else{
        register char newPosition;
        // Otherstuff
        __asm__ __volatile__(
                "setd res[%0], %1"
                : : "r" (ourChannel), "r" (targetChannel) :
        );
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        // Wait for a signal
        while(1){
            mutexSignalRead(ourChannel,targetChannel,newPosition);
            position--;
            if(position == newPosition){
                // The signal is meant for us
                //printf("ping\n");
                __asm__ __volatile__(
                        "setd res[%0], %1\n"
                        : : "r" (ourChannel), "r" (targetChannel) :
                );
            }
            else if(position == 1){
                //printf("go\n");
                // We've become the thread holding the mutex
                return XTHREADS_ENONE;
            }
            else{
                //printf("echo %d %d\n",targetChannel,newPosition);
                // The signal means nothing for us
                mutexSignalTransmit(ourChannel,targetChannel,newPosition);
            }
        }
    }
    return XTHREADS_ENONE;
}

int xthreads_mutex2_unlock(xthreads_mutex2_t *mutex){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    int a = 0;
    char b = 0;
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
    mutex->waitCount--;
    if(mutex->waitCount != 0){
        __asm__ __volatile__(
                "setd res[%0], %1\n"
                : : "r" (ourChannel), "r" (mutex->channel) :
        );
        mutexSignalTransmit(ourChannel,a,b);
    }
    xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
    //printf("unlocked\n");
    return XTHREADS_ENONE;
}

int xthreads_mutex2_timedlock(volatile xthreads_mutex2_t *mutex, xthreads_time_t time){
    xthreads_t currentThreadId = xthreads_self();
    register const xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    // Lock the mutex header quickly
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
    mutex->waitCount++;
    register char position = mutex->waitCount; //Our position in the queue
    register xthreads_channel_t targetChannel = mutex->channel;
    mutex->channel = ourChannel;
    //printf("mutex lock requeusted by thread %d, queue position %d\n", currentThreadId, position);
    if(position == 1){
        // The mutex is unlocked, grab it
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        return XTHREADS_ENONE;
    }
    else{
        register char newPosition;
        // Otherstuff
        __asm__ __volatile__(
                "setd res[%0], %1"
                : : "r" (ourChannel), "r" (targetChannel) :
        );
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        xthreads_resource_t timer;
        __asm__ __volatile__("getr %0, 1" : "=r" (timer) : :);
        if(timer == 0){
            exit(1);
        }
        prepTimer(timer,time);
        __asm__ __volatile__(
                "ldap r11, 0x7\n"
                "setv res[%0], r11\n"
                "ldap r11, 0x1f\n"
                "setv res[%1], r11\n"
                "eeu res[%0]\n"
                "eeu res[%1]\n"
                "waiteu\n"
                : : "r" (timer), "r" (ourChannel) : "r11"
        );
        // Wait for signal on timer
        __asm__ __volatile__("nop");
        xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
        mutex->waitCount--;
        if(mutex->channel != ourChannel){
            __asm__ __volatile__("setd res[%0], %1" : : "r" (ourChannel), "r" (mutex->channel) :);
            mutexSignalTransmit(ourChannel,targetChannel,position);
        }
        else{
            mutex->channel = targetChannel;
        }
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
        __asm__("bu 0x19");
        // Wait for a signal on channel
        while(1){
            __asm__ __volatile("nop");
            mutexSignalRead(ourChannel,targetChannel,newPosition);
            position--;
            if(position == newPosition){
                // The signal is meant for us
                __asm__ __volatile__(
                        "setd res[%0], %1\n"
                        : : "r" (ourChannel), "r" (targetChannel) :
                );
            }
            else if(position == 1){
                // We've become the thread holding the mutex
                return XTHREADS_ENONE;
            }
            else{
                // The signal means nothing for us
                //printf("echo %d %d\n",targetChannel,newPosition);
                //printf("transmitting that %d should target %d\n",newPosition,targetChannel);
                mutexSignalTransmit(ourChannel,targetChannel,newPosition);
            }
            __asm__ __volatile__("waiteu");
        }
    }
    return XTHREADS_ENONE;
}
