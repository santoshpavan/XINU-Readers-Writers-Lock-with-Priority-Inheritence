/* Delete the lock descriptor passed */

int ldelete (int lock) {
    STATWORD ps;    
	int	pid;
	struct	lentry	*lptr;

	disable(ps);
	if (isbadsem(lock) || locks[lock].lstate == LFREE) {
		restore(ps);
		return(SYSERR);
	}
	lptr = &locks[lock];
	lptr->lstate = LFREE;
	if ( nonempty(lptr->sqhead) ) {
		while( (pid = getfirst(lptr->lqhead)) != EMPTY)
		  {
		    proctab[pid].pwaitret = DELETED;
		    ready(pid, RESCHNO);
		  }
		resched();
	}
	restore(ps);
	return(OK);
}
