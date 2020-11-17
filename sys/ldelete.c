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
	/* reset bit mask of process ids currently holding the lock */
	int i = 0;
    for (; i < NPROC; i++) {
		if (lptr->procs_hold_list[i] == ACQUIRED) {
			lptr->procs_hold_list[i] = UNACQUIRED;
			proctab[i].bm_locks[lockdescriptor] = UNACQUIRED;
		}
	}	
	
	if (nonempty(lptr->lqhead)) {
    	int	pid = getfirst(lptr->lqhead);
		while(pid != EMPTY) {
		    proctab[pid].plockret = DELETED;
		    proctab[pid].wait_lockid = -1;	
		    ready(pid,RESCHNO);
		  }
		resched();
	}
	restore(ps);
	return(OK);
}
