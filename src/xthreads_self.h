/*
 * xthreads_self.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_SELF_H_
#define XTHREADS_SELF_H_

#include "xthreads.h"

xthreads_t xthreads_self(void);
xthreads_t xthreads_self_outer(void);

#define xthreads_self_modulo(self) (self % NUM_OF_THREADS)

inline xthreads_channel_t xthreads_self_threadChannel(xthreads_t self){
    return xthreads_globals.stacks[self].data.threadChannel;
}

inline xthreads_channel_t xthreads_self_cancelChannel(xthreads_t self){
    return xthreads_globals.stacks[self].data.cancelChannel;
}

inline int xthreads_self_cancelStatus(xthreads_t self){
    return xthreads_globals.stacks[self].data.cancelState;
}

inline int xthreads_self_cancelType(xthreads_t self){
    return xthreads_globals.stacks[self].data.cancelType;
}

inline char xthreads_self_detached(xthreads_t self){
    return xthreads_globals.stacks[self].data.detached;
}

inline xthreads_channel_t xthreads_self_returnChannel(xthreads_t self){
    return xthreads_globals.stacks[self].data.returnChannel;
}

#endif /* XTHREADS_SELF_H_ */
