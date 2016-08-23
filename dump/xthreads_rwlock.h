/*
 * xthreads_rwlock.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_RWLOCK_H_
#define XTHREADS_RWLOCK_H_

#include "xthreads_threadqueue.h"

typedef struct xthreads_rwlock{
    xthreads_t templocked;
    xthreads_threadqueue_t write_queue;
    xthreads_threadqueue_t read_queue;
    int activeThread;
} xthreads_rwlock_t;

typedef char xthreads_rwlockattr_t;

void xthreads_rwlock_init(xthreads_rwlock_t *rwlock, xthreads_rwlockattr_t *attr);
void xthreads_rwlock_rdlock(xthreads_rwlock_t *rwlock);
void xthreads_rwlock_wrlock(xthreads_rwlock_t *rwlock);
void xthreads_rwlock_unlock(xthreads_rwlock_t *rwlock);

#endif /* XTHREADS_RWLOCK_H_ */
