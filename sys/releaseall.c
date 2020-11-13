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
    while (i < numlocks) {
        struct lentry *lptr = &locktab[i];
        lockid = (int *)(&locks) + i;
        
        if (isbadlock(lockid) || lptr->proc_types[currpid] == LNONE)
            return SYSERR;
        releaseLock(lockid, currpid);
        if (assignNextProctoLock() == SYSERR) {
            // might never actually happen. Sanity check
            return SYSERR;
        }
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
        lptr->lstate = LFREE;
        lptr->ltype = LNONE;
    }

    lptr->proc_types[pid] = LNONE;
    proctab[pid].lock_types[lockid] = LNONE;
}

int assignNextProctoLock(int lockid) {
    struct lenty *lptr = &locktab[lockid];
    // only free locks should call this
    if (lptr->lstate == LUSED)
        return SYSERR;
    
}
