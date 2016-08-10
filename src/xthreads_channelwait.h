/*
 * xthreads_channelwait.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_CHANNELWAIT_H_
#define XTHREADS_CHANNELWAIT_H_

void xthreads_channelwait(xthreads_t thread);
int xthreads_channelwait_timed(xthreads_t thread, unsigned int time);

void xthreads_channelwait_nocancel(xthreads_t thread);
int xthreads_channelwait_nocancel_timed(xthreads_t thread, unsigned int time);

void *xthreads_channelwait_data(xthreads_t thread);
void *xthreads_channelwait_data_timed(xthreads_t thread, unsigned int time);

void *xthreads_channelwait_data_nocancel(xthreads_t thread);
void *xthreads_channelwait_data_nocancel_timed(xthreads_t thread, unsigned int time);

#endif /* XTHREADS_CHANNELWAIT_H_ */
