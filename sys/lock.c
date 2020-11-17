#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int getMaxPrioWaitingProcs(int);
void cascadingRampUpPriorities(int);
int getProcessPriority(int);
void priorityInheritence(int, int);
void processWaitForLock(int, int, int, int);
void claimUnusedLock(int, int, int, int);
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
		claimUnusedLock(lockid, type, priority, currpid);
	
    else if (lptr->ltype == READ) {
		if (type == READ) {
            // collecting all the readers from tail till write
			int next = firstid(lptr->lqhead);
			while (next != lptr->lqtail) {
				struct pentry *wptr = &proctab[next];
				if (wptr->wait_ltype == WRITE && q[next].qkey > priority) {
    				processWaitForLock(lockid, type, priority, currpid);
    				restore(ps);
    				return pptr->plockret;
				}
                next = q[next].qnext;
			}
            claimUnusedLock(lockid, type, priority, currpid);
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
			return pptr->plockret;
		}
	}
	else {
        // if write then cannot claim
		processWaitForLock(lockid, type, priority, currpid);
		restore(ps);
		return pptr->plockret;
	}
}

int getMaxPrioWaitingProcs(int lockid) {
	struct lentry *lptr = &locktab[lockid];
	int maxprio = -1;
	int next = firstid(lptr->lqhead);
    while (next != lptr->lqtail) {
		struct pentry *pptr = &proctab[next];
		int pprio = getProcessPriority(next); 
		if (maxprio < pprio)
			maxprio = pprio;
		next = q[next].qnext;
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
			if (!isbadlockid(pptr->wait_lockid))
				cascadingRampUpPriorities(pptr->wait_lockid);
		}
	}
}

int getMaxAcquiredProcPrio(int pid) {
	struct pentry *pptr = &proctab[pid];
	int maxprio = -1;
    int i = 0;
	for (; i < NLOCKS; i++) {
		if (pptr->bm_locks[i] == ACQUIRED) {
			struct lentry *lptr = &locktab[i];
			if (maxprio < lptr->lprio)
				maxprio = lptr->lprio;
		}
	}
	return maxprio;
}

int getProcessPriority(int pid) {
    return (proctab[pid].pinh == 0) ? proctab[pid].pprio : proctab[pid].pinh;
}

void priorityInheritence(int lockid, int priority) {
  	struct lentry *lptr = &locktab[lockid];    
	int i = 0;
	for (; i < NPROC; i++) {
		if (lptr->procs_hold_list[i] == ACQUIRED) {
			struct pentry *pptr = &proctab[i];
			if (getProcessPriority(i) < priority) {
				pptr->pinh = priority;
                int waitlockid = pptr->wait_lockid;
                // cascade the inheritence if required
				if (!isbadlockid(waitlockid))
					cascadingRampUpPriorities(waitlockid);
			}
		}
	}
}

void processWaitForLock(int lockid, int type, int priority, int pid) {
    struct pentry *pptr = &proctab[pid];
    struct lentry *lptr = &locktab[lockid];
	pptr->pstate = PRWAIT;
	pptr->wait_lockid = lockid;
	pptr->wait_time = ctr1000;
	pptr->wait_pprio = priority;
	pptr->wait_ltype = type;
	insert(currpid, lptr->lqhead, priority);
	lptr->lprio = getMaxPrioWaitingProcs(lockid);
    // priority inheritence
    priorityInheritence(lockid, getProcessPriority(pid));
	pptr->plockret = OK;
	resched();
}

void claimUnusedLock(int lockid, int type, int priority, int pid) {
    struct pentry *pptr = &proctab[pid];
    struct lentry *lptr = &locktab[lockid];
    lptr->ltype = type;
    lptr->lprio = -1;
	lptr->procs_hold_list[currpid] = ACQUIRED;
	pptr->bm_locks[lockid] = ACQUIRED;
	pptr->wait_lockid = -1;
    pptr->wait_pprio = priority;
	pptr->wait_ltype = -1;
}
