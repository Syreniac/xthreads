/*
 * xthreads_cond.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_COND_H_
#define XTHREADS_COND_H_

#include "xthreads_threadqueue.h"
#include "xthreads_mutex.h"
#include "xthreads_spin.h"
#include "xthreads_self.h"

typedef struct xthreads_cond{
    xthreads_t templocked;
    xthreads_threadqueue_t queue;
} xthreads_cond_t;

int xthreads_cond_init(xthreads_cond_t *cond);
int xthreads_cond_broadcast(xthreads_cond_t *cond);
int xthreads_cond_destroy(xthreads_cond_t *cond);
int xthreads_cond_signal(xthreads_cond_t *cond);
int xthreads_cond_timedwait(xthreads_cond_t *cond, xthreads_mutex_t *mutex, xthreads_time_t *time);
int xthreads_cond_wait(xthreads_cond_t *cond, xthreads_mutex_t *mutex);

#endif /* XTHREADS_COND_H_ */
