/* Acquisition of a lock for read or write */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

void processWaitForLock(int, int, int);
int getHighestWritePriority(int);

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
        lptr->proc_types[currpid] = type;
        proctab[currpid].lock_types[ldes] = type;
        if (type == LWRITE)
            lptr->nreaders = 0; //no readers when writer locked it
        restore(ps);
        return OK;
    }
    // not free
    if (lptr->ltype == LREAD) {
        if (type == LREAD) {
            int highest_writer_prio = getHighestWritePriority(ldes);
            if (highes_write_prio <= priority) {
                // give lock to this proc
                lptr->proc_types[currpid] = type;
                proctab[currpid].lock_types[ldes] = type;
                lptr->nreaders++;
                // assign all readers with greater priority than highest writer priority
                assignOtherWaitingReaders(ldes, highest_writer_prio);
            }
            else {
                // make it wait
                processWaitForLock(leds, type, priority);
            }
        }
        else if (type == LWRITE) {
            // wait the process
            processWaitForLock(ldes, type, priority);
        }
    }
    else {
        // if lock is write wait the process
        processWaitForLock(ldes, type, priority);
    }
    
    restore(ps);
    //NOT SURE IF I SHOULD RETURN THIS!
    return (proctab[currpid].pwaitret);
}

void processWaitForLock(int lock_ind, int type, int priority) {
    struct pentry *pptr = &proctab[currpid];
    pptr->pstate = PRWAIT;
    pptr->waitlockid = lock_ind;
    
    /* Priority Inheritence */
    // TODO: Not sure how to handle the multiple Read processes condition
    prioInheritence(lock_ind, priority);
        
    //TODO: use inherited; changing the lprio value if required
    if (pptr->pprio > locktab[lock_ind].lprio)
        locktab[lock_ind].lprio = pptr->pprio;
    
    // insert in the queue based on waiting priority
    insert(currpid, locktab[lock_ind].lhead, locktab[lock_ind].ltail);
    pptr->wait_time_start = ctr1000;
    resched();
}

void prioInheritence(int lockind, int priority) {
    struct lentry *lptr = &locktab[lockind];
    int i = 0;
    for (; i < NPROC; i++) {
        if (lptr->proc_types[i] != LNONE) {
            //this proc owns this lock, now
            if (proctab[i].pinh < priority) {
                // inherit the prio
                proctab[i].pinh = priority;
                // transitive property - cascade the priority inheritence
                if (proctab[i].waitlockid > 0)
                    prioInheritence(proctab[i].waitlockid, proctab[i].pinh);
            }
        }
    }
}

int getHighestWritePriority(int lock_ind) {
    //#define   MININT          0x80000000
    int max = MININT;
    int lind = getlast(locktab[lock_ind].ltail);
    while (lind != head) {
        if (locktab[lind].ltype == LWRITE && q[lind].qkey > max) {
            max = q[lind].qkey;
            break;
        }
        lind = q[lind].qprev;
    }
    return max;
}

void assignOtherWaitingReaders(int lock_ind, int write_prio) {
    int lind = getlast(locktab[lock_ind].ltail);
    while (lind != head) {
        if (locktab[lind].ltype == LWRITE)
            break;
        else{
            // assign this lock to this reader
            lptr->proc_types[currpid] = type;
            proctab[currpid].lock_types[ldes] = type;
            lptr->nreaders++;
        }
        lind = q[lind].qprev;
    }
}
