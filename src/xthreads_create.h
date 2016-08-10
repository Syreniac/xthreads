/*
 * xthreads_create.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_CREATE_H_
#define XTHREADS_CREATE_H_

typedef struct xthreads_attr{
    /* threads that cannot be joined are unsynchronised threads */
    char detachState;
} xthreads_attr_t;

void xthreads_create(xthreads_t *thread, const xthreads_attr_t *attr, void *(start_routine)(void*), void *arg);

#endif /* XTHREADS_CREATE_H_ */
