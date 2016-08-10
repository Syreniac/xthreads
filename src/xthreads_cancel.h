/*
 * xthreads_cancel.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_CANCEL_H_
#define XTHREADS_CANCEL_H_

#define XTHREADS_CANCEL_DISABLED 0x00
#define XTHREADS_CANCEL_ENABLED 0x10
#define XTHREADS_CANCEL_DEFERRED 0x00
#define XTHREADS_CANCEL_ASYNC 0x10

void xthreads_cancel(xthreads_t threadId);
void xthreads_testcancel(void);
int xthreads_setcanceltype(int type, int *oldType);
int xthreads_setcancelstate(int state, int *oldState);

inline int xthreads_cancellationPointCheck(xthreads_t currentThreadId){
   return (xthreads_globals.stacks[currentThreadId].data.cancelState == XTHREADS_CANCEL_ENABLED &&
           xthreads_globals.stacks[currentThreadId].data.cancelType == XTHREADS_CANCEL_DEFERRED);
}

#endif /* XTHREADS_CANCEL_H_ */
