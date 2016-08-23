/*
 * xthreads_task.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_TASK_H_
#define XTHREADS_TASK_H_

#include "xthreads.h"

void xthreads_task(void *(*target)(void*), void* args, xthreads_data_t* threadData);
void xthreads_task_end(xthreads_data_t *threadData);
void xthreads_task_endsudden(void);
void xthreads_task_exit(void* retval);

#endif /* XTHREADS_TASK_H_ */
