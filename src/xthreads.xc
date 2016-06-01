/*
 * xthreads.xc
 *
 *  Created on: 29 May 2016
 *      Author: Matthew
 */
#include <xs1.h>
#include <stdio.h>
#include <platform.h>

void function1(void){
    printf("hello");
}

interface my_interface{
    void function1();
};

[[distributable]]
void task(interface my_interface server i1, int a){
    int i = 0;
    timer tmr;
    unsigned time;
    tmr :> time;
    while(i < a){
        i++;
    }
    while(1){
        select{
            case i1.function1():
                    return;
        }
    }
}

void theoreticalCreateThread(void){
    asm("getr r4, 3");
    printf("1");
    asm("getst r11, res[r4]");
    printf("2");
    asm("init t[r11]:sp, r1");
    printf("3");
    /*asm("init t[r11]:pc, r0");
    printf("4");
    asm("set t[r11]:r0, r2");
    printf("5");
    asm("set t[r11]:r1, r3");
    printf("6");
    asm("ldaw r0, dp[0]");
    printf("7");
    asm("init t[r11]:dp, r0");
    printf("8");
    asm("add r1, r11, 0");
    printf("9");
    asm("ldaw r11, cp[0]");
    printf("10");
    asm("init t[r11]:cp, r11");
    printf("11");
    //asm("msync res[r4]");
    printf("12");*/
}

/*
    getr      r4,        3        // get a thread synchroniser, store in r4
    getst     r11,       res[r4]  // get synchronised thread, bound on the thread synchroniser in r4
    init      t[r11]:sp, r1       // set stack pointer of new thread
    init      t[r11]:pc, r0       // set PC of new thread
    set       t[r11]:r0, r2       // set r0, argument of new thread
    set       t[r11]:r1, r3       // set r1, chanend of new thread
    ldaw      r0,        dp[0]    // load value of data pointer in r0
    init      t[r11]:dp, r0       // set data pointer of new thread
    add       r1,        r11,  0  // copy thread resource id to r1
    ldaw      r11,       cp[0]    // load value of constant pointer in r11
    init      t[r1]:cp,  r11      // set constant pointer of new thread
    msync     res[r4]             // start new thread
 */

[[combinable]]
void task2(interface my_interface client i1, int a){
    int i = 0;
    timer tmr;
    unsigned time;
    tmr :> time;
    while(i < a){
        i++;
    }
    while(1){
        select{
            case tmr when timerafter(time) :> int now:
                    return;
        }
    }
}

int main(unsigned int argc, char *unsafe argv[argc]){
    /*interface my_interface i1;
    par{
        on tile[0].core[0]: task(i1,1);
        on tile[0].core[0]: task2(i1,1);
    }*/
    printf("A");
    //void * unsafe ptr = null;
    //unsafe{
    //}
    theoreticalCreateThread();
    printf("A");
    return 0;
}
