/*
 * xthreads_joindetach.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_JOINDETACH_H_
#define XTHREADS_JOINDETACH_H_

void xthreads_detach(xthreads_t threadId);
void *xthreads_join(xthreads_t threadId);

#endif /* XTHREADS_JOINDETACH_H_ */
