/*
 * xthreads_channelstack.c
 *
 *  Created on: 3 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_channelstack.h"



/* Channel stacks form the basis of several synchronisation primitives in the current implementation of xthreads.
 * They allow us to use locks to replicate the behaviour of ssync and msync without having the unfortunate implications of
 * synchronised threads
 *
 * They're also used for thread-agnostic barriers.
 */

void xthreads_channelstack_init(volatile xthreads_channelstack_t *channelStack){
    channelStack->topChannel = XTHREADS_NOCHANNEL;
}

/* Implicitly not threadsafe, guard behind a spinlock or something */
/* UPDATE: Pass in the spinlock variable as an argument */
void xthreads_channelstack_start(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin){
    channelStack->topChannel = channel;
    xthreads_spin_unlock_inner(spin);
    __asm__ __volatile__(
            "chkct res[%0], 1"
            : : "r" (channel) :
    );
    return;
}

void *xthreads_channelstack_start_data(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin){
    channelStack->topChannel = channel;
    xthreads_spin_unlock_inner(spin);
    void* data = 0x0;
    __asm__ __volatile__(
            "in %0, res[%1]"
            : "=r" (data) : "r" (channel) :
    );
    return data;
}

void xthreads_channelstack_cascade(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin){
    xthreads_channel_t targetChannel = channelStack->topChannel;

    __asm__ __volatile__(
            "setd res[%0], %1\n"
            "outct res[%0], 1"
            : : "r" (channel), "r" (targetChannel) :
    );
    xthreads_channelstack_init(channelStack);
    xthreads_spin_unlock_inner(spin);
    return;
}

void xthreads_channelstack_cascade_data(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin, void* data){
    xthreads_channel_t targetChannel = channelStack->topChannel;
    channelStack->topChannel = channel;
    __asm__ __volatile__(
            "setd res[%0], %1\n"
            "out res[%0], %2"
            : : "r" (channel), "r" (targetChannel), "r" (data):
    );
    xthreads_channelstack_init(channelStack);
    xthreads_spin_unlock_inner(spin);
    return;
}

void xthreads_channelstack_pushwait(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin){
    xthreads_channel_t targetChannel = channelStack->topChannel;
    channelStack->topChannel = channel;
    xthreads_spin_unlock_inner(spin);

    __asm__ __volatile__(
            "setd res[%0], %1\n"
            "chkct res[%0], 1\n"
            "outct res[%0], 1"
            : : "r" (channel), "r" (targetChannel) :
    );
    return;
}

/* Also not threadsafe */
void *xthreads_channelstack_pushwait_data(volatile xthreads_channelstack_t *channelStack, xthreads_channel_t channel, xthreads_spin_t *spin){
    xthreads_channel_t targetChannel = channelStack->topChannel;
    channelStack->topChannel = channel;
    register void* data = 0x0;
    xthreads_spin_unlock_inner(spin);
    __asm__ __volatile__(
            "setd res[%1], %2\n"
            "in %0, res[%1]\n"
            "out res[%1], %3"
            : "=r" (data) : "r" (channel), "r" (targetChannel), "r" (data):
    );
    return data;
}
