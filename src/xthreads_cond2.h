/*
 * xthreads_cond2.h
 *
 *  Created on: 14 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_COND2_H_
#define XTHREADS_COND2_H_

#include "xthreads_mutex2.h"
#include "xthreads_spin.h"
#include "xthreads_self.h"

typedef struct xthreads_cond2{
    xthreads_t spinlock;
    xthreads_channel_t channel;
    char waitCount;
} xthreads_cond2_t;

int xthreads_cond2_init(xthreads_cond2_t *cond);
int xthreads_cond2_broadcast(xthreads_cond2_t *cond);
int xthreads_cond2_destroy(xthreads_cond2_t *cond);
int xthreads_cond2_signal(xthreads_cond2_t *cond);
int xthreads_cond2_timedwait(xthreads_cond2_t *cond, xthreads_mutex2_t *mutex, xthreads_time_t *time);
int xthreads_cond2_wait(xthreads_cond2_t *cond, xthreads_mutex2_t *mutex);

#endif /* XTHREADS_COND2_H_ */
