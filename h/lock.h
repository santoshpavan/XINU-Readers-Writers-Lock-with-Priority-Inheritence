##ifndef _LOCK_H_
#define _LOCK_H_

#define NLOCKS 50

int lcreate (void);
int ldelete (int);
int lock (int, int , int);
int releaseall (int, int,..., int);

#endif