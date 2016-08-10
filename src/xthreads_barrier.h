/*
 * xthreads_barrier.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_BARRIER_H_
#define XTHREADS_BARRIER_H_

#include "xthreads_self.h"
#include "xthreads_spin.h"
#include "xthreads_channelstack.h"

typedef struct xthreads_barrier{
    xthreads_t templocked;
    xthreads_channelstack_t channelStack;
    char currentCount;
    char desiredCount;
    char cont; // Value to spinlock on
} xthreads_barrier_t;

typedef char xthreads_barrierattr_t;

int xthreads_barrier_init(xthreads_barrier_t *barrier, xthreads_barrierattr_t *attr);
int xthreads_barrier_wait(volatile xthreads_barrier_t *barrier);

#endif /* XTHREADS_BARRIER_H_ */
