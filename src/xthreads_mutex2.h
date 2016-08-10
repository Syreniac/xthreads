/*
 * xthreads_mutex2.h
 *
 *  Created on: 8 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_MUTEX2_H_
#define XTHREADS_MUTEX2_H_

#include "xthreads.h"

typedef char xthreads_mutex2attr_t;

typedef struct xthreads_mutex2{
    xthreads_t spinlock;
    xthreads_channel_t channel;
    char waitCount;
} xthreads_mutex2_t;

int xthreads_mutex2_init(xthreads_mutex2_t *mutex, xthreads_mutex2attr_t *attr);
int xthreads_mutex2_lock(volatile xthreads_mutex2_t *mutex);
int xthreads_mutex2_unlock(xthreads_mutex2_t *mutex);
int xthreads_mutex2_timedlock(volatile xthreads_mutex2_t *mutex, xthreads_time_t time);

#endif /* XTHREADS_MUTEX2_H_ */
