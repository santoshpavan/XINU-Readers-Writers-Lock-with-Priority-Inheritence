/* Delete the lock descriptor passed */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>


SYSCALL ldelete(int lockdescriptor) {
	STATWORD ps;    
	disable(ps);
    
	if (isbadlockid(lockdescriptor) || locktab[lockdescriptor].lstate==LFREE) {
		restore(ps);
		return(SYSERR);
	}
    
    struct	lentry	*lptr = &locktab[lockdescriptor];
	lptr->lstate = LFREE;
	lptr->ltype = DELETED;
	lptr->lprio = -1;
	// reset the mappings
	int i = 0;
    for (; i < NPROC; i++) {
		if (lptr->procs_hold_list[i] == ACQUIRED) {
			lptr->procs_hold_list[i] = UNACQUIRED;
			proctab[i].locks_hold_list[lockdescriptor] = UNACQUIRED;
		}
	}
	
	if (nonempty(lptr->lqhead)) {
    	int	pid = getfirst(lptr->lqhead);
		while(pid != EMPTY) {
		    proctab[pid].lockreturn = DELETED;
		    proctab[pid].waitlockid = BADPID;	
		    ready(pid,RESCHNO);
		  }
		resched();
	}
	restore(ps);
	return(OK);
}
