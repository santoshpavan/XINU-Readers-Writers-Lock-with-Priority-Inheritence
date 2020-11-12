/* Initialize the Locks */
#include <loch.h>

void linit() {
    struct lentry *lptr;
    
    for (i=0 ; i< NLOCKS ; i++) {
		(lptr = &locks[i])->lstate = LFREE;
		lptr->lqtail = 1 + (lptr->lqhead = newqueue());
	}
}
