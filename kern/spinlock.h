#ifndef JOS_INC_SPINLOCK_H
#define JOS_INC_SPINLOCK_H

#include <inc/types.h>

// After you implemented ticket_spinlock,
// define USE_TICKET_SPIN_LOCK.
// LAB 4: Your code here.

//#define USE_TICKET_SPIN_LOCK

// Comment this to disable spinlock debugging
#define DEBUG_SPINLOCK

// Mutual exclusion lock.
struct spinlock {
#ifndef USE_TICKET_SPIN_LOCK
	unsigned locked;   // Is the lock held?
#else
	unsigned own;
	unsigned next;
#endif

#ifdef DEBUG_SPINLOCK
	// For debugging:
	char *name;        // Name of lock.
	struct Cpu *cpu;   // The CPU holding the lock.
	uintptr_t pcs[10]; // The call stack (an array of program counters)
	                   // that locked the lock.
#endif
};

void __spin_initlock(struct spinlock *lk, char *name);
void spin_lock(struct spinlock *lk);
void spin_unlock(struct spinlock *lk);

#define spin_initlock(lock)   __spin_initlock(lock, #lock)

extern struct spinlock kernel_lock;

static inline void
lock_kernel(void)
{
	spin_lock(&kernel_lock);
}

static inline void
unlock_kernel(void)
{
	spin_unlock(&kernel_lock);

	// Normally we wouldn't need to do this, but QEMU only runs
	// one CPU at a time and has a long time-slice.  Without the
	// pause, this CPU is likely to reacquire the lock before
	// another CPU has even been given a chance to acquire it.
	asm volatile("pause");
}

#endif
