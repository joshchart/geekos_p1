/*
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003,2013,2014 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 */
#ifndef GEEKOS_LOCK_H
#define GEEKOS_LOCK_H

#include <geekos/ktypes.h>

typedef struct {
    int lock;
    struct Kernel_Thread *locker;
    void *lockRA;
    struct Kernel_Thread *lastLocker;
    bool iflag;  /* Saved interrupt state for Lock_List/Unlock_List */
} Spin_Lock_t;

#define SPIN_LOCK_INITIALIZER { 0, NULL, NULL, NULL, false }

extern void Spin_Lock_Init(Spin_Lock_t *);
extern int Try_Spin_Lock(Spin_Lock_t *);
extern void Spin_Lock(Spin_Lock_t *);
extern void Spin_Unlock(Spin_Lock_t *);
extern int Is_Locked(Spin_Lock_t *);

/*
 * Combined interrupt-disabling + spinlock variants.
 *
 * These combine the common pattern of:
 *   1. Disable interrupts (to prevent deadlock with interrupt handlers)
 *   2. Acquire spinlock (for mutual exclusion across CPUs)
 *
 * Using these functions prevents common errors like forgetting to
 * disable interrupts before acquiring a spinlock.
 *
 * Usage:
 *   bool iflag = Spin_Lock_Irq_Save(&lock);
 *   // ... critical section ...
 *   Spin_Unlock_Irq_Restore(&lock, iflag);
 */

/* Forward declaration - actual inline definition requires int.h */
extern bool Spin_Lock_Irq_Save(Spin_Lock_t *lock);
extern void Spin_Unlock_Irq_Restore(Spin_Lock_t *lock, bool iflag);

#endif // GEEKOS_LOCK_H
