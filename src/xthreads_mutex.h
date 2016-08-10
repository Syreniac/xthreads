/*
 * xthreads_mutex.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_MUTEX_H_
#define XTHREADS_MUTEX_H_

#include "xthreads_threadqueue.h"

typedef struct xthreads_mutex{
    xthreads_t templocked;
    xthreads_threadqueue_t queue;
    int activeThread;
    void *extra; // Certain mutex configurations need an extra value.
} xthreads_mutex_t;

typedef char xthreads_mutexattr_t;

int xthreads_mutex_init(xthreads_mutex_t *mutex, xthreads_mutexattr_t *attr);
int xthreads_mutex_lock(xthreads_mutex_t *mutex);
int xthreads_mutex_unlock(xthreads_mutex_t *mutex);
int xthreads_mutex_timedlock(xthreads_mutex_t *mutex, unsigned int time);

#endif /* XTHREADS_MUTEX_H_ */
