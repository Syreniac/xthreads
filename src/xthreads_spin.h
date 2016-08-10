/*
 * xthreads_spin.h
 *
 *  Created on: 5 Aug 2016
 *      Author: Matthew
 */

#ifndef XTHREADS_SPIN_H_
#define XTHREADS_SPIN_H_

typedef struct xthreads_spin{
    xthreads_t thread;
} xthreads_spin_t;

int xthreads_spin_init(xthreads_spin_t *spin);
int xthreads_spin_destroy(xthreads_spin_t *spin);
int xthreads_spin_lock(xthreads_spin_t *spin);
int xthreads_spin_unlock(xthreads_spin_t *spin);
int xthreads_spin_trylock(xthreads_spin_t *spin);
// Other structures inherit certain functionality of spin locks that these functions are useful for
// accessing.
void xthreads_spin_lock_inner(volatile xthreads_spin_t *spin, const xthreads_t currentThreadId);
void xthreads_spin_unlock_inner(xthreads_spin_t *spin);
int xthreads_spin_trylock_inner(xthreads_spin_t *spin, xthreads_t currentThreadId);

#endif /* XTHREADS_SPIN_H_ */
