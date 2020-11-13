/* Acquisition of a lock for read or write */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

int lock (int ldes, int type, int priority) {
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
    
    if (isbadlock(ldes)) {
        restore(ps);
        return SYSERR;
    }
    
    struct lentry lptr = &locktab[ldes];
    
    if (lptr->lstate == LFREE || lptr->lstate == DELETED) {
        lptr->lstate = LUSED;
        lptr->ltype = type;
        lptr->proc_status[currpid] = ACQUIRED;
        proctab[currpid].locks_status[ldes] = ACQUIRED;
        restore(ps);
        return OK;
    }
    // not free
    if (lptr->ltype == READ) {
        if (type == READ) 
            if (getHighestWritePriority(ldes) <= priority) {
                // give lock to this proc
                lptr->proc_types[currpid] = type;
                proctab[currpid].locks[ldes] = type;
            }
            else {
                // make it wait
                processWaitForLock(leds, type);
            }
        }
        else if (type == WRITE) {
            // wait the process
            processWaitForLock(ldes, type);
        }
    }
    else {
        // wait the process
        processWaitForLock(ldes, type);
    }
    
    restore(ps);
}

void processWaitForLock(int lock_ind, int type) {
    struct pentry *pptr = &proctab[currpid];
    pptr->pstate = PRWAIT;
    pptr->locks[lock_ind] = type;
    insert(currpid, locktab[lock_ind].lhead, locktab[lock_ind].ltail);
    pptr->wait_time_start = ctr1000;
    resched();
}

int getHighestWritePriority(int lock_ind) {
    int max = -1;
    int i = 0;
    int lind = getlast(locktab[lock_ind].ltail);
    while (lind != head) {
        if (locktab[lind].ltype == WRITE && q[lind].qkey > max) {
            max = q[lind].qkey;
        }
        lind = q[lind].qprev;
    }
    return max;
}
