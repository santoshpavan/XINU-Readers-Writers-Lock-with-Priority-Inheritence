/* Delete the lock descriptor passed */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

int ldelete (int lock) {
    STATWORD ps;    
	int	pid;
	struct	lentry	*lptr = &locktab[lock];

	disable(ps);
	if (isbadsem(lock) || lptr->lstate == LFREE) {
		restore(ps);
		return(SYSERR);
	}
	lptr->lstate = LFREE;
	if ( nonempty(lptr->lqhead) ) {
		while( (pid = getfirst(lptr->lqhead)) != EMPTY)
		  {
		    proctab[pid].pwaitret = DELETED;
		    ready(pid, RESCHNO);
		  }
		resched();
	}
	restore(ps);
	return(OK);
}
