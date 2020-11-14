/* Acquisition of a lock for read or write */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

void processWaitForLock(int, int, int);
void processWaitForLock(int, int, int);
void prioInheritence(int , int);
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
    
    struct lentry *lptr = &locktab[ldes];
    
    if (lptr->lstate == LFREE || lptr->lstate == DELETED) {
        claimUnusedLock(ldes, type, currpid);
        // if no waiting queue
        if (getlast(lptr->ltail) == lhead) {
            restore(ps);
            return OK;
        }
        else if (type == LREAD) {
            // just to make sure. Mostly won't be used
            int highest_writer_prio = getHighestWritePriority(ldes);
            if (highes_write_prio <= priority) {
                // give lock to this proc
                lptr->proc_types[currpid] = type;
                proctab[currpid].lock_types[ldes] = type;
                lptr->nreaders++;
                // assign all readers with greater priority than highest writer priority
                assignOtherWaitingReaders(ldes, highest_writer_prio);
        }
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
    pptr->waittype = type;
    /* Priority Inheritence */
    // TODO: Not sure how to handle the multiple Read processes condition
    prioInheritence(lock_ind, priority);
    
    //TODO: use inherited; changing the lprio value if required
    //if (pptr->pprio > locktab[lock_ind].lprio)
    //    locktab[lock_ind].lprio = pptr->pprio;
    if (pptr->pihn > locktab[lock_ind].lprio)
        locktab[lock_ind].lprio = pptr->pihn;
    
    // insert in the queue based on waiting priority
    // TODO: do we need to insert based on pinh?
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
    //int lind = getlast(locktab[lock_ind].ltail);
    int pid = getlast(locktab[lock_ind].ltail);
    while (pid != head) {
        //if (locktab[lind].ltype == LWRITE && q[lind].qkey > max) {
        if (proctab[pid].waittype == LWRITE && q[pid].qkey > max) {
            max = q[pid].qkey;
            break;
        }
        pid = q[pid].qprev;
    }
    return max;
}

//void assignOtherWaitingReaders(int lock_ind, int write_prio) {
void assignOtherWaitingReaders(int lock_ind) {
    int pid = getlast(locktab[lock_ind].ltail);
    while (pid != head) {
        //if (locktab[lind].ltype == LWRITE)
        //    break;
        if (proctab[pid].waittype == LWRITE)
            break;
        else{
            // assign this lock to this reader
            struct lentry *lptr = &locktab[lock_ind];
            lptr->proc_types[pid] = type;
            proctab[pid].lock_types[lock_ind] = type;
            dequeue(pid);
            lptr->nreaders++;
        }
        pid = q[pid].qprev;
    }
}

void claimUnusedLock(int ldes, int type, int pid) {
    struct lentry *lptr = &locktab[ldes];
    lptr->lstate = LUSED;
    lptr->ltype = type;
    lptr->proc_types[pid] = type;
    proctab[pid].lock_types[ldes] = type;
    proctab[pid].wait_start_time = 0;
    proctab[pid].waittype = LNONE;
    if (type == LWRITE)
        lptr->nreaders = 0; //no readers when writer locked it
}
