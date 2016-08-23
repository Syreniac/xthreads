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
#include "xthreads_cancel.h"
#include "xthreads_task.h"
#include "xthreads_access.h"

#define XTHREADS_NOCHANNEL 0x0
#define XTHREADS_MUTEX_HACKYNOTAPOINTER (void*)((long) 0x1)

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

#define xthreads_mutex2_prelock(self,mutex) if(mutex->type < RECURSIVE){            \
                                                if(mutex->type == MINIMUM){         \
                                                    return XTHREADS_EAGAIN;         \
                                                }                                   \
                                                mutex->type--;                      \
                                                return XTHREADS_ENONE;              \
                                            }                                       \
                                            else if(mutex->type == ERRORCHECK){  \
                                                if(mutex->holding == self){         \
                                                    return XTHREADS_EDEADLK;        \
                                                }                                   \
                                            }

int xthreads_mutex2_init(xthreads_mutex2_t *mutex, xthreads_mutex2attr_t *attr){
    mutex->spinlock = XTHREADS_SPINBLOCK;
    mutex->channel = XTHREADS_NOCHANNEL;
    mutex->waitCount = 0;
    if(attr == NULL){
        mutex->type = DEFAULT;
    }
    else{
        mutex->type = attr->type;
    }
    xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
    return XTHREADS_ENONE;
}

int xthreads_mutex2_lock(volatile xthreads_mutex2_t *mutex){
    const xthreads_t currentThreadId = xthreads_self();
    xthreads_mutex2_prelock(currentThreadId,mutex);
    const register xthreads_channel_t ourChannel = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    // Lock the mutex header quickly
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
    register char position asm("r10") = mutex->waitCount + 1;
    mutex->waitCount = position;
    register xthreads_channel_t targetChannel = mutex->channel;
    mutex->channel = ourChannel;
    xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
     //Our position in the queue
    //printf("mutex lock requeusted by thread %d, queue position %d\n", currentThreadId, position);
    if(position == 1){
        // The mutex is unlocked, grab it
        return XTHREADS_ENONE;
    }
    else{
        register char newPosition;
        // Otherstuff
        __asm__ __volatile__(
                "setd res[%0], %1"
                : : "r" (ourChannel), "r" (targetChannel) :
        );
        // Wait for a signal
        while(1){
            mutexSignalRead(ourChannel,targetChannel,newPosition);
            __asm__ __volatile__("sub %0, %1, 0x1" : "=r" (position) : "r" (position) :);
            if(position == newPosition){
                // The signal is meant for us
                //printf("ping\n");
                __asm__ __volatile__(
                        "setd res[%0], %1\n"
                        : : "r" (ourChannel), "r" (targetChannel) :
                );
                xthreads_spin_unlock_inner((xthreads_spin_t*) mutex);
            }
            else if(position == 1){
                //printf("go\n");
                // We've become the thread holding the mutex
                xthreads_spin_unlock_inner((xthreads_spin_t*) mutex);
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
    if(mutex->holding == currentThreadId){
        if(mutex->type < RECURSIVE){
            mutex->type++;
            return XTHREADS_ENONE;
        }
    }
    else if(mutex->type == ERRORCHECK || mutex->type < RECURSIVE){
        return XTHREADS_EPERM;
    }
    __asm__ __volatile__("clrsr 0x1\n");
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);
    mutex->waitCount--;
    if(mutex->waitCount > 0){
        __asm__ __volatile__(
                "setd res[%0], %1\n"
                : : "r" (ourChannel), "r" (mutex->channel) :
        );
        mutexSignalTransmit(ourChannel,a,b);
        // The spin lock is unlocked by the thread at the end of the chain
    }
    else{
        xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);
    }
    if(xthreads_access_cancelType(currentThreadId) == ASYNC && xthreads_access_cancelState(currentThreadId) == ENABLED){
        __asm__ __volatile__("setsr 0x1\n");
    }
    //printf("unlocked\n");
    return XTHREADS_ENONE;
}

// This is an incredibly long function for some important reasons.
// Firstly, much of the work is done in registers that are tricky to pass accurately between functions.
// Secondly, the link register is not updated properly if we use event vectors into functions
// Thirdly, the event hierarchy gets a bit weird. On their own, certain events would block the mutex preventing other
// events from happening, causing a deadlock. To resolve this, I've set it up such that
int xthreads_mutex2_timedlock(volatile xthreads_mutex2_t *mutex,const xthreads_time_t time){
    register const xthreads_t currentThreadId = xthreads_self();
    xthreads_mutex2_prelock(currentThreadId,mutex);
    register const xthreads_channel_t ourChannel = xthreads_access_threadChannel(currentThreadId);
    register xthreads_canceltype_t cancelType;
    register char position;
    register xthreads_channel_t targetChannel;

    // First we need to set the cancel type.
    // If we set the cancelState to BLANK when disabled, we only need to keep cancelType
    // and perform checks against that rather than keeping both
    if(xthreads_access_cancelState(currentThreadId) == DISABLED){
        cancelType = BLANK;
    }
    else{
        cancelType = xthreads_access_cancelType(currentThreadId);//xthreads_globals.stacks[currentThreadId].data.cancelType;
    }

    // Disable events and lock the mutex head
    // It's critically important that even if we want to have async cancelling
    // that we don't cancel whilst inside the spinlocking on the mutex otherwise
    // it will be in a permanently damaged and ususable state.
    __asm__ __volatile__("clrsr 0x1\n");
    xthreads_spin_lock_inner((xthreads_spin_t*)mutex,currentThreadId);

    position = mutex->waitCount + 1;
    mutex->waitCount = position;
    targetChannel = mutex->channel;
    mutex->channel = ourChannel;

    xthreads_spin_unlock_inner((xthreads_spin_t*)mutex);

    if(position == 1){
        // The mutex is unlocked, grab it and potentially re-enable events
        if(cancelType == ASYNC){
            __asm__ __volatile__("setsr 0x1\n");
        }
        return XTHREADS_ENONE;
    }

    /* Else we need to prep the various events that can wake us from waiting on the mutex*/
    register const xthreads_channel_t cancelChannel = xthreads_self_cancelChannel(currentThreadId);
    register xthreads_resource_t timer asm("r10") = 0;
    register char newPosition;

    if(cancelType == DEFERRED){
        // Deferred cancelling shouldn't be checked in mutexes according to the standard
        // Therefore we just toggle the cancelChannel off here
        __asm__ __volatile__("edu res[%0]" : : "r" (cancelChannel):);
    }
    else if(cancelType == ASYNC){
            // Asynchronous cancelling needs to move to a different vector for the course of this function otherwise
            // The mutex will get made unusable by a cancelled thread
        __asm__ __volatile__("ldap r11, 0x74\n"
                             "setv res[%0], r11\n"
                             : : "r" (cancelChannel) : "r11");
    }
    /* If we're dealing with a non-stupid time amount, we'll need to grab a timer and configure it for events.*/
    if(time > 0){
        __asm__ __volatile__("getr %0, 1" : "=r" (timer) : :);
        if(timer == 0){
            // If the timer gets a 0 from the getr request, something bad has happened.
            exit(1);
        }
        prepTimer(timer,time);
        // set vector and enable timer events
        __asm__ __volatile__(
                "ldap r11, 0xA\n"
                "setv res[%0], r11\n"
                "eeu res[%0]\n"
                : : "r" (timer) : "r11"
        );
    }
    /* Configure the channel for events:
     *  1. Set the destination to the channel in front of us in the queue
     *  2. Set the event vector to the channel handling code
     *  (NB: modifications to this function require recalucating the vector by hand...)
     *  3. Enable events on the channel
     *  4. Waiteu
     * */
    __asm__ __volatile__(
            "setd res[%0], %1\n"
            "ldap r11, 0x30\n"
            "setv res[%0], r11\n"
            "eeu res[%0]\n"
            "waiteu\n"
            : : "r" (ourChannel), "r" (targetChannel) : "r11"
    );
    // ----------------------------------------------------------------------------------------
    // Despite appearances, these blocks of code will not be run in sequence. Depending on what event
    // Triggers first, either the code above will run, or the code below before essentially looping back up to
    // event enabling block.
    // There's not really a nice way of handling this in C, and XC throws a tantrum if you try to merge
    // xthreads straight into it because of function pointers and unsafe pointers, so I'm handling it as best I can
    // (Splitting it into subfunctions doesn't work because XCC will not cooperate with the register manipulation
    // that is needed, and because each event can trigger the others, it's a lot trickier to deal with them separately)
    // If it makes it easier, each section can be read independently as a pseudofunction.
    // ---------------------------------------------------------------------------------------------------------------------
    __asm__ __volatile__("edu res[%0]" : : "r" (timer) : );
    timerHandling:
    if(xthreads_spin_trylock_inner((xthreads_spin_t*) mutex, currentThreadId) == XTHREADS_ENONE){
        mutex->waitCount = mutex->waitCount - 1;
        if(mutex->channel != ourChannel && mutex->waitCount > 1){
            __asm__ __volatile__("setd res[%0], %1" : : "r" (ourChannel), "r" (mutex->channel) :);
            mutexSignalTransmit(ourChannel,targetChannel,position);
        }
        else{
            mutex->channel = targetChannel;
            xthreads_spin_unlock_inner((xthreads_spin_t*) mutex);
        }
        // Can't use a formal goto because the compiler cuts everything after it out :(
        // Return also culls stuff out, so weird hacky jump it is
        __asm__ __volatile__("bu 0x5E");
    }
    else{
        /* The pseudo code for what is happening here on a hardware level is:
         *  if(channel.eventTriggered){ goto channelHandling;}
         *  else if(cancel.eventTriggered){ goto cancelHandling;}
         *
         *  It's simulataneous though, so it's not a true if/else
         *
         *  However, the hardware actually handles all this logic behind the scenes. All I do is
         *  tell the thread to pay attention to what the hardware wants us to do.
         */
        __asm__ __volatile__("setsr 0x1\n");
        __asm__ __volatile__("clrsr 0x1\n");
        goto timerHandling;
    }
    // ---------------------------------------------------------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------------------------------
    //channelHandling:
    mutexSignalRead(ourChannel,targetChannel,newPosition);
    __asm__ __volatile__("sub %0, %1, 0x1" : "=r" (position) : "r" (position) :);
    if(position == 1){
        // We've locked the mutex
        xthreads_spin_unlock_inner((xthreads_spin_t*) mutex);
        __asm__ __volatile__("clre");
        if(timer != 0){
            __asm__ __volatile__("freer res[%0]" : : "r" (timer) : );
        }
        mutex->holding = currentThreadId;
        if(cancelType != BLANK){
            xthreads_cancel_revector(cancelChannel);
            __asm__ __volatile__("eeu res[%0]" : : "r" (cancelChannel) :);
            if(cancelType == ASYNC){
                __asm__ __volatile__("setsr 0x1\n");
            }
        }
        return XTHREADS_ENONE;
    }
    else if(position == newPosition){
        // Signal is for us
        __asm__ __volatile__("setd res[%0], %1" : : "r" (ourChannel), "r" (targetChannel) : );
        xthreads_spin_unlock_inner((xthreads_spin_t*) mutex);
    }
    else{
        mutexSignalTransmit(ourChannel,targetChannel,newPosition);
    }
    if(timer != 0){
        // If the timer has been freed, we can't enable it
        __asm__ __volatile__("eeu res[%0]" : : "r" (timer) : );
    }
    // The only time both the timer is freed and channel flush needs to happen is if we've had an asynchronous cancellation
    else{
        // Re-enabling capturing cancel events
        __asm__ __volatile__("eeu res[%0]" : : "r" (cancelChannel) :);
    }
    // The thread waits here until one of the events happens.
    __asm__ __volatile__("waiteu");
    // ---------------------------------------------------------------------------------------------------------------------
    // cancelHandling;
    // Because we can't simply asynchronously leave the mutex queue without causing seriously weird behaviour, we
    // need to override the normal behaviour with some wrapping to fix this

    // If we're cancelling, kill the timer because its more important that we self-terminate than return on time
    __asm__ __volatile__("chkct res[%0], 1\n" : : "r" (cancelChannel) :);
    if(timer != 0){
        __asm__ __volatile__("freer res[%0]" : : "r" (timer) :);
        timer = 0;
    }
    // Temporarily disable checking cancellation events (it gets re-enabled when flushing the channel)
    // This means that even if the cancel channel technically has priority on being called first, it
    // lets the channel (which is more important) handle anything relevant.
    // Because this is a loop style section (I'm using a goto rather than a formal loop because XCC
    // is a pain, and I need a bit of flexibility in handling things) if there are no events, it
    // will simply continue until done.
    __asm__ __volatile__("edu res[%0]" : : "r" (cancelChannel) :);
    asyncCancelHandling:
    if(xthreads_spin_trylock_inner((xthreads_spin_t*) mutex, currentThreadId) == XTHREADS_ENONE){
        char waitCountTemp = mutex->waitCount - 1;
        mutex->waitCount = waitCountTemp;
        xthreads_channel_t channelTemp = mutex->channel;
        if(mutex->channel != ourChannel && waitCountTemp > 1){
            __asm__ __volatile__("setd res[%0], %1" : : "r" (ourChannel), "r" (channelTemp) :);
            mutexSignalTransmit(ourChannel,targetChannel,position);
        }
        else{
            mutex->channel = targetChannel;
            xthreads_spin_unlock_inner((xthreads_spin_t*) mutex);
        }
        __asm__ __volatile__("edu res[%0]" : : "r" (ourChannel) : );
        xthreads_task_endsudden(); // The thread kills itself here and doesn't return.
    }
    else{
        // If a message is being transmitted (i.e. the spinlock isn't available) we may need to flush the channel
        // We'll trick the event system into sending us back to the cancel handler by sending ourself a cancel message
        // That we will capture immediately in one of three cases:
        // 1. There is no pending event elsewhere in which case it is the chkct here
        // 2. There is a pending channel signal, which we handle with the chkct at the start of this cancel handler
        // 3. We've acquired the mutex and will handle it after returning from this function (after the call to setsr
        // in the channel handler)
        __asm__ __volatile__("outct res[%0], 1\n" : : "r" (cancelChannel) :);
        __asm__ __volatile__("setsr 0x1\n");
        __asm__ __volatile__("clrsr 0x1\n");
        __asm__ __volatile__("chkct res[%0], 1\n" : : "r" (cancelChannel) :);
        goto asyncCancelHandling;
    }
    // ---------------------------------------------------------------------------------------------------------------------

    /* This is essentially the missing return statements from the timer handling.
     * Because there's no apparent way to prevent XCC from culling what it thinks is unreachable
     * code, the first event has to be the final one to return which requires a very long branch
     * from the end of the timer code to the start of here
     *
     * Even more annoyingly, this has to be an asm command rather than a direct goto because the compiler
     * optimises around a goto and removes everything it skips.
     */

    // timerTriggeredReturn:
    __asm__ __volatile__("edu res[%0]\n"
                         "freer res[%1]" : : "r" (ourChannel), "r" (timer) : );
    if(cancelType != BLANK){
        xthreads_cancel_revector(cancelChannel);
        __asm__ __volatile__("eeu res[%0]" : : "r" (cancelChannel) :);
        if(cancelType == ASYNC){
            __asm__ __volatile__("setsr 0x1\n");
        }
    }
    // I would check, but the only way the code ever gets to here is when the timer
    // throws an event, meaning that it exists and is not unallocated
    return XTHREADS_ETIMEDOUT;
}
