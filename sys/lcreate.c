
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newlock();

int lcreate() {
	STATWORD ps;
	disable(ps);
    int	lockid = newlock();
	if (lockid == SYSERR) {
		restore(ps);
		return(SYSERR);
	}
	restore(ps);
	return(lockid);
}

LOCAL int newlock()
{
	int	i = 0;
	for (; i < NLOCKS; i++) {
		int lockid = nextlock--;
		if (lockid < 0)
			nextlock = NLOCKS - 1;
		if (locktab[lockid].lstate == LFREE) {
			locktab[lockid].lstate = LUSED;
			return(lockid);
		}
	}
	return(SYSERR);
}

