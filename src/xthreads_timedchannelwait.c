/*
 * xthreads_timedchannelwait.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include <stdio.h>
#include <stdlib.h>

/* Because the code to wait on a timer and channel at once are used in numerous places, I've put it here */


int xthreads_timedchannelwait(xthreads_channel_t channelEnd, xthreads_time_t time){


    register volatile xthreads_resource_t timer asm("r5");
    volatile register int value asm("r2") = 0;
    __asm__ __volatile__(
          "getr %0, 1\n" // Get a timer"
          : "=r" (timer) : : );

    if(timer == 0){
        printf("xthreads exception: unable to get timer for timed channel wait\n");
        exit(1);
    }

    __asm__ __volatile__(
            "setd res[%0], %1\n" // Set the timer's time
            "setc res[%0], 0x9\n"
            : : "r" (timer), "r" (time) :
    );

    __asm__ __volatile__(
          "clre\n" // Clear all enabled events (there shouldn't be many)
          "ldap r11, 0x7\n"
          "setv res[%1], r11\n" //Mark where the channel end sends us
          "eeu res[%1]\n" //Mark the channel end as giving us an interrupt
          "ldap r11, 0x8\n"
          "setv res[%2], r11\n"
          "eeu res[%2]\n"
          "waiteu\n"
          "chkct res[%1], 1\n"
          "inct r10, res[%1]\n"
          "ldc r9, %3\n"
          "bu 0x3\n"
          "in r10, res[%2]\n"
          "ldc r9, %4\n"
          "edu res[%1]\n"
          "edu res[%2]\n"
          "freer res[%2]\n"
          "mov %0, r9\n"
    : "=r" (value) : "r" (channelEnd), "r" (timer), "i" (XTHREADS_ENONE), "i" (XTHREADS_ETIMEDOUT): "r11","r10","r9");
    return value;
}
