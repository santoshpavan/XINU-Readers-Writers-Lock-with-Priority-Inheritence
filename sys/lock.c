#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int lock (int ldes1, int type, int priority) {
	STATWORD ps;
	struct lentry *lptr = &rw_locks[ldes1];
	struct pentry *pptr = &proctab[currpid];
	disable(ps);
	
	if (isbadlock(ldes1) || lptr->lstate == LFREE) {
		restore(ps);
		return(SYSERR);
	}

    /* ltype = DELETED means it has not been acquired by any process yet for Read/Write but is created as lstate = LUSED  */
	/* thus any process could get the lock */
	if (lptr->ltype == DELETED) {
		claimUnusedLock(ldes1, type, priority, currpid);
	}
	
	/* already some process has acquired READ lock on the descriptor  */
	else if (lptr->ltype == READ) {
		/* current process wants to acquire WRITE lock */
		if (type == WRITE) {
			processWaitForLock(ldes1, type, priority, currpid);
			restore(ps);
			return pptr->plockret;
		}		
				
		/* current process wants to acquire READ lock */
		else if (type == READ) {
			/* check whether any higher write priority process is there in lock's wait queue */
			int next = lptr->lqhead;
			struct pentry *wptr;
			
			while (q[next].qnext != lptr->lqtail) {
				next = q[next].qnext;
				wptr = &proctab[next];
				if (wptr->wait_ltype == WRITE && q[next].qkey > priority) {
                    /* block the current process*/
    				processWaitForLock(ldes1, type, priority, currpid);
    				restore(ps);
    				return pptr->plockret;
				}
			}
            claimUnusedLock(ldes1, type, priority, currpid);
            lptr->lprio = getMaxPriorityInLockWQ(ldes1); /* set lprio field to max priority process in wait queue of the lock */
			rampUpProcPriority (ldes1, -1);
		}	
	}

	/* already some process has acquired WRITE lock on the descriptor */
	else if (lptr->ltype == WRITE) {
		/* block the current process as WRITE lock is exclusive */
		processWaitForLock(ldes1, type, priority, currpid);
		restore(ps);
		return pptr->plockret;
	}
	
	restore(ps);
	return(OK); 
}

int getMaxPriorityInLockWQ (int ld) {
	struct lentry *lptr = &rw_locks[ld];

	int maxprio = -1;
	int gprio;

	int next = firstid(lptr->lqhead);
    while (next != lptr->lqtail) {
		struct pentry *pptr = &proctab[next];
		gprio = getProcessPriority(pptr); 
		if (maxprio < gprio)
			maxprio = gprio;
		next = q[next].qnext;
	}

	return maxprio;				
}

void rampUpProcPriority (int ld, int priority) {
	struct lentry *lptr = &rw_locks[ld];
	int gprio = -1;
	int maxprio = -1;
    
	int i = 0;
	for (; i < NPROC; i++) {
		if (lptr->lproc_list[i] == 1) {
			struct pentry *pptr = &proctab[i];

			/* priority will be -1 when called this func for priority inheritance but when called during lock func it will be */
			/* priority of process currently blocked in lock func */
			if (priority == -1) {
				maxprio = getMaxWaitProcPrioForPI(i);
				
				if (maxprio > pptr->pprio)
					pptr->pinh = maxprio;
				else
					pptr->pinh = 0; /* as maxprio is either equal or less than original priority of pptr process */
				
                int waitlockid = pptr->wait_lockid;
				if (!isbadlock(waitlockid))
					rampUpProcPriority (waitlockid,-1);
			}

			/* ramp up the priority of pptr upto priority passed */
			else if (getProcessPriority(pptr) < priority) {
				pptr->pinh = priority;
                int waitlockid = pptr->wait_lockid;
				if (!isbadlock(waitlockid))
					rampUpProcPriority (waitlockid,-1);
			}
		}
	}
}

/* get max of p_i waiting on any of the locks held by process pid */
int getMaxWaitProcPrioForPI (int pid) {
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

int getProcessPriority(struct pentry *pptr) {
    return (pptr->pinh == 0) ? pptr->pprio : pptr->pinh;
}

void processWaitForLock(int ldes1, int type, int priority, int pid) {
    /* block the current process as WRITE lock is exclusive */
    struct pentry *pptr = &proctab[pid];
    struct lentry *lptr = &rw_locks[ldes1];
    
	pptr->pstate = PRWAIT;
	pptr->wait_lockid = ldes1;   /* process waiting on this lock id*/ 
	pptr->wait_time = ctr1000;   /* waiting time start */	
	pptr->wait_pprio = priority; /* waiting priority for the lock queue */
	pptr->wait_ltype = type; 

	insert(currpid, lptr->lqhead, priority); /* insert the proc in wait queue of lock descriptor based on its wait priority */
	lptr->lprio = getMaxPriorityInLockWQ(ldes1); /* set lprio field to max priority process in wait queue of the lock */
	rampUpProcPriority(ldes1,getProcessPriority(pptr));

	pptr->plockret = OK;
	resched();
}

void claimUnusedLock(int ldes1, int type, int priority, int pid) {
    struct pentry *pptr = &proctab[pid];
    struct lentry *lptr = &rw_locks[ldes1];
    
    lptr->ltype = type;
    lptr->lprio = -1;
	lptr->lproc_list[currpid] = 1;

	pptr->bm_locks[ldes1] = 1; /* set bit mask corresponding to lock desc to 1 for the current process */
	pptr->wait_lockid = -1; /* as it acquires lock it will take value -1  */
    pptr->wait_pprio = priority; /* waiting priority for the lock queue */
	pptr->wait_ltype = -1; /* as the current process is not being blocked */
}

