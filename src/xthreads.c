/*
 * xthreads.c
 *
 *  Created on: 31 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"

xthreads_global_t xthreads_globals;

__attribute__ ((constructor)) void xthreads_configureMain(){
    printf("configuring main\n");
    xthreads_t mainThreadId = 0;
    __asm__ __volatile__(
            "get r11, id\n"
            "mov %0, r11\n"
            : "=r" (mainThreadId) : : "r11"
    );
    xthreads_globals.stacks[0].data.threadId = mainThreadId;
    xthreads_globals.stacks[0].data.resourceId = 0xffffffff;
    xthreads_channel_t threadChannel = 0;
    xthreads_channel_t cancelChannel = 0;
    __asm__ __volatile__(
            "getr %0, 2\n"
            "getr %1, 2\n"
            : "=r" (threadChannel), "=r" (cancelChannel) : :
    );
    xthreads_globals.stacks[0].data.threadChannel = threadChannel;
    xthreads_globals.stacks[0].data.cancelChannel = cancelChannel;
}
