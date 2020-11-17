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
void releaseWaitProcNoClaim(int, int); 
  
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;
	struct lentry *lptr;
	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
    
    
    /*
    PSP:
    release locks
    if pid is waiting on a lock, then
        release that 
    */
    // release the locks
	int resched_flag = 0;
	int lockid = 0;
	for (; lockid < NLOCKS; lockid++) {
		if (pptr->locks_hold_list[lockid] == 1) {
            // nreaders being updated here        
			releaseLocksandAssignNextProc(pid, lockid);
			resched_flag = 1;
		}
	}
    // if in wait state then release that too
    if (pptr->pstate == PRWAIT) {
    	if (!isbadlockid(pptr->waitlockid)) {
    		pptr->pinh = 0;
    		releaseWaitProcNoClaim(pid, pptr->waitlockid);
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

	if (resched_flag == 1)
		resched();
	
	restore(ps);
	return(OK);
}

void releaseWaitProcNoClaim(int pid, int lockid) {
    // proc side
    dequeue(pid);
	struct pentry *pptr = &proctab[pid];
	pptr->waitlockid = BADPID;
	pptr->waittype = BADTYPE;
	pptr->wait_time_start = 0;
	pptr->lockreturn = DELETED;
    // lock side
	locktab[lockid].lprio = getMaxPrioWaitingProcs(lockid);
	cascadingRampUpPriorities(lockid);
}

