/*
 * xthreads_channelwait.c
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_channelwait.h"
#include "xthreads_util.h"
#include <stdio.h>
#include <stdlib.h>

void xthreads_channelwait(xthreads_t thread){
    xthreads_channel_t signalChannel = xthreads_globals.stacks[thread].data.threadChannel;
    xthreads_channel_t cancelChannel = xthreads_globals.stacks[thread].data.cancelChannel;
    char eeble = xthreads_getEeble();
    __asm__ __volatile__(
            "ldap r11, 0x5\n"
            "setv res[%0], r11\n"
            "eeu res[%0]\n"
            "eeu res[%1]\n"
            "waiteu\n"
            "chkct res[%0], 1\n"
            // Reset the SR to the state it was in before we messed with it
            "edu res[%1]\n"
            "edu res[%0]\n"
            "bf %2, 0x1\n"
            "clrsr 0x1\n"
            : : "r" (signalChannel), "r" (cancelChannel), "r" (eeble) :
    );
    return;
}

int xthreads_channelwait_timed(const xthreads_t thread, const unsigned int time){
    const xthreads_channel_t signalChannel = xthreads_globals.stacks[thread].data.threadChannel;
    const xthreads_channel_t cancelChannel = xthreads_globals.stacks[thread].data.cancelChannel;
    register xthreads_resource_t timer asm("r5");
    register volatile int value = 0;
    char eeble = xthreads_getEeble();
    __asm__ __volatile__(
          "getr %0, 1\n" // Get a timer"
          : "=r" (timer) : : );

    if(timer == 0){
        printf("xthreads exception: unable to get timer for timed mutex lock\n");
        exit(1);
    }

    prepTimer(timer,time);

    __asm__ __volatile__(
            "ldap r11, 0x8\n"
            "setv res[%1], r11\n"
            "ldap r11, 0x9\n"
            "setv res[%2], r11\n"
            // Enable events
            "eeu res[%1]\n"
            "eeu res[%2]\n"
            "eeu res[%6]\n"
            // Wait
            "waiteu\n"
            // Signal channel handling
            "chkct res[%1], 1\n"
            "ldc %0, %3\n"
            "bu 0x3\n"
            // Timer handling
            "in r10, res[%2]\n"
            "ldc %0, %4\n"
            // Cancel channel handling predefined elsewhere (xthreads_create())
            // Disable events
            "edu res[%1]\n"
            "edu res[%2]\n"
            "edu res[%6]\n"
            // Reset the SR to the state it was in before we messed with it
            "bf %5, 0x1\n"
            "clrsr 0x1\n"
            : "=r" (value) : "r" (signalChannel), "r" (timer), "i" (XTHREADS_ENONE), "i" (XTHREADS_ETIMEDOUT), "r" (eeble) , "r" (cancelChannel) : "r11", "r10"
    );
    return value;
}

void xthreads_channelwait_nocancel(xthreads_t thread){
    const xthreads_channel_t signalChannel = xthreads_globals.stacks[thread].data.threadChannel;
    __asm__ __volatile__(
            "chkct res[%0], 1"
            : : "r" (signalChannel) :
    );
    return;
}
int xthreads_channelwait_nocancel_timed(xthreads_t thread, unsigned int time){
    register xthreads_channel_t signalChannel asm("r9") = xthreads_globals.stacks[thread].data.threadChannel;
    register xthreads_resource_t timer asm("r8");
    register volatile int value = 0;
    char eeble = xthreads_getEeble();
    __asm__ __volatile__(
          "getr %0, 1\n" // Get a timer"
          : "=r" (timer) : : );

    if(timer == 0){
        printf("xthreads exception: unable to get timer for timed mutex lock\n");
        exit(1);
    }
    prepTimer(timer,time);

    __asm__ __volatile__(
            "ldap r11, 0x7\n"
            "setv res[%1], r11\n"
            "ldap r11, 0x8\n"
            "setv res[%2], r11\n"
            // Enable events
            "eeu res[%1]\n"
            "eeu res[%2]\n"
            // Wait
            "waiteu\n"
            // Signal channel handling
            "chkct res[%1], 1\n"
            "ldc %0, %3\n"
            "bu 0x3\n"
            // Timer handling
            "in r10, res[%2]\n"
            "ldc %0, %4\n"
            // Cancel channel handling predefined elsewhere (xthreads_create())
            // Disable events
            "edu res[%1]\n"
            "edu res[%2]\n"
            "bf %5, 0x1\n"
            "clrsr 0x1\n"
            : "=r" (value) : "r" (signalChannel), "r" (timer), "i" (XTHREADS_ENONE), "i" (XTHREADS_ETIMEDOUT), "r" (eeble) : "r11", "r10"
    );
    return value;
}
