#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

void removeWaitingProcess(int pid, int ld, int type);
int isWriterProcessPresentInWaiting(int ld);
void releaseLDForProc(int pid, int ld);

int releaseall (int numlocks, long locks,...) {
	/*
    release the mentioned locks held by this process
    if the lock is not of the proc -> SYSERR
    after release of each lock, assign the lock to another process in its queue
    */
    STATWORD ps;
    disable(ps);
    
	struct pentry *pptr = &proctab[currpid];
	int ld;

    int i = 0;
	while (i < numlocks) {
        ld = (int *)(&locks) + i;
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
	struct pentry *nptr = &proctab[pid];
	struct lentry *lptr = &rw_locks[ld];
    dequeue(pid);
	nptr->bm_locks[ld] = 1;
 	lptr->ltype = type;
	lptr->lproc_list[pid] = 1;
	nptr->wait_time = 0;
	nptr->wait_lockid = -1;
	nptr->wait_ltype = -1;
	ready(pid, RESCHNO);
}

int isWriterProcessPresentInWaiting(int ld) {
    // check if there is a writer process in waiting queue
    int prev = lastid(rw_locks[ld].lqtail);
    while (prev != rw_locks[ld].lqhead) {
		if (proctab[prev].wait_ltype == WRITE)
			return prev;
	    prev = q[prev].qprev;
	}
    return -1;
}

void releaseLDForProc(int pid, int ld)
{
	struct lentry *lptr;
	struct pentry *pptr;
	struct pentry *nptr;
	struct pentry *wptr;
	
	int oltype = lptr->ltype;
	int maxprio = -1;
	int i=0;

	lptr = &rw_locks[ld];
	pptr = &proctab[pid];

	/* set ltype deleted temporarily */
	lptr->ltype = DELETED;
	lptr->lproc_list[pid] = 0;
	
	/* release ld lock from bit mask */
	pptr->bm_locks[ld] = 0;
	pptr->wait_lockid = -1;
	pptr->wait_ltype = -1;

	if (nonempty(lptr->lqhead)) {
		int prev = lptr->lqtail;
		int writerProcExist = 0;
		int readerProcHoldingLock = 0;
		int wpid = 0;
		struct qent *mptr;
		unsigned long tdf = 0;	
		maxprio = q[q[prev].qprev].qkey;
		
		while (q[prev].qprev != lptr->lqhead) {
			prev = q[prev].qprev;
			wptr = &proctab[prev];
			if (wptr->wait_ltype == WRITE) {
				writerProcExist = 1;
				wpid = prev;
				mptr = &q[wpid];
				break;		
			}	
		}
		
		if (writerProcExist == 0) {
			prev = lptr->lqtail;
			while (q[prev].qprev != lptr->lqhead && q[prev].qprev < NPROC && q[prev].qprev > 0) {	
				prev = q[prev].qprev;
				removeWaitingProcess(prev, ld, READ);
			}	
		}
		else if (writerProcExist == 1) {
			prev = lptr->lqtail;
			if (mptr->qkey == maxprio) {
				tdf = proctab[q[prev].qprev].wait_time - wptr->wait_time;
				if (tdf < 0) {
					tdf = (-1)*tdf;
				}
				if (tdf < 1000) {
					for (i = 0;i < NPROC;i++) {
						if (lptr->lproc_list[i] == 1) {
							readerProcHoldingLock = 1;
							break;
						}
					}
					if (readerProcHoldingLock == 0)	{
						
						removeWaitingProcess(wpid, ld, WRITE);
					}
				}
				else {
					prev = lptr->lqtail;
					while (q[prev].qprev != wpid) {
						prev = q[prev].qprev;
						removeWaitingProcess(prev, ld, READ);
					}				
				}
			}
			else {
				prev = lptr->lqtail;
				while (q[prev].qprev != wpid) {
					prev = q[prev].qprev;
					removeWaitingProcess(prev, ld, READ);
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
