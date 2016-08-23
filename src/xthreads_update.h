/*
 * xthreads_update.h
 *
 *  Created on: 17 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_UPDATE_H_
#define XTHREADS_UPDATE_H_

#include "xthreads.h"

inline void xthreads_update_count(xthreads_t thread, char count){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.count = count;
}

inline void xthreads_update_detached(xthreads_t thread, char detached){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.detached = detached;
}

inline void xthreads_update_threadId(xthreads_t thread, xthreads_t threadId){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.threadId = threadId;
}

inline void xthreads_update_resourceId(xthreads_t thread, xthreads_resource_t resourceId){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.resourceId = resourceId;
}

inline void xthreads_update_threadChannel(xthreads_t thread, xthreads_channel_t threadChannel){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.threadChannel = threadChannel;
}

inline void xthreads_update_cancelChannel(xthreads_t thread, xthreads_channel_t cancelChannel){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cancelChannel = cancelChannel;
}

inline void xthreads_update_returnedValue(xthreads_t thread, void* returnedValue){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.returnedValue = returnedValue;
}

inline void xthreads_update_cleanup(xthreads_t thread, xthreads_cleanup_function_t *cleanup){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cleanup = cleanup;
}

inline void xthreads_update_cancelState(xthreads_t thread, xthreads_cancelstate_t cancelState){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cancelState = cancelState;
}

inline void xthreads_update_cancelType(xthreads_t thread, xthreads_canceltype_t cancelType){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.cancelType = cancelType;
}

inline void xthreads_update_returnChannel(xthreads_t thread, xthreads_canceltype_t returnChannel){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.returnChannel = returnChannel;
}

/*inline void xthreads_update_keys(xthreads_t thread, void** keys){
    xthreads_globals.stacks[thread % NUM_OF_THREADS].data.keys = keys;
}*/

#endif /* XTHREADS_UPDATE_H_ */
