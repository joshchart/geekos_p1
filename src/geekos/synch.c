/*
 * Synchronization primitives
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003,2013,2014 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2016 Neil Spring <nspring@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 *
 * $Revision: 1.13 $
 * 
 */

#include <geekos/kthread.h>
#include <geekos/int.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/synch.h>
#include <geekos/smp.h>

extern void Schedule_And_Unlock(Spin_Lock_t * unlock_me);

/* the following is a reimplementation of mutexes for smp (no interrupt disabling) */
void Mutex_Init(struct Mutex *mutex) {
    mutex->state = MUTEX_UNLOCKED;
    Spin_Lock_Init(&mutex->guard);
    mutex->owner = 0;
    Clear_Thread_Queue(&mutex->waitQueue);
    Spin_Lock_Init(&mutex->waitQueue.lock);     /* ns15 */
}

void Mutex_Lock(struct Mutex *mutex) {
    int was_held;
    bool iflag = Spin_Lock_Irq_Save(&mutex->guard);
    /* unnecessary to xchg given guard lock: predates guard; left alone. */
    __asm__ __volatile__("movl %2, %0\n\t"
                         "xchg %0, %1\n\t":"=a"(was_held),
                         "=m"(mutex->state)
                         :"i"(MUTEX_LOCKED));
    if(was_held == MUTEX_LOCKED) {
        Add_To_Back_Of_Thread_Queue(&mutex->waitQueue, CURRENT_THREAD);
        /* Store iflag so Schedule_And_Unlock can restore it */
        mutex->guard.iflag = iflag;
        Schedule_And_Unlock(&mutex->guard);
    } else {
        Spin_Unlock_Irq_Restore(&mutex->guard, iflag);
    }
    mutex->owner = get_current_thread(0);
}
void Mutex_Lock_Interrupts_Disabled(struct Mutex *mutex) {
    Mutex_Lock(mutex);
}
void Mutex_Unlock_Interrupts_Disabled(struct Mutex *mutex) {
    Mutex_Unlock(mutex);
}

static void Mutex_Unlock_With_Guard_Held(struct Mutex *mutex) {
    if(!Is_Thread_Queue_Empty(&mutex->waitQueue)) {
        Wake_Up_One(&mutex->waitQueue);
    } else {
        mutex->state = MUTEX_UNLOCKED;
    }
}

void Mutex_Unlock(struct Mutex *mutex) {
    bool iflag = Spin_Lock_Irq_Save(&mutex->guard);
    Mutex_Unlock_With_Guard_Held(mutex);
    Spin_Unlock_Irq_Restore(&mutex->guard, iflag);
}

/* for when the mutex covers a thread queue and you're
   inserting your own thread onto that queue */
void Mutex_Unlock_And_Schedule(struct Mutex *mutex) {
    /* Spin_Lock_Irq_Save returns iflag; store it for Schedule_And_Unlock */
    mutex->guard.iflag = Spin_Lock_Irq_Save(&mutex->guard);
    Mutex_Unlock_With_Guard_Held(mutex);
    Schedule_And_Unlock(&mutex->guard);
}

void Cond_Init(struct Condition *cond) {
    Clear_Thread_Queue(&cond->waitQueue);
    Spin_Lock_Init(&cond->waitQueue.lock);
}

void Cond_Wait(struct Condition *cond, struct Mutex *mutex) {
    /* Spin_Lock_Irq_Save returns iflag; store it for Schedule_And_Unlock */
    mutex->guard.iflag = Spin_Lock_Irq_Save(&mutex->guard);
    Add_To_Back_Of_Thread_Queue(&cond->waitQueue, CURRENT_THREAD);
    Mutex_Unlock_With_Guard_Held(mutex);
    /* release the guard only after this thread is fully on the wait queue */
    Schedule_And_Unlock(&mutex->guard);
    Mutex_Lock(mutex);
}

void Cond_Signal(struct Condition *cond) {
    bool iflag = Save_And_Disable_Interrupts();
    Wake_Up_One(&cond->waitQueue);
    Restore_Interrupt_State(iflag);
}

void Cond_Broadcast(struct Condition *cond) {
    bool iflag = Save_And_Disable_Interrupts();
    Wake_Up(&cond->waitQueue);
    Restore_Interrupt_State(iflag);
}
