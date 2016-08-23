/*
 * xthreads_cleanup.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_CLEANUP_H_
#define XTHREADS_CLEANUP_H_

#include "xthreads.h"

void xthreads_cleanup_push(void (*routine)(void*), void *arg);
void xthreads_cleanup_pop(int execute);
void xthreads_cleanup_drop(xthreads_cleanup_function_t *predrop);

#endif /* XTHREADS_CLEANUP_H_ */
