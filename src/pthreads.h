/*
 * pthreads.h
 *
 *  Created on: 5 Jul 2016
 *      Author: Matthew
 */


#ifndef PTHREADS_H_
#define PTHREADS_H_

#include "xthreads.h"


#define pthread_t xthreads_t
#define pthread_attr_t xthreads_attr_t

#define pthread_attr_init(attributes){attributes->detachState = XTHREADS_CREATE_UNDETACHED;}
#define pthread_attr_destroy(attributes){}

#define pthread_attr_getdetachstate(attributes, intPointer){*iPointer = attributes->detachState;}
#define pthread_attr_getguardsize(attributes, sizePointer){*sizePointer = 0;}
#define pthread_attr_getinheritsched(attributes, intPointer){*iPointer = 0;}
#define pthread_attr_getschedparam(attributes,schedPointer){*schedPointer = NULL;}
#define pthread_attr_getschedpolicy(attributes, intPointer){*intPointer = NULL;}
#define pthread_attr_getscope(attributes, intPointer){*intPointer = NULL;}
#define pthread_attr_getstackaddr(attributes, voidPointer){*voidPointer = NULL}
#define pthread_attr_getstacksize(attributes, sizePointer){*sizePointer = NULL;}

#define pthread_attr_setdetachstate(attributes, intArg){attributes->detachState = intArg;}
#define pthread_attr_setguardsize(attributes, size_t){}
#define pthread_attr_setinheritsched(attributes, intArg){}
#define pthread_attr_setschedparam(attributes, schedPointer){}
#define pthread_attr_setschedpolicy(attributes, intArg){}
#define pthread_attr_setscope(attributes, intArg){}
#define pthread_attr_setstackaddr(attributes, voidPointer){}
#define pthread_attr_setstacksize(attributes, size){}

#define pthread_create(threadIdPointer, attributes, target, arg){xthreads_create(threadIdPointer, attributes, target, arg);};

#define pthread_detach(threadId){xthreads_detach(threadId)}
#define pthread_join(threadId, voidPointer)(xthreads_join(threadId))
#define pthread_equal(threadIdA, threadIdB){return threadIdA == threadIdB}
#define pthread_self(){__asm__ __volatile__("get r11, id" : : : "r11");}



#endif /* PTHREADS_H_ */
