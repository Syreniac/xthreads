/*
 * xthreads_cancel.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_CANCEL_H_
#define XTHREADS_CANCEL_H_

#include "xthreads.h"

void xthreads_cancel(xthreads_t threadId);
void xthreads_testcancel(void);
int xthreads_setcanceltype(xthreads_canceltype_t type, xthreads_canceltype_t *oldType);
int xthreads_setcancelstate(xthreads_cancelstate_t state, xthreads_cancelstate_t *oldState);
void xthreads_cancel(const xthreads_t thread);
void xthreads_cancel_event(void);

inline void xthreads_cancel_revector(xthreads_channel_t cancelChannel){
    void *revector = &xthreads_cancel_event;
    __asm__ __volatile__("setv res[%0], %1\n" : : "r" (cancelChannel), "r" (revector) :);
}

#endif /* XTHREADS_CANCEL_H_ */
