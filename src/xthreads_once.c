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
    if(controller->doneBy == XTHREADS_NOTHREAD){
        controller->doneBy = xthreads_self();
        if(controller->doneBy == xthreads_self()){
            routine();
        }
    }
    return XTHREADS_ENONE;
}
