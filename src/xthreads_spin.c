/*
 * xthreads_spin.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_spin.h"
#include "xthreads_self.h"
#include "assert.h"

int xthreads_spin_init(xthreads_spin_t *spin){
    spin->thread = XTHREADS_NOTHREAD;
    return XTHREADS_ENONE;
}

int xthreads_spin_destroy(xthreads_spin_t *spin){
    // No resources allocated!
    return XTHREADS_ENONE;
}

int xthreads_spin_lock(xthreads_spin_t *spin){
    xthreads_t self = xthreads_self();
    xthreads_spin_lock_inner(spin, self);
    assert(spin->thread == self);
    return XTHREADS_ENONE;
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

/* Should technically return an error if a deadlock is detected, but I don't think there's a way of
 * reliably picking up on deadlocks here.
 */
void xthreads_spin_lock_inner(xthreads_spin_t *spin, const xthreads_t currentThreadId){
    __asm__ __volatile__(
            "ldw r5, %0[0]\n"
            "add r5, r5, 1\n"
            "bt r5, -0x3\n"
            "stw %1, %0[0]\n"
            "nop\n"
            "nop\n"
            "nop\n"
            "ldw r5, %0[0]\n"
            "eq r6, r5, %1\n"
            "bt r6, 0x1\n"
            "bu -0xB\n"
            : : "r" (spin), "r" (currentThreadId) : "r5","r6"
    );
    return;

    /*register volatile xthreads_t locked = spin->thread;
    while(1){
        if(spin->thread == XTHREADS_NOTHREAD){
            spin->thread = currentThreadId;
            __asm__ __volatile__("nop");
            __asm__ __volatile__("ldw %0, %1[0]" : "=r" (locked) : "r" (spin) :);
            if(locked == currentThreadId){
                return;
            }
        }
    }*/
}

