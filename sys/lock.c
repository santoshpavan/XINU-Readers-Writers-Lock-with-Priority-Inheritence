#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int getMaxPriorityInLockWQ(int);
void rampUpProcPriority(int);
int getProcessPriority(int);
void priorityInheritence(int, int);
void processWaitForLock(int, int, int, int);
void claimUnusedLock(int, int, int, int);

int lock(int ldes1, int type, int priority) {
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
	
	struct lentry *lptr = &rw_locks[ldes1];
	struct pentry *pptr = &proctab[currpid];        
	
    if (isbadlock(ldes1) || lptr->lstate == LFREE) {
		restore(ps);
		return(SYSERR);
	}

    // DELETED implies available space
	if (lptr->ltype == DELETED) {
		claimUnusedLock(ldes1, type, priority, currpid);
	}
	else if (lptr->ltype == READ) {
		if (type == WRITE) {
			processWaitForLock(ldes1, type, priority, currpid);
			restore(ps);
			return pptr->plockret;
		}
		else if (type == READ) {
            // collecting all the readers from tail till write
			int next = firstid(lptr->lqhead);
			while (next != lptr->lqtail) {
				struct pentry *wptr = &proctab[next];
				if (wptr->wait_ltype == WRITE && q[next].qkey > priority) {
    				processWaitForLock(ldes1, type, priority, currpid);
    				restore(ps);
    				return pptr->plockret;
				}
                next = q[next].qnext;
			}
            claimUnusedLock(ldes1, type, priority, currpid);
            // changing the priority
            lptr->lprio = getMaxPriorityInLockWQ(ldes1);
            // cascading the changed priority
			rampUpProcPriority (ldes1);
		}	
	}

	else if (lptr->ltype == WRITE) {
		processWaitForLock(ldes1, type, priority, currpid);
		restore(ps);
		return pptr->plockret;
	}
	
	restore(ps);
	return(OK); 
}

int getMaxPriorityInLockWQ(int ld) {
	struct lentry *lptr = &rw_locks[ld];
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

void rampUpProcPriority(int ld) {
    // for cascading inheritence
	struct lentry *lptr = &rw_locks[ld];
	int i = 0;
	for (; i < NPROC; i++) {
		if (lptr->lproc_list[i] == 1) {
			struct pentry *pptr = &proctab[i];
			int maxprio = getMaxWaitProcPrioForPI(i);
			if (maxprio > pptr->pprio)
				pptr->pinh = maxprio;
			else
				pptr->pinh = 0; /* as maxprio is either equal or less than original priority of pptr process */
            int waitlockid = pptr->wait_lockid;
			if (!isbadlock(waitlockid))
				rampUpProcPriority (waitlockid);
		}
	}
}

int getMaxWaitProcPrioForPI(int pid) {
	struct pentry *pptr = &proctab[pid];
	int maxprio = -1;
    int i = 0;
	for (; i < NLOCKS; i++) {
		if (pptr->bm_locks[i] == 1)  {
			struct lentry *lptr = &rw_locks[i];
			if (maxprio < lptr->lprio)
				maxprio = lptr->lprio;
		}		
	}	
	return maxprio;
}

int getProcessPriority(int pid) {
    return (proctab[pid].pinh == 0) ? proctab[pid].pprio : proctab[pid].pinh;
}

void priorityInheritence(int ld, int priority) {
  	struct lentry *lptr = &rw_locks[ld];    
	int i = 0;
	for (; i < NPROC; i++) {
		if (lptr->lproc_list[i] == 1) {
			struct pentry *pptr = &proctab[i];
			if (getProcessPriority(i) < priority) {
				pptr->pinh = priority;
                int waitlockid = pptr->wait_lockid;
                // cascade the inheritence if required
				if (!isbadlock(waitlockid))
					rampUpProcPriority (waitlockid);
			}
		}
	}
}

void processWaitForLock(int ldes1, int type, int priority, int pid) {
    struct pentry *pptr = &proctab[pid];
    struct lentry *lptr = &rw_locks[ldes1];
	pptr->pstate = PRWAIT;
	pptr->wait_lockid = ldes1;
	pptr->wait_time = ctr1000;
	pptr->wait_pprio = priority;
	pptr->wait_ltype = type;
	insert(currpid, lptr->lqhead, priority);
	lptr->lprio = getMaxPriorityInLockWQ(ldes1);
    // priority inheritence
    priorityInheritence(ldes1, getProcessPriority(pid));
	pptr->plockret = OK;
	resched();
}

void claimUnusedLock(int ldes1, int type, int priority, int pid) {
    struct pentry *pptr = &proctab[pid];
    struct lentry *lptr = &rw_locks[ldes1];
    lptr->ltype = type;
    lptr->lprio = -1;
	lptr->lproc_list[currpid] = 1;
	pptr->bm_locks[ldes1] = 1;
	pptr->wait_lockid = -1;
    pptr->wait_pprio = priority;
	pptr->wait_ltype = -1;
}
