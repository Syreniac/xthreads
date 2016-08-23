/*
 * xthreads_once.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_ONCE_H_
#define XTHREADS_ONCE_H_

#define XTHREADS_ONCE_INIT -1

typedef xthreads_t xthreads_once_t;

int xthreads_once(volatile xthreads_once_t *controller, void (*routine)(void));

#endif /* XTHREADS_ONCE_H_ */
