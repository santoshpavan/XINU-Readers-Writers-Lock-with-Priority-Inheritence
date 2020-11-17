#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef NLOCKS
#define	NLOCKS		50	/* number of maximum locks */
#endif

#define	LFREE	0		/* this lock is free		*/
#define	LUSED	1		/* this lock is used		*/

#define READ     0
#define WRITE    1

#define ACQUIRED  1
#define UNACQUIRED  0

struct	lentry	{
	int	    lstate;		/* LFREE or LUSED */
	int	    lqhead;
	int	    lqtail;
	int	    ltype;		/* READ or WRITE */
	int	    lprio;		/* max proc priority in waiting queue */
	int 	procs_hold_list[NPROC]; /* procs holding this lock */
};
struct lentry rw_locks[NLOCKS];
int nextlock;

// lock.c
extern void claimUnusedLock(int , int , int , int );
extern int getProcessPriority(int);
extern void cascadingRampUpPriorities(int);
extern int getMaxPrioWaitingProcs(int);
extern int getMaxAcquiredProcPrio(int);

extern unsigned long ctr1000;

#define	isbadlockid(lockid)	(lockid < 0 || lockid >= NLOCKS)

#endif
