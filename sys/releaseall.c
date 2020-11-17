/* Simultaneously release all the locks */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

void removeWaitingProcess(int, int, int);
int isWriterProcessPresentInWaiting(int);
void releaseLocksandAssignNextProc(int, int);
void writerAcquireHandler(int, int);

int releaseall(int numlocks, int locks,...) {
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
		if (isbadlockid(lockid))
            return SYSERR;

		if (locktab[lockid].procs_hold_list[currpid] == ACQUIRED)
			releaseLocksandAssignNextProc(currpid, lockid);
		else
			return SYSERR;
	}
	resched();
    
	restore(ps);
	return OK;	
}

void removeWaitingProcess(int pid, int lockid, int type) {
    dequeue(pid);
    // proc side
	struct pentry *nptr = &proctab[pid];
	nptr->waittype = BADTYPE;
	nptr->waitlockid = BADPID;
	nptr->wait_time_start = 0;
    // lock side
	struct lentry *lptr = &locktab[lockid];
 	lptr->ltype = type;
	nptr->locks_hold_list[lockid] = ACQUIRED;
	lptr->procs_hold_list[pid] = ACQUIRED;
    
	ready(pid, RESCHNO);
}

int isWriterProcessPresentInWaiting(int lockid) {
    // check if there is a writer process in waiting queue
    int procid = lastid(locktab[lockid].lqtail);
    while (procid != locktab[lockid].lqhead) {
		if (proctab[procid].waittype == WRITE)
			return procid;
	    procid = q[procid].qprev;
	}
    return -1;
}

void writerAcquireHandler(int lockid, int writer_pid) {
    // writer claiming
    struct lentry *lptr = &locktab[lockid];
    int procid = lastid(lptr->lqtail);
	while (procid != writer_pid) {
	    removeWaitingProcess(procid, lockid, READ);
		procid = q[procid].qprev;
	}
}

void releaseLocksandAssignNextProc(int pid, int lockid) {
    /*
    change lock_types in proctab
    change proc_types in locktab
    assign another process from its waiting queue
    if none is there, then lock is LFREE
    */
	struct lentry *lptr = &locktab[lockid];
	struct pentry *pptr = &proctab[pid];
    // release the lock
	lptr->ltype = DELETED;
	pptr->waittype = BADTYPE;
	pptr->waitlockid = BADPID;
	lptr->procs_hold_list[pid] = UNACQUIRED;
	pptr->locks_hold_list[lockid] = UNACQUIRED;

    /*
    --assign the next proc acc to policy
    get the maximum priority proc in the waiting queue
    get the waiting priority of that proc
    loop from tail to head till qkey != maxwaitprio or id == head
    find if multiple procs are there with the same prio
        if not, then lock that proc
    if yes, then find the procid with the max waiting time (within 1000)
    */
    
	int maxprio = MININT;
    // if the waiting queue is not empty
	if (nonempty(lptr->lqhead)) {
		int procid = lastid(lptr->lqtail);
		maxprio = lastkey(lptr->lqtail);
        // checking for write process in waiting
        int writerProcExist = isWriterProcessPresentInWaiting(lockid);
		if (!isbadpid(writerProcExist)) {
            // if writer present - valid pid
			procid = lastid(lptr->lqtail);
            // if it's the last one
            if (q[writerProcExist].qkey == maxprio) {
				unsigned long time_diff = 0;
                if (proctab[procid].wait_time_start >= proctab[writerProcExist].wait_time_start)
                    time_diff = proctab[procid].wait_time_start - proctab[writerProcExist].wait_time_start;
                else
                    time_diff = proctab[writerProcExist].wait_time_start - proctab[procid].wait_time_start;
				if (time_diff < 1000) {
                    // time difference is less than 1sec                
		            int flag = 0;
                    int i = 0;
					for (; i < NPROC; i++) {
						if (lptr->procs_hold_list[i] == ACQUIRED) {
							flag = 1;
							break;
						}
					}
					if (flag == 0)
						removeWaitingProcess(writerProcExist, lockid, WRITE);
				}
				else
					writerAcquireHandler(lockid, writerProcExist);				
			}
			else
				writerAcquireHandler(lockid, writerProcExist);
		}
		else {         
            // if writer absent -- invalid pid
			procid = lastid(lptr->lqtail);
			while (!isbadpid(procid) && procid != lptr->lqhead) {
				removeWaitingProcess(procid, lockid, READ);
				procid = q[procid].qprev;
			}
		}
	}
    
    // updating the values after changes
	lptr->lprio = getMaxPrioWaitingProcs(lockid);
	maxprio = getMaxAcquiredProcPrio(pid);
    pptr->pinh = (maxprio > pptr->pprio) ? maxprio : 0;
}
