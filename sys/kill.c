/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
    
    /*
    PSP:
    remove mapping from locktab and proctab
    if pid is waiting on a lock, then
        remove that
        dequeue it from the lock's wait queue
        change lprio if required
        call prioInheritence if required    
    */
    int i = 0;
    for(; i < NLOCKS; i++) {
        if (pptr->lock_types[i] != LNONE) {
            releaseLock(i, pid);
        }
    }
    int waitlockid = pptr->waitlockid;
    if (waitlockid != -1) {
        // not waiting
        pptr->waitlockid = -1;
        dequeue(pid);
        if (locktab[waitlockid].lprio == pptr->pinh && isSamePrioWaitProcPresent(waitlockid) == 0) {
            locktab[waitlockid].lprio = getAllMaxWaitingPrio(waitlockid);
            prioInheritence(waitlockid, locktab[waitlockid].lprio);
        }
    }
    
    
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
