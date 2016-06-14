/*
 * xthreads.h
 *
 *  Created on: 14 Jun 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_H_
#define XTHREADS_H_

// The identifier for a thread
typedef int xthreads_t;

typedef struct xthreads_attr{
    char detachState;
} xthreads_attr_t;

#endif /* XTHREADS_H_ */
