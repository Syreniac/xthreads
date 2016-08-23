/*
 * xthreads_cond2.c
 *
 *  Created on: 14 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_cond2.h"
#include "xthreads_spin.h"
#include "xthreads_errors.h"
#include "xthreads_util.h"
#include <stdlib.h>

#define XTHREADS_NOCHANNEL 0x0
#define condRelease 0xffffffff

#define condSignalRead(channel,targetChannel,position) __asm__ __volatile__(\
                                                  "in %0, res[%2]\n"\
                                                  "int %1, res[%2]\n"\
                                                  "chkct res[%2], 1\n"\
                                                  : "=r" (targetChannel), "=r" (position) : "r" (channel) :)

#define condSignalTransmit(channel,targetChannel,position) __asm__ __volatile__(\
                                                      "out res[%0], %1\n"\
                                                      "outt res[%0], %2\n"\
                                                      "outct res[%0], 1\n"\
                                                      : : "r" (channel), "r" (targetChannel), "r" (position) :)
static int xthreads_cond2_cascade(xthreads_cond2_t *cond, xthreads_channel_t channel, char position);

int xthreads_cond2_broadcast(xthreads_cond2_t *cond){
    return xthreads_cond2_cascade(cond,condRelease,0);
}
int xthreads_cond2_signal(xthreads_cond2_t *cond){
    return xthreads_cond2_cascade(cond,0,0);
}

int xthreads_cond2_init(xthreads_cond2_t *cond){
    cond->spinlock = XTHREADS_SPINBLOCK;
    cond->channel = XTHREADS_NOCHANNEL;
    cond->waitCount = 0;
    xthreads_spin_unlock_inner((xthreads_spin_t*)cond);
    return XTHREADS_ENONE;
}
int xthreads_cond2_destroy(xthreads_cond2_t *cond){
    return XTHREADS_ENONE;
}
static int xthreads_cond2_cascade(xthreads_cond2_t *cond, xthreads_channel_t channel, char position){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    xthreads_spin_lock_inner((xthreads_spin_t*)cond,currentThreadId);
    cond->waitCount--;
    if(cond->waitCount != 0){
        __asm__ __volatile__(
                "setd res[%0], %1\n"
                : : "r" (ourChannel), "r" (cond->channel) :
        );
        condSignalTransmit(ourChannel,channel,position);
    }
    xthreads_spin_unlock_inner((xthreads_spin_t*)cond);
    //printf("unlocked\n");
    return XTHREADS_ENONE;
}

int xthreads_cond2_timedwait(xthreads_cond2_t *cond, xthreads_mutex2_t *mutex, xthreads_time_t *time){
    xthreads_t currentThreadId = xthreads_self();

    xthreads_resource_t timer;
    register const xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    // Lock the cond header quickly
    xthreads_spin_lock_inner((xthreads_spin_t*)cond,currentThreadId);
    cond->waitCount++;
    register char position = cond->waitCount; //Our position in the queue
    register xthreads_channel_t targetChannel = cond->channel;
    cond->channel = ourChannel;
    //printf("cond lock requeusted by thread %d, queue position %d\n", currentThreadId, position);
    if(position == 1){
        // The cond is unlocked, grab it
        xthreads_spin_unlock_inner((xthreads_spin_t*)cond);
        return XTHREADS_ENONE;
    }
    else{
        register char newPosition;
        // Otherstuff
        __asm__ __volatile__(
                "setd res[%0], %1"
                : : "r" (ourChannel), "r" (targetChannel) :
        );
        xthreads_spin_unlock_inner((xthreads_spin_t*)cond);
        __asm__ __volatile__("getr %0, 1" : "=r" (timer) : :);
        if(timer == 0){
            exit(1);
        }
        prepTimer(timer,time);
        // XCC doesn't support asm gotos, so I have to manually recalculate the offsets here every time...
        __asm__ __volatile__(
                "ldap r11, 0x6\n"
                "setv res[%0], r11\n"
                "ldap r11, 0x26\n"
                "setv res[%1], r11\n"
                "eeu res[%0]\n"
                "eeu res[%1]\n"
                "waiteu\n"
                : : "r" (timer), "r" (ourChannel) : "r11"
        );
        // ----------------------------------------------------------------------------------------
        // Despite appearances, these blocks of code will not be run in sequence. Depending on what event
        // Triggers first, either the code above will run, or the code below before essentially looping back up to
        // event enabling block.
        // There's not really a nice way of handling this in C, and XC throws a tantrum if you try to merge
        // xthreads straight into it because of function pointers and unsafe pointers, so I'm handling it as best I can
        // (Splitting it into subfunctions doesn't work because XCC will not cooperate with the register manipulation
        // that is needed.
        // ----------------------------------------------------------------------------------------
        // Wait for signal on timer
        __asm__ __volatile__("nop");
        // If we try to lock the condition var and we can't, we should check that there's not pending signals
        // to pass on
        if(xthreads_spin_trylock_inner((xthreads_spin_t*)cond,currentThreadId) == XTHREADS_ENONE){
            cond->waitCount--;
            if(cond->channel != ourChannel){
                __asm__ __volatile__("setd res[%0], %1" : : "r" (ourChannel), "r" (cond->channel) :);
                condSignalTransmit(ourChannel,targetChannel,position);
            }
            else{
                cond->channel = targetChannel;
            }
            xthreads_spin_unlock_inner((xthreads_spin_t*)cond);
        }
        else{
            // If we can't lock the spinlock, something else somewhere is trying to send a signal
            // TODO -
            __asm__ __volatile__(
                    "waiteu\n"
                    : : : );
        }
        // Unfortunately GCC in its infinite wisdom will cut out all the code after this if you use a non-obscure
        // technique for getting to the end of the function
        __asm__ __volatile__("bu 0x19");

        // ----------------------------------------------------------------------------------------
        // Wait for a signal on channel
        while(1){
            condSignalRead(ourChannel,targetChannel,newPosition);
            position--;
            if(targetChannel == condRelease){
                __asm__ __volatile__("edu res[%0]" : : "r" (ourChannel) : );
                __asm__ __volatile__("freer res[%0]" : : "r" (timer) :);
                return XTHREADS_ENONE;
            }
            if(position == newPosition){
                // The signal is meant for us
                __asm__ __volatile__(
                        "setd res[%0], %1\n"
                        : : "r" (ourChannel), "r" (targetChannel) :
                );
            }
            else if(position == 1){
                // We've become the thread holding the cond var, disable events and return
                __asm__ __volatile__("edu res[%0]" : : "r" (ourChannel) : );
                __asm__ __volatile__("freer res[%0]" : : "r" (timer) :);
                return XTHREADS_ENONE;
            }
            else{
                // The signal means nothing for us
                //printf("echo %d %d\n",targetChannel,newPosition);
                //printf("transmitting that %d should target %d\n",newPosition,targetChannel);
                condSignalTransmit(ourChannel,targetChannel,newPosition);
            }
            __asm__ __volatile__("waiteu\n");
        }
    }
    // ---------------------------------------------------------------------------------------
    // Disable the events
    __asm__ __volatile__("edu res[%0]" : : "r" (ourChannel) : );
    __asm__ __volatile__("freer res[%0]" : : "r" (timer) :);
    return XTHREADS_ETIMEDOUT;
}
int xthreads_cond2_wait(xthreads_cond2_t *cond, xthreads_mutex2_t *mutex){
    return XTHREADS_ENONE;
}
