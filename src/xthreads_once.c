/*
 * xthreads_once.c
 *
 *  Created on: 1 Aug 2016
 *      Author: Matthew
 */

#include "xthreads.h"
#include "xthreads_once.h"
#include "xthreads_self.h"

int xthreads_once(volatile xthreads_once_t *controller, void (*routine)(void)){
    if(*controller == XTHREADS_NOTHREAD){
        *controller = xthreads_self();
        __asm__ __volatile("nop");
        if(*controller == xthreads_self()){
            routine();
        }
    }
    return XTHREADS_ENONE;
}
