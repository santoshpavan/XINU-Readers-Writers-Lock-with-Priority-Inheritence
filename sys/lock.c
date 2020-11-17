/* Acquisition of a lock for read or write */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int getMaxPrioWaitingProcs(int);
void cascadingRampUpPriorities(int);
int getNewProcPrio(int);
void priorityInheritence(int, int);
void processWaitForLock(int, int, int, int);
void claimUnusedLock(int, int, int);
int getMaxAcquiredProcPrio(int);

int lock(int lockid, int type, int priority) {
    /*
    sanity checks
    if this is the first one, then take it
    if already occupied check read or write
    if read 
      and this is read, and highest prio write in queue of this lock <= this prio
      else wait
    if write has it then wait
    */
	STATWORD ps;
	disable(ps);
	
	struct lentry *lptr = &locktab[lockid];
	struct pentry *pptr = &proctab[currpid];        
	
    if (isbadlockid(lockid) || lptr->lstate == LFREE) {
		restore(ps);
		return(SYSERR);
	}

    // DELETED implies available space
	if (lptr->ltype == DELETED)
		claimUnusedLock(lockid, type, currpid);
    else if (lptr->ltype == READ) {
		if (type == READ) {
            // collecting all the readers from tail till write
			int procid = firstid(lptr->lqhead);
			while (procid != lptr->lqtail) {
				struct pentry *wptr = &proctab[procid];
				if (wptr->waittype == WRITE && q[procid].qkey > priority) {
    				processWaitForLock(lockid, type, priority, currpid);
    				restore(ps);
    				return pptr->lockreturn;
				}
                procid = q[procid].qnext;
			}
            claimUnusedLock(lockid, type, currpid);
            // changing the priority
            lptr->lprio = getMaxPrioWaitingProcs(lockid);
            // cascading the changed priority
			cascadingRampUpPriorities(lockid);
        	restore(ps);
        	return OK;
		}
        else {
            // if write then cannot claim
			processWaitForLock(lockid, type, priority, currpid);
			restore(ps);
			return pptr->lockreturn;
		}
	}
	else {
        // if write then cannot claim
		processWaitForLock(lockid, type, priority, currpid);
		restore(ps);
		return pptr->lockreturn;
	}
}

int getMaxPrioWaitingProcs(int lockid) {
	struct lentry *lptr = &locktab[lockid];
	int maxprio = MININT;
	int procid = firstid(lptr->lqhead);
    while (procid != lptr->lqtail) {
		int pprio = getNewProcPrio(procid); 
		if (maxprio < pprio)
			maxprio = pprio;
		procid = q[procid].qnext;
	}
	return maxprio;				
}

void cascadingRampUpPriorities(int lockid) {
    // for cascading inheritence
	struct lentry *lptr = &locktab[lockid];
	int i = 0;
	for (; i < NPROC; i++) {
		if (lptr->procs_hold_list[i] == ACQUIRED) {
			struct pentry *pptr = &proctab[i];
			int maxprio = getMaxAcquiredProcPrio(i);
			if (maxprio > pptr->pprio)
				pptr->pinh = maxprio;
            // maxprio is <= than original priority of pptr process
			else
				pptr->pinh = 0;
            // continue recursion if applicable
			if (!isbadlockid(pptr->waitlockid))
				cascadingRampUpPriorities(pptr->waitlockid);
		}
	}
}

int getMaxAcquiredProcPrio(int pid) {
	struct pentry *pptr = &proctab[pid];
	int maxprio = -1;
    int i = 0;
	for (; i < NLOCKS; i++) {
		if (pptr->locks_hold_list[i] == ACQUIRED) {
			struct lentry *lptr = &locktab[i];
			if (maxprio < lptr->lprio)
				maxprio = lptr->lprio;
		}
	}
	return maxprio;
}

int getNewProcPrio(int pid) {
    return (proctab[pid].pinh == 0) ? proctab[pid].pprio : proctab[pid].pinh;
}

void priorityInheritence(int lockid, int priority) {
  	struct lentry *lptr = &locktab[lockid];    
	int i = 0;
	for (; i < NPROC; i++) {
		if (lptr->procs_hold_list[i] == ACQUIRED) {
			struct pentry *pptr = &proctab[i];
			if (getNewProcPrio(i) < priority) {
				pptr->pinh = priority;
                int waitlockid = pptr->waitlockid;
                // cascade the inheritence if required
				if (!isbadlockid(waitlockid))
					cascadingRampUpPriorities(waitlockid);
			}
		}
	}
}

void processWaitForLock(int lockid, int type, int priority, int pid) {
    struct pentry *pptr = &proctab[pid];
	pptr->waittype = type;
	pptr->pstate = PRWAIT;
	pptr->waitlockid = lockid;
	pptr->wait_time_start = ctr1000;
	insert(currpid, locktab[lockid].lqhead, priority);
	locktab[lockid].lprio = getMaxPrioWaitingProcs(lockid);
    // priority inheritence
    priorityInheritence(lockid, getNewProcPrio(pid));
	pptr->lockreturn = OK;
	resched();
}

void claimUnusedLock(int lockid, int type, int pid) {
    //proctab side
    struct pentry *pptr = &proctab[pid];
	pptr->waittype = BADTYPE;
	pptr->waitlockid = BADPID;
	pptr->locks_hold_list[lockid] = ACQUIRED;
	pptr->wait_time_start = 0;
    //locktab side    
    struct lentry *lptr = &locktab[lockid];
    lptr->ltype = type;
    lptr->lprio = -1;
	lptr->procs_hold_list[currpid] = ACQUIRED;
}
