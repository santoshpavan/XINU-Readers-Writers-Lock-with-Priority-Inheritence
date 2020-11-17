/* Semaphores and current Locks comparision */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

#ifndef DEFAULT_LOCK_PRIO
#define DEFAULT_LOCK_PRIO 20
#endif

/*
first give to low prio
then give to highes prio
then give to medium
use writers to block each other
*/

void semwriter(char msg, int sem) {
    kprintf ("  %c: to acquire sem\n", msg);
    wait(sem);
    kprintf ("  %c: acquired sem, sleep 1s\n", msg);
	sleep (5);
    kprintf ("  %c: to release sem\n", msg);
    signal(sem);	
}

void test_sems() {
    int sem  = screate(1);
    
	int wr1 = create(semwriter, 2000, 10, "writer1", 2, 'A', sem);
	int wr2 = create(semwriter, 2000, 20, "writer2", 2, 'B', sem);
	int wr3 = create(semwriter, 2000, 30, "writer3", 2, 'C', sem);
    
    kprintf("-start writer 3, then sleep 1s. Sem granted to Writer A (prio 10)\n");
    resume(wr1);
    sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));

    kprintf("-start writer 3, then sleep 1s. Writer C (prio 30) blocked on the lock\n");
    resume(wr3);
    sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));
    
    kprintf("-start writer 3, then sleep 1s. Writer B (prio 20) blocked on the lock\n");
    resume(wr2);
    sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));
}

void lockwriter (char msg, int lck)
{
	kprintf ("  %c: to acquire lock\n", msg);
    lock (lck, WRITE, DEFAULT_LOCK_PRIO);
    kprintf ("  %c: acquired lock, sleep 1s\n", msg);
    sleep (5);
    kprintf ("  %c: to release lock\n", msg);
    releaseall (1, lck);
}

void test_locks() {
    int lck  = lcreate();
    
	int wr1 = create(lockwriter, 2000, 10, "writer1", 2, 'A', lck);
	int wr2 = create(lockwriter, 2000, 20, "writer2", 2, 'B', lck);
	int wr3 = create(lockwriter, 2000, 30, "writer3", 2, 'C', lck);
    
    kprintf("-start writer 3, then sleep 1s. Lock granted to Writer A (prio 10)\n");
    resume(wr1);
    sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));

    kprintf("-start writer 3, then sleep 1s. Writer C (prio 30) blocked on the lock\n");
    resume(wr3);
    sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));
    
    kprintf("-start writer 2, then sleep 1s. Writer B (prio 20) blocked on the lock\n");
    resume(wr2);
    sleep (1);
	kprintf("Priority of writer:%d\n", getprio(wr1));
}

void task1() {
    kprintf("\nTASK1:\nTesting Semphores...\n");
    test_sems();
    kprintf("\nTesting current Locks...\n");
    test_locks();
}
