#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef
#define NLOCKS 50
#endif

#ifndef
#define NPROC 50
#endif

#define	LFREE	0
#define	LUSED	1
#define LREAD   1
#define LWRITE  2
/* 
LNONE means no mapping for that process and lock
LNONE => neither LREAD or LWRITE
*/
#define LNONE   0

/*
#define NOT_WAITING  0
#define WAITING      1
#define ACQUIRED     2
*/

struct	lentry	{
    int  lstate;    /* the state LFREE or LUSED */
	int	  lqhead;	 /* q index of head of list	*/
	int	  lqtail;    /* q index of tail of list	*/
    int   ltype;     /* LREAD or LWRITE */
    int  proc_types[NPROC]; /* LREAD or LWRITE for assigned locks */
    int   nreaders;  /* number of readers */
    int   lprio; /* max proc prio of all the waiting procs */
};
struct	lentry	locktab[];
int	nextlock;

#define	isbadlock(lock_id)	(lock_id < 0 || lock_id >= NLOCKS)

int lcreate (void);
int ldelete (int);
int lock (int, int , int);
int releaseall (int, int,...);

// clkinit.c
extern unsigned long ctr1000;

// lock.c
void assignOtherWaitingReaders(int, int);
void claimLock(int, int, int);
void prioInheritence(int, int);
// releaseall.c
void releaseLock(int, int);
int getAllMaxWaitingPrio(int);

#endif
