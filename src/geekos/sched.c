#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/screen.h>
#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/symbol.h>
#include <geekos/string.h>
#include <geekos/kthread.h>
#include <geekos/malloc.h>
#include <geekos/user.h>
#include <geekos/alarm.h>
#include <geekos/projects.h>
#include <geekos/smp.h>
#include <geekos/synch.h>

/* The lock associated with the run queue(s). */
static Spin_Lock_t run_queue_spinlock;

/* The centralized run queue of all threads that are ready to run. */
static struct Thread_Queue s_runQueue;

static struct Kernel_Thread *Get_Next_Runnable_Locked(void);
static void Make_Runnable_Locked(struct Kernel_Thread *kthread);

enum Scheduler { RR = 0,        /* default */
    MLFQ = 1,
    MPWS,
};
static enum Scheduler s_scheduler = RR;

/*
 * Add given thread to the run queue, so that it may be
 * scheduled.  Must be called with interrupts disabled and
 * run_queue_spinlock held.
 */
static void Make_Runnable_Locked(struct Kernel_Thread *kthread) {
    KASSERT(Is_Locked(&run_queue_spinlock));
    Enqueue_Thread(&s_runQueue, kthread);
    TODO_P(PROJECT_SCHEDULING, "replace make runnable as needed");
}

void Make_Runnable(struct Kernel_Thread *kthread) {
    KASSERT(!Interrupts_Enabled());

    KASSERT0(kthread->inThread_Queue == NULL,
             "attempting to make runnable a thread that is in another list.");

    if(kthread->priority == PRIORITY_IDLE)
        return;                 /* idle handled oob ns14 */

    Spin_Lock(&run_queue_spinlock);

    Make_Runnable_Locked(kthread);

    Spin_Unlock(&run_queue_spinlock);
}

/*
 * Atomically make a thread runnable.
 */
void Make_Runnable_Atomic(struct Kernel_Thread *kthread) {
    bool iflag = Save_And_Disable_Interrupts();
    Make_Runnable(kthread);
    Restore_Interrupt_State(iflag);
}


/*
 * Find the best (highest priority) thread in given
 * thread queue.  Returns null if queue is empty.
 */
static __inline__ struct Kernel_Thread *Find_Best(struct Thread_Queue
                                                  *queue) {
    int cpuID;

    KASSERT(Is_Locked(&run_queue_spinlock));

    cpuID = Get_CPU_ID();

    /* Pick the highest priority thread */
    struct Kernel_Thread *kthread = queue->head, *best = 0;
    while (kthread != 0) {
        if(kthread->affinity == AFFINITY_ANY_CORE ||
           kthread->affinity == cpuID) {
            if(best == 0 || kthread->priority > best->priority)
                // if (kthread->alive) - must finish exiting if not alive.
                best = kthread;
        }
        kthread = Get_Next_In_Thread_Queue(kthread);
    }

    if(!best) {
        best = CPUs[cpuID].idleThread;
    }

    return best;
}

/*
 * Get the next runnable thread from the run queue.
 * This is the scheduler.
 */
struct Kernel_Thread *Get_Next_Runnable_Locked(void) {
    struct Kernel_Thread *best = 0;
    best = Find_Best(&s_runQueue);
    KASSERT(best != 0);
    if(best->priority != PRIORITY_IDLE) {       /* ns14 oob idle */
        Remove_Thread(&s_runQueue, best);
    }

    KASSERT(Is_Locked(&run_queue_spinlock));
    TODO_P(PROJECT_SCHEDULING, "fix Get_Next_Runnable");
    return best;
}

/*
 * Called by lowlevel.asm in handle_interrupt, with
 * interrupts disabled, but no locks held.
 */
struct Kernel_Thread *Get_Next_Runnable(void) {
    struct Kernel_Thread *ret;

    KASSERT(!Interrupts_Enabled());
    KASSERT0(!I_Locked_The_Kernel(),
             "kernel lock should not be held when scheduling");

    Spin_Lock(&run_queue_spinlock);

    /* Disable preemption while we hold the run queue lock */
    g_preemptionDisabled[Get_CPU_ID()] = true;

    ret = Get_Next_Runnable_Locked();

    Spin_Unlock(&run_queue_spinlock);

    g_preemptionDisabled[Get_CPU_ID()] = false;

    /* At least could be the idle thread */
    KASSERT(ret);

    /* Ensure the new thread has a valid kernel stack pointer */
    KASSERT(((unsigned long)(ret->esp - 1) & ~0xfff) ==
            ((unsigned long)ret->stackPage));

    return ret;
}


/* This helper function is meant to facilitate implementing PS */
int Is_Thread_On_Run_Queue(const struct Kernel_Thread *thread) {
    if(s_scheduler == RR) {
        return Is_Member_Of_Thread_Queue(&s_runQueue, thread);
    } else {
        KASSERT0(0,
                 "Is_Thread_On_Run_Queue unimplemented for non-RR schedulers\n");
    }
}
