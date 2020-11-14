/* Simultaneously release all the locks */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

int releaseall (int numlocks, int locks,...) {
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
    
    // changing the priority to the max of waiting processes across all locks held by this proc
    pptr->pinh = getAllMaxWaitingPrio(currpid);
    
    // releasing the locks held by proc
    while (i < numlocks) {
        struct lentry *lptr = &locktab[i];
        lockid = (int *)(&locks) + i;
        
        if (isbadlock(lockid) || lptr->proc_types[currpid] == LNONE)
            return SYSERR;

        releaseLock(lockid, currpid);
        //returns SYSERR when still being used eg: multiple Readers
        assignNextProctoLock(lockid);
        i++;
    }
    
    restore(ps);
    return OK;
}

// to release specific lockid
void releaseLock(int lockid, int pid) {
    /*
    change lock_types in proctab to LNONE
    change proc_types in locktab to LNONE
    assign another process from its waiting queue
    if none is there, then lock is LFREE
    */
    struct lentry *lptr = &locktab[lockid];
    if (lptr->ltype == LREAD) {
        lptr->nreaders--;
        if (lptr->nreaders == 0) {
            lptr->lstate = LFREE;
            lptr->ltype = LNONE;
        }
    }
    else {
        // write is single owned
        lptr->lstate = LFREE;
        lptr->ltype = LNONE;
   }

    lptr->proc_types[pid] = LNONE;
    proctab[pid].lock_types[lockid] = LNONE;
}

int assignNextProctoLock(int lockid) {
    struct lenty *lptr = &locktab[lockid];
    // only free locks should call this - sanity check
    // state changes in the above function
    if (lptr->lstate == LUSED)
        return SYSERR;
        
    /*
    get the maximum priority proc in the waiting queue
    get the waiting priority of that proc
    loop from tail to head till qkey != maxwaitprio or id == head
    find if multiple procs are there with the same prio
        if not, then lock that proc
    if yes, then find the procid with the max waiting time (within 1000)
    */
    int lastprocid = getlast(lptr->ltail);
    int maxwaitprio = q[waitprocid].qkey;
    int waitprocid = lastprocid;
    int nextprocid = -1;
    int count = 0;
    
    while (waitprocid != lptr->lhead && q[waitprocid] == maxwaitprio) {
        count++;
        waitprocid = q[waitprocid].qprev;
    }
    if (count > 1) {
        /*
        find the longest waiting time and its procid
        traverse again to check if there are timediff < 1000
        if yes, check if anyone of them is a writer and take that
        */
        
        // there are multiple procs with same prio
        waitprocid = lastprocid;
        unsigned long timenow = ctr1000;
        unsigned long max = -1;
        int i = 0;
        // finding the longest waiting time
        for (;i < count; i++, waitprocid = q[waitprocid].qprev) {
            unsigned long waittimediff = timenow - proctab[waitprocid].wait_time_start;
            if (max < waittimediff) {
                if (waittimediff - max >= 1000) {
                    max = waittimediff;
                    nextprocid = waitprocid;
                }
            }
        }
        // checking for timediff < 1000 and if there is a writer
        int temp = nextprocid;
        waitprocid = lastprocid;
        for (i = 0; i < count; i++, waitprocid = q[waitprocid].qprev) {
            unsigned long waittimediff = timenow - proctab[waitprocid].wait_time_start;
            if (max - waittimediff < 1000 && waitprocid != temp) {
                // writer is given priority
                if (proctab[waitprocid].waittype == LWRITE) {
                    nextprocid = waitprocid;
                    break;
                }
            }
        }
    }
    else {
        // there is only one
        nextprocid = waitprocid;
    }
    
    // cannot call lock here as it operates on currpid
    // in lock.c
    claimUnusedLock(lockid, proctab[nextprocid].waittype, nextprocid);
    dequeue(nextprocid);
    // if reader then claim other readers too
    if (locktab[lockid].type == LREAD) {
        //int highest_writer_prio = getHighestWritePriority(ldes);
        assignOtherWaitingReaders(lockid);
    }
}

int getAllMaxWaitingPrio(int pid) {
    struct pentry *pptr = &proctab[pid];
    int max = MININT;
    int i = 0;
    for (; i < NLOCKS; i++) {
        if (pptr->lock_types[i] != LNONE && max < locktab[i].lprio) {
            max = locktab[i].lprio;
        }
    }
    
    return max;
}
