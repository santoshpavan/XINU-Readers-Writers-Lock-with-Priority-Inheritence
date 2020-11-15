/* creates a lock descriptor */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>
#include <lock.h>

LOCAL int newlock();

int lcreate () {
    STATWORD ps;    
	int	lock = newlock();

	disable(ps);
	
    if ( lock == SYSERR ) {
		restore(ps);
		return(SYSERR);
	}
	
    restore(ps);
	return(lock);
}

LOCAL int newlock()
{
	int	lock;
	int	i;
    
	for (i = 0 ;i < NLOCKS ;i++) {
		lock = nextlock--;
		if (nextlock < 0)
			nextlock = NLOCKS - 1;
		if (semaph[lock].sstate == LFREE) {
			semaph[lock].sstate = LUSED;
			return(lock);
		}
	}
    
	return(SYSERR);
}
