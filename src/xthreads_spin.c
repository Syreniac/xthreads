/*
 * xthreads_spin.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_spin.h"
#include "xthreads_self.h"

int xthreads_spin_init(xthreads_spin_t *spin){
    spin->thread = XTHREADS_NOTHREAD;
    return XTHREADS_ENONE;
}

int xthreads_spin_destroy(xthreads_spin_t *spin){
    // No resources allocated!
    return XTHREADS_ENONE;
}

int xthreads_spin_lock(xthreads_spin_t *spin){
    xthreads_spin_lock_inner(spin, xthreads_self());
    return XTHREADS_ENONE;
}

/* Should technically return an error if a deadlock is detected, but I don't think there's a way of
 * reliably picking up on deadlocks here.
 *
 * The obvious one of a thread trying to relock a spin lock should just cause it to pass through silently
 */
void xthreads_spin_lock_inner(volatile xthreads_spin_t *spin, const xthreads_t currentThreadId){
    register volatile xthreads_t locked = spin->thread;
    while(1){
        if(spin->thread == XTHREADS_NOTHREAD){
            spin->thread = currentThreadId;
            __asm__ __volatile__("nop");
            __asm__ __volatile__("ldw %0, %1[0]" : "=r" (locked) : "r" (spin) :);
            if(locked == currentThreadId){
                return;
            }
        }
    }
}

int xthreads_spin_unlock(xthreads_spin_t *spin){
    xthreads_spin_unlock_inner(spin);
    return XTHREADS_ENONE;
}

void xthreads_spin_unlock_inner(xthreads_spin_t *spin){
    spin->thread = XTHREADS_NOTHREAD;
}

int xthreads_spin_trylock(xthreads_spin_t *spin){
    return xthreads_spin_trylock_inner(spin,xthreads_self());
}

int xthreads_spin_trylock_inner(volatile xthreads_spin_t *spin, const xthreads_t currentThreadId){
    register volatile xthreads_t locked = spin->thread;
    if(spin->thread == XTHREADS_NOTHREAD){
        spin->thread = currentThreadId;
        __asm__ __volatile__("nop");
        __asm__ __volatile__("ldw %0, %1[0]" : "=r" (locked) : "r" (spin) :);
        if(locked == currentThreadId){
            return XTHREADS_ENONE;
        }
    }
    return XTHREADS_EBUSY;
}

void xthreads_spin_lock_inner_events(volatile xthreads_spin_t *spin, const xthreads_t currentThreadId){
    register volatile xthreads_t locked = spin->thread;
    while(1){
        if(spin->thread == XTHREADS_NOTHREAD){
            spin->thread = currentThreadId;
            __asm__ __volatile__("nop");
            __asm__ __volatile__("ldw %0, %1[0]" : "=r" (locked) : "r" (spin) :);
            if(locked == currentThreadId){
                return;
            }
        }
        __asm__ __volatile__("setsr 0x1\n"
                             "clrsr 0x1\n");
    }
}


