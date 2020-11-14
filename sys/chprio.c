/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>

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
    
    pptr->pinh = newprio;
	pptr->pprio = newprio;
	
    /*
    PSP:
    get the lockid of the waiting lock
    dequeue this proc from the wait and reinsert
    priority inheritence if the new proc is greater
    */
    struct pentry *pptr = &proctab[pid];
    int lockid = pptr->waitlockid;
    struct lentry *lptr = &locktab[lockid];
    dequeue(pid);
    insert(pid, lptr->qhead, pptr->pinh);
    if (pptr->pinh > lptr->lprio) {
        lptr->lprio = pptr->pinh;
        prioInheritence(lockid, pptr->pinh);
    }
    
    restore(ps);
	return(newprio);
}
