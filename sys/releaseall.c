#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

void removeWaitingProcess(int, int, int);
int isWriterProcessPresentInWaiting(int);
void releaseLDForProc(int, int);

int releaseall (int numlocks, long locks,...) {
	/*
    release the mentioned locks held by this process
    if the lock is not of the proc -> SYSERR
    after release of each lock, assign the lock to another process in its queue
    */
    STATWORD ps;
    disable(ps);
    
	struct pentry *pptr = &proctab[currpid];
	int lockid;
    int i = 0;
	for(; i < numlocks; i++) {
        lockid = (int *)(&locks) + i;
		if (isbadlock(lockid))
            return SYSERR;
            
		if (rw_locks[lockid].lproc_list[currpid] == ACQUIRED)
			releaseLDForProc(currpid, lockid);
		else
			return SYSERR;
	}
	resched();

	restore(ps);
	return OK;	
}

void removeWaitingProcess(int pid, int lockid, int type) {
	struct pentry *nptr = &proctab[pid];
	struct lentry *lptr = &rw_locks[lockid];
    dequeue(pid);
	nptr->bm_locks[lockid] = ACQUIRED;
	lptr->lproc_list[pid] = ACQUIRED;
 	lptr->ltype = type;
	nptr->wait_time = 0;
	nptr->wait_lockid = -1;
	nptr->wait_ltype = -1;
	ready(pid, RESCHNO);
}

int isWriterProcessPresentInWaiting(int lockid) {
    // check if there is a writer process in waiting queue
    int prev = lastid(rw_locks[lockid].lqtail);
    while (prev != rw_locks[lockid].lqhead) {
		if (proctab[prev].wait_ltype == WRITE)
			return prev;
	    prev = q[prev].qprev;
	}
    return -1;
}

void releaseLDForProc(int pid, int lockid) {
	struct lentry *lptr = &rw_locks[lockid];
	struct pentry *pptr = &proctab[pid];
	int maxprio = -1;

	// set ltype deleted temporarily
	lptr->ltype = DELETED;
	lptr->lproc_list[pid] = UNACQUIRED;
	// release ld lock from bit mask
	pptr->bm_locks[lockid] = UNACQUIRED;
	pptr->wait_lockid = -1;
	pptr->wait_ltype = -1;

	if (nonempty(lptr->lqhead)) {
		int writerProcExist = 0;
		unsigned long tdf = 0;
		int prev = lastid(lptr->lqtail);
		maxprio = lastkey(lptr->lqtail);
		
        // checking for write process in waiting
        writerProcExist = isWriterProcessPresentInWaiting(lockid);
		if (writerProcExist == -1) {
			prev = lastid(lptr->lqtail);
			while (!isbadpid(prev) && prev != lptr->lqhead) {
				removeWaitingProcess(prev, lockid, READ);
				prev = q[prev].qprev;
			}
		}
		else {
			prev = lastid(lptr->lqtail);
            if (q[writerProcExist].qkey == maxprio) {
				tdf = proctab[prev].wait_time - proctab[writerProcExist].wait_time;
				if (tdf < 0)
					tdf = -tdf;
				if (tdf < 1000) {
		            int readerProcHoldingLock = 0;
                    int i = 0;
					for (; i < NPROC; i++) {
						if (lptr->lproc_list[i] == ACQUIRED) {
							readerProcHoldingLock = 1;
							break;
						}
					}
					if (readerProcHoldingLock == 0)
						removeWaitingProcess(writerProcExist, lockid, WRITE);
				}
				else {
					prev = lastid(lptr->lqtail);
					while (prev != writerProcExist) {
						removeWaitingProcess(prev, lockid, READ);
						prev = q[prev].qprev;
					}				
				}
			}
			else {
				prev = lastid(lptr->lqtail);
				while (prev != writerProcExist) {
					removeWaitingProcess(prev, lockid, READ);
					prev = q[prev].qprev;
				}
			}
		}
	}

	lptr->lprio = getMaxPrioWaitingProcs(lockid);
	maxprio = getMaxWaitingProcPrio(pid);
    pptr->pinh = (maxprio > pptr->pprio) ? maxprio : 0;
}
