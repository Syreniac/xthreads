/*
 * xthreads_channelstack.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_CHANNELSTACK_H_
#define XTHREADS_CHANNELSTACK_H_

#include "xthreads_channelstack.h"
#include "xthreads_spin.h"

#define XTHREADS_NOCHANNEL -1

typedef struct xthreads_channelstack{
  xthreads_channel_t topChannel;
} xthreads_channelstack_t;

void xthreads_channelstack_init(volatile xthreads_channelstack_t *channelStack);
void xthreads_channelstack_start(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin);
void *xthreads_channelstack_start_data(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin);
void xthreads_channelstack_cascade(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin);
void xthreads_channelstack_cascade_data(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin, void* data);
void xthreads_channelstack_pushwait(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin);
void *xthreads_channelstack_pushwait_data(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin);


#endif /* XTHREADS_CHANNELSTACK_H_ */
