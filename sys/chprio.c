/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;
	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
    
    /*
    PSP  
    cal the new max prio and cascading
    based on newprio assign the new pinh
    */
    if (newprio > pptr->pprio)
		pptr->pinh = newprio;
	else {
		pptr->pprio = newprio;
		pptr->pinh = 0;
	}
    // calculating the new max prio and cascading if required
	int lockid = pptr->waitlockid;
    if (!isbadlockid(lockid)) {
		locktab[lockid].lprio = getMaxPrioWaitingProcs(lockid);
		cascadingRampUpPriorities(lockid);
	}
    
	restore(ps);
	return(newprio);
}
