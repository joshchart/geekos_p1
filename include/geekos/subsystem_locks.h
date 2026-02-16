/*
 * GeekOS Subsystem Locks
 *
 * This header provides a central reference for kernel subsystem locks.
 * Some locks are already separate (good for concurrency), while others
 * are still aliases for globalLock (the "big hammer").
 *
 * LOCK STATUS:
 * ------------
 * SEPARATE (already independent):
 *   - kthreadLock  - Thread/process management
 *   - alarmLock    - Alarm/timer management
 *   - pidLock      - PID allocation
 *   - printLock    - Screen output
 *   - intLock      - Interrupt handling
 *
 * ALIASES (currently use globalLock):
 *   - kernelLock   - Generic kernel-wide locking
 *   - ideLock      - IDE disk driver
 *   - floppyLock   - Floppy disk driver
 *   - dmaLock      - DMA controller
 *   - netLock      - Networking subsystem
 *
 * FUTURE WORK FOR STUDENTS:
 * -------------------------
 * The alias locks are candidates for splitting into separate locks.
 * Each could potentially be independent if:
 *   1. The protected data structures are not shared with other subsystems
 *   2. No lock ordering issues would arise (potential deadlocks)
 *   3. Interrupt handlers don't need to acquire multiple subsystem locks
 *
 * TWO LOCKING PATTERNS:
 * ---------------------
 * 1. ACQUIRE pattern (most code): Enter without lock, acquire for critical section
 *      bool iflag = Spin_Lock_Irq_Save(&ideLock);
 *      // ... critical section ...
 *      Spin_Unlock_Irq_Restore(&ideLock, iflag);
 *
 * 2. RELEASE-FOR-BLOCKING pattern (syscalls): Enter with lock, release for blocking
 *      // Entered with lock held, interrupts disabled (from trap handler)
 *      Spin_Unlock_Irq_Enable(&netLock);
 *      // ... blocking operation (e.g., network I/O) ...
 *      Spin_Lock_Irq_Disable(&netLock);
 *      // Return with lock held, interrupts disabled
 */

#ifndef GEEKOS_SUBSYSTEM_LOCKS_H
#define GEEKOS_SUBSYSTEM_LOCKS_H

#include <geekos/lock.h>

/*
 * The global kernel lock - the "big hammer" still used by some subsystems.
 */
extern Spin_Lock_t globalLock;

/*
 * Already-separate subsystem locks (defined elsewhere, externed here for reference)
 */
extern Spin_Lock_t kthreadLock;   /* smp.c - thread/process management */
extern Spin_Lock_t alarmLock;     /* alarm.c - alarm/timer management */
extern Spin_Lock_t intLock;       /* int.c - interrupt handling */
/* pidLock is static in kthread.c */
/* printLock is static in screen.c */

/*
 * Subsystem lock aliases - currently point to globalLock.
 * Using named locks makes the code self-documenting about what
 * each critical section is intended to protect.
 */

/* For code that legitimately needs kernel-wide mutual exclusion */
#define kernelLock   globalLock

/* IDE disk driver - protects IDE controller state and pending operations */
#define ideLock      globalLock

/* Floppy disk driver - protects floppy controller state */
#define floppyLock   globalLock

/* DMA controller - protects DMA channel state */
#define dmaLock      globalLock

/* Networking subsystem - protects network buffers and connection state */
#define netLock      globalLock

/*
 * Helper functions for the RELEASE-FOR-BLOCKING pattern.
 *
 * Used by syscall handlers that enter with the lock held (from trap handler)
 * and need to release it to allow blocking operations (e.g., waiting for
 * network or disk I/O). The lock must be re-acquired before returning.
 *
 * Contract:
 *   - Spin_Unlock_Irq_Enable: Call when holding the lock with interrupts disabled.
 *                             Releases lock and enables interrupts.
 *   - Spin_Lock_Irq_Disable:  Call when NOT holding the lock with interrupts enabled.
 *                             Disables interrupts and acquires lock.
 *
 * Example (syscall handler):
 *   // Entered with lock held, interrupts disabled
 *   Spin_Unlock_Irq_Enable(&netLock);
 *   rc = blocking_network_operation();
 *   Spin_Lock_Irq_Disable(&netLock);
 *   // Return with lock held, interrupts disabled
 */

/*
 * Release lock and enable interrupts - for releasing lock before blocking.
 * Precondition: lock is held, interrupts are disabled.
 * Postcondition: lock is released, interrupts are enabled.
 */
static inline void Spin_Unlock_Irq_Enable(Spin_Lock_t *lock) {
    Spin_Unlock(lock);
    __asm__ __volatile__("sti");  /* Enable interrupts */
}

/*
 * Disable interrupts and acquire lock - for re-acquiring lock after blocking.
 * Precondition: lock is not held, interrupts are enabled.
 * Postcondition: lock is held, interrupts are disabled.
 */
static inline void Spin_Lock_Irq_Disable(Spin_Lock_t *lock) {
    __asm__ __volatile__("cli");  /* Disable interrupts */
    Spin_Lock(lock);
}

#endif /* GEEKOS_SUBSYSTEM_LOCKS_H */
