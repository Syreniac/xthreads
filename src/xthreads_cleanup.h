/*
 * xthreads_cleanup.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_CLEANUP_H_
#define XTHREADS_CLEANUP_H_

typedef struct xthreads_cleanup_function xthreads_cleanup_function_t;

struct xthreads_cleanup_function{
    void (*function)(void*);
    void* arg;
    xthreads_cleanup_function_t *next;
};

void xthreads_cleanup_push(void (*routine)(void*), void *arg);
void xthreads_cleanup_pop(int execute);
void xthreads_cleanup_drop(xthreads_cleanup_function_t *cleanup, int execute);

#endif /* XTHREADS_CLEANUP_H_ */
