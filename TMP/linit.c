/* Initialize the Locks */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

void linit() {
	struct  lentry  *lptr;
	nextlock = NLOCKS - 1;
	int i = 0;
	for (; i < NLOCKS; i++) {
		lptr = &locktab[i];
		lptr->lstate = 	LFREE;
		lptr->ltype = DELETED;
		lptr->lprio = MININT;
        lptr->nreaders = 0;
		lptr->lqtail = 1 + (lptr->lqhead = newqueue());
        int j = 0;
		for (; j < NPROC; j++) {
			lptr->procs_hold_list[j] = UNACQUIRED;
        }
	}
}
