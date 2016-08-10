/*
 * xthreads_util.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_UTIL_H_
#define XTHREADS_UTIL_H_

#include <stdio.h>

#define prepTimer(timerr,time){__asm__ __volatile__("setd res[%0], %1\n" "setc res[%0], 0x9\n" : : "r" (timerr), "r" (time) :);}

inline void xthreads_printBytes(int i);
inline char xthreads_getEeble(void);

inline void xthreads_printBytes(int i){
    printf("%x%x%x%x\n",((i >> 24) & 0xff),((i >> 16) & 0xff),((i >> 8) & 0xff),(i & 0xff));
}

inline char xthreads_getEeble(void){
    char eeble;
    __asm__ __volatile__(
            "getsr r11, 0x1\n"
            "mov %0, r11\n"
            : "=r" (eeble) : :
    );
    return eeble;
}

inline void xthreads_lockThreadHeaders(){
    // Experimentally, I'm going to have a lock for the thread headers
    if(xthreads_globals.lock == 0){
        __asm__ __volatile__("getr %0, 5" : "=r" (xthreads_globals.lock) : :);
    }
    __asm__ __volatile__("in r0, res[%0]" : : "r" (xthreads_globals.lock) : "r0");
}

inline void xthreads_unlockThreadHeaders(){
    __asm__ __volatile__("out res[%0], r0" : : "r" (xthreads_globals.lock) : "r0" );
}


#endif /* XTHREADS_UTIL_H_ */
