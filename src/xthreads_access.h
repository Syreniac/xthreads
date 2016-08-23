/*
 * xthreads_other.h
 *
 *  Created on: 17 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_OTHER_H_
#define XTHREADS_OTHER_H_

#include "xthreads.h"
#include <stdio.h>
#include <stdlib.h>

#define topBit 0x80000000
#define assertAccess(thread) if(!(thread & topBit)){printf("attempting to access dead thread");exit(1);}

inline xthreads_t xthreads_confirm_access(xthreads_t thread){
    return thread | 0x80000000;
}

inline char xthreads_access_count(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.count;
}

inline char xthreads_access_detached(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.detached;
}

inline xthreads_t xthreads_access_threadId(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.threadId;
}

inline xthreads_t xthreads_access_resourceId(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.resourceId;
}

inline xthreads_channel_t xthreads_access_threadChannel(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.threadChannel;
}

inline xthreads_channel_t xthreads_access_cancelChannel(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cancelChannel;
}

inline void* xthreads_access_returnedValue(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.returnedValue;
}

inline xthreads_cleanup_function_t* xthreads_access_cleanup(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cleanup;
}

inline xthreads_cancelstate_t xthreads_access_cancelState(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cancelState;
}

inline xthreads_canceltype_t xthreads_access_cancelType(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cancelType;
}

inline void** xthreads_access_keys(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.keys;
}

inline xthreads_channel_t xthreads_access_returnChannel(xthreads_t thread){
    return xthreads_globals.stacks[thread % NUM_OF_THREADS].data.returnChannel;
}

inline xthreads_t xthreads_check_access(xthreads_t thread){
    xthreads_t remainder = (thread) - (thread % NUM_OF_THREADS);
    if(remainder == xthreads_access_count(thread)){
        return 1;
    }
    else{
        return 0;
    }
}

#endif /* XTHREADS_OTHER_H_ */
