#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int releaseall (int numlocks, long locks,...) {
	STATWORD ps;
    disable(ps);
    
	struct pentry *pptr = &proctab[currpid];
	int ld;

    int i = 0;
	while (i < numlocks) {
        ld = (int *)(&locks) + i;
		/* check if lock descriptor passed is valid or not and is held by the calling process */
		if (isbadlock(ld))
            return SYSERR;
            
		if (rw_locks[ld].lproc_list[currpid] == 1)
			releaseLDForProc(currpid, ld);
		else
			return SYSERR;		
	}
	resched();

	restore(ps);
	return OK;	
}

void removeWaitingProcess(int pid, int ld, int type) {
    dequeue(pid);
	struct pentry *nptr = &proctab[pid];
	nptr->bm_locks[ld] = 1;
	struct lentry *lptr = &rw_locks[ld];
 	lptr->ltype = type;
	lptr->lproc_list[pid] = 1;
	nptr->wait_time = 0;
	nptr->wait_lockid = -1;
	nptr->wait_ltype = -1;
	ready(pid, RESCHNO);
}


void releaseLDForProc(int pid, int ld) {
	struct lentry *lptr = &rw_locks[ld];
	struct pentry *pptr = &proctab[pid];
	struct pentry *wptr;
	
	int oltype = lptr->ltype;
	int maxprio = -1;

	/* set ltype deleted temporarily */
	lptr->ltype = DELETED;
	lptr->lproc_list[pid] = 0;
	
	/* release ld lock from bit mask */
	pptr->bm_locks[ld] = 0;
	pptr->wait_lockid = -1;
	pptr->wait_ltype = -1;

	if (nonempty(lptr->lqhead)) {
		int writerProcExist = 0;
		int readerProcHoldingLock = 0;
		int wpid = 0;
		struct qent *mptr;
		unsigned long tdf = 0;
        maxprio = lastkey(lptr->lqtail);                                
		int prev = lastid(lptr->lqtail);

        /* check writer proc exist in queue  */
		while (prev != lptr->lqhead) {
			wptr = &proctab[prev];
			if (wptr->wait_ltype == WRITE) {
				writerProcExist = 1;
				wpid = prev;
				mptr = &q[wpid];
				break;		
			}
			prev = q[prev].qprev;
		}
		
		if (writerProcExist == 0) {
			prev = lastid(lptr->lqtail);
			while (!isbadpid(prev) && prev != lptr->lqhead) {
                removeWaitingProcess(prev, ld, READ);
            	prev = q[prev].qprev;
			}	
		}
		else if (writerProcExist == 1) {
			prev = lastid(lptr->lqtail);
			if (mptr->qkey == maxprio) {
				tdf = proctab[q[prev].qprev].wait_time - wptr->wait_time;
				if (tdf < 0)
					tdf = -tdf; /* make time difference positive */
				if (tdf < 1000) {
	                int i = 0;
					for (; i < NPROC; i++) {
						if (lptr->lproc_list[i] == 1) {
							readerProcHoldingLock = 1;
							break;
						}
					}
					if (readerProcHoldingLock == 0) {
                        removeWaitingProcess(wpid, ld, WRITE);
					}
				}
				else {
					prev = lastid(lptr->lqtail);
					while (prev != wpid) {
                        removeWaitingProcess(prev, ld, READ);
						prev = q[prev].qprev;
					}				
				}
			}
		}
	}

	lptr->lprio = getMaxPriorityInLockWQ(ld);
	maxprio = getMaxWaitProcPrioForPI(pid);
	
	if (maxprio > pptr->pprio)
		pptr->pinh = maxprio;
	else
		pptr->pinh = 0; /* as maxprio is either equal or less than original priority of pptr process */	
}
