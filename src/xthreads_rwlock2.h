/*
 * xthreads_rwlock2.h
 *
 *  Created on: 10 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_RWLOCK2_H_
#define XTHREADS_RWLOCK2_H_

typedef struct xthreads_rwlock2{
    xthreads_t spinlock;
    xthreads_channel_t rdChannel;
    xthreads_channel_t wrChannel;
    char rdWaitCount;
    char wrWaitCount;
} xthreads_rwlock2_t;

typedef char xthreads_rwlock2attr_t;

int xthreads_rwlock2_init(xthreads_rwlock2_t *rwlock, xthreads_rwlock2attr_t *attr);
int xthreads_rwlock2_rdlock(volatile xthreads_rwlock2_t *rwlock);
int xthreads_rwlock2_wrlock(volatile xthreads_rwlock2_t *rwlock);
int xthreads_rwlock2_unlock(xthreads_rwlock2_t *rwlock);

#endif /* XTHREADS_RWLOCK2_H_ */
