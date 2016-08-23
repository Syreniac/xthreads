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
#include "xthreads_task.h"
#include "xthreads_util.h"
#include "xthreads_access.h"
#include "xthreads_update.h"

static void xthreads_cancel_enableEvents(const xthreads_t self);
#define XTHREADS_CANCELLED 0xD0


void xthreads_testcancel(void){
    xthreads_t self = xthreads_self();
    if(xthreads_access_cancelType(self) == DEFERRED &&
       (xthreads_access_cancelState(self) == ENABLED)){
        // I believe cycling the SR bit will cause pending interrupts to disrupt the system
        __asm__ __volatile__(
               "setsr 0x1\n"
               "clrsr 0x1\n"
        );
    }
    return;
}

int xthreads_setcancelstate(xthreads_cancelstate_t state, xthreads_cancelstate_t *oldState){
    xthreads_t self = xthreads_self();
    if(oldState != NULL){
        *oldState = xthreads_access_cancelState(self);
    }
    xthreads_update_cancelState(self,state);
    if(state == DISABLED){
        __asm__ __volatile__(
                "clrsr 0x1\n"
                "edu res[%0]\n"
                : : "r" (xthreads_access_cancelChannel(self)) :
        );
    }
    // I don't *think* I need to check that they weren't enabled because all this is doing
    // realistically is setting a bunch of bits to 1.
    // If it was already enabled, then it's doing 1->1 which does nothing
    else if(state == ENABLED){
        xthreads_cancel_enableEvents(self);
    }
    return XTHREADS_ENONE;
}

int xthreads_setcanceltype(xthreads_canceltype_t type, xthreads_canceltype_t *oldType){
    xthreads_t self = xthreads_self();
    if(oldType != NULL){
        *oldType = xthreads_access_cancelType(self);
    }
    xthreads_update_cancelType(self,type);

    if(type == ENABLED){
        xthreads_cancel_enableEvents(self);
    }
    return XTHREADS_ENONE;
}

static void xthreads_cancel_enableEvents(const xthreads_t self){
    const xthreads_channel_t cancelChannel = xthreads_access_cancelChannel(self);
    xthreads_cancel_revector(cancelChannel);
    switch(xthreads_access_cancelType(self)){
        case BLANK:
            exit(1);
        case DEFERRED:
            __asm__ __volatile__(
                "eeu res[%0]\n"
                : : "r" (cancelChannel) :
            );
            return;
        case ASYNC:
            __asm__ __volatile__(
                "eeu res[%0]\n"
                "setsr 0x1\n"
                : : "r" (cancelChannel) :
            );
            return;
    }
}

void xthreads_cancel(const xthreads_t thread){
    const xthreads_t self = xthreads_self();
    const xthreads_channel_t ourChannel = xthreads_access_threadChannel(self);
    const xthreads_channel_t cancelChannel = xthreads_access_cancelChannel(thread);

    if(cancelChannel != 0){
        __asm__ __volatile__("setd res[%0], %1\n"
                             "outct res[%0], 1\n"
                             : : "r" (ourChannel), "r" (cancelChannel) : );
    }
}

void xthreads_cancel_event(void){
    const xthreads_t self = xthreads_self();
    const xthreads_channel_t cancelChannel = xthreads_access_cancelChannel(self);
    __asm__ __volatile__("chkct res[%0], 1\n" : : "r" (cancelChannel) :);
    xthreads_task_endsudden();
}
