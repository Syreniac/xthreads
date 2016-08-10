/*
 * xthreads_cancel.c
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_cancel.h"
#include "xthreads_errors.h"
#include "xthreads_self.h"

static void xthreads_cancel_enableEvents(xthreads_data_t *dataPointer);

void xthreads_cancel(xthreads_t thread){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_channel_t chanend = xthreads_globals.stacks[currentThreadId].data.threadChannel;
    xthreads_channel_t chandest = xthreads_globals.stacks[thread].data.cancelChannel;
    __asm__ __volatile__(
            "setd res[%0], %1\n"
            "outct res[%0], 1\n"
            : : "r" (chanend), "r" (chandest) :
    );
}

void xthreads_testcancel(void){
    xthreads_t currentThreadId = xthreads_self();
    xthreads_data_t* dataPointer = &xthreads_globals.stacks[currentThreadId].data;
    if(dataPointer->cancelState == XTHREADS_CANCEL_ENABLED && dataPointer->cancelType == XTHREADS_CANCEL_DEFERRED){
        // I believe cycling the SR bit will cause pending interrupts to disrupt the system
        __asm__ __volatile__(
               "setsr 0x1\n"
               "clrsr 0x1\n"
        );
    }
    return;
}

int xthreads_setcancelstate(int state, int *oldState){
    xthreads_t currentThreadId = xthreads_self();
    *oldState = xthreads_globals.stacks[currentThreadId].data.cancelState;
    xthreads_globals.stacks[currentThreadId].data.cancelState = state;
    if(state == XTHREADS_CANCEL_DISABLED){
        __asm__ __volatile__(
                "clrsr 0x1\n"
                "edu res[%0]\n"
                : : "r" (xthreads_globals.stacks[currentThreadId].data.cancelChannel) :
        );
    }
    // I don't *think* I need to check that they weren't enabled because all this is doing
    // realistically is setting a bunch of bits to 1.
    // If it was already enabled, then it's doing 1->1 which does nothing
    else if(state == XTHREADS_CANCEL_ENABLED){
        xthreads_cancel_enableEvents(&xthreads_globals.stacks[currentThreadId].data);
    }
    return XTHREADS_ENONE;
}

int xthreads_setcanceltype(int type, int *oldType){
    xthreads_t currentThreadId = xthreads_self();
    *oldType = xthreads_globals.stacks[currentThreadId].data.cancelType;
    xthreads_globals.stacks[currentThreadId].data.cancelType = type;

    if(xthreads_globals.stacks[currentThreadId].data.cancelState == XTHREADS_CANCEL_ENABLED){
        xthreads_cancel_enableEvents(&xthreads_globals.stacks[currentThreadId].data);
    }
    return XTHREADS_ENONE;
}

static void xthreads_cancel_enableEvents(xthreads_data_t *dataPointer){
    switch(dataPointer->cancelType){
        case XTHREADS_CANCEL_DEFERRED:
            __asm__ __volatile__(
                "eeu res[%0]\n"
                : : "r" (dataPointer->cancelChannel) :
            );
        case XTHREADS_CANCEL_ASYNC:
            __asm__ __volatile__(
                "eeu res[%0]\n"
                "setsr 0x1\n"
                : : "r" (dataPointer->cancelChannel) :
            );
    }
}
