/*
 * GeekOS interrupt handling data structures and functions
 * Copyright (c) 2001, David H. Hovemeyer <daveho@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 *
 * $Revision: 1.12 $
 * 
 */

/*
 * This module describes the C interface which must be implemented
 * by interrupt handlers, and has the initialization function
 * for the interrupt system as a whole.
 */

#ifndef GEEKOS_INT_H
#define GEEKOS_INT_H

#include <geekos/kassert.h>
#include <geekos/ktypes.h>
#include <geekos/defs.h>
#include <geekos/lock.h>

/*
 * This struct reflects the contents of the stack when
 * a C interrupt handler function is called.
 * It must be kept up to date with the code in "lowlevel.asm".
 */
struct Interrupt_State {
    /*
     * The register contents at the time of the exception.
     * We save these explicitly.
     */
    uint_t gs;
    uint_t fs;
    uint_t es;
    uint_t ds;
    uint_t ebp;
    uint_t edi;
    uint_t esi;
    uint_t edx;
    uint_t ecx;
    uint_t ebx;
    uint_t eax;

    /*
     * We explicitly push the interrupt number.
     * This makes it easy for the handler function to determine
     * which interrupt occurred.
     */
    uint_t intNum;

    /*
     * This may be pushed by the processor; if not, we push
     * a dummy error code, so the stack layout is the same
     * for every type of interrupt.
     */
    uint_t errorCode;

    /* These are always pushed on the stack by the processor. */
    uint_t eip;
    uint_t cs;
    uint_t eflags;
};

/*
 * An interrupt that occurred in user mode.
 * If Is_User_Interrupt(state) returns true, then the
 * Interrupt_State object may be cast to this kind of struct.
 */
struct User_Interrupt_State {
    struct Interrupt_State state;
    uint_t espUser;
    uint_t ssUser;
};


#ifdef GEEKOS                   /* stuff above may be used by user code. not obviously useful. */

static __inline__ bool Is_User_Interrupt(struct Interrupt_State *state) {
    return (state->cs & 3) == USER_PRIVILEGE;
}

/*
 * The interrupt flag bit in the eflags register.
 * FIXME: should be in something like "cpu.h".
 */
#define EFLAGS_IF (1 << 9)

/*
 * The signature of an interrupt handler.
 */
typedef void (*Interrupt_Handler) (struct Interrupt_State * state);

/*
 * Perform all low- and high-level initialization of the
 * interrupt system.
 */
void Init_Interrupts(int secondaryCPU);

/*
 * Query whether or not interrupts are currently enabled.
 */
bool Interrupts_Enabled(void);

extern Spin_Lock_t intLock;

extern void lockKernel();
extern void unlockKernel();
extern bool Kernel_Is_Locked(); /* it is locked, perhaps by another thread or inherited */
extern bool I_Locked_The_Kernel();      /* it is locked, definitely by me */

/*
 * Block interrupts on this CPU only.
 * Does NOT acquire any locks - use spinlocks for SMP protection.
 */
static __inline__ void __Disable_Interrupts(void) {
    __asm__ __volatile__("cli");
}

#define Disable_Interrupts()		\
do {					\
    KASSERT(Interrupts_Enabled());	\
    __Disable_Interrupts();		\
} while (0)

/*
 * Unblock interrupts on this CPU.
 */
static __inline__ void __Enable_Interrupts(void) {
    __asm__ __volatile__("sti");
}

#define Enable_Interrupts()		\
do {					\
    KASSERT(!Interrupts_Enabled());	\
    __Enable_Interrupts();		\
} while (0)

/*
 * Dump interrupt state struct to screen
 */
void Dump_Interrupt_State(struct Interrupt_State *state);

/**
 * Save the current interrupt state and disable interrupts on THIS CPU only.
 * (Linux equivalent: local_irq_save)
 *
 * WARNING: This does NOT provide atomicity on SMP systems! Disabling interrupts
 * only prevents preemption on the current CPU - code on other CPUs can still
 * run concurrently.
 *
 * Use this when:
 * - Accessing per-CPU data that cannot be touched by other CPUs
 * - About to acquire a spinlock (interrupts must be disabled before spinlock)
 * - Protecting against interrupt handlers on this CPU only
 *
 * Do NOT use this for:
 * - Reference counting or other data shared across CPUs - use Atomic_Increment/
 *   Atomic_Decrement from <geekos/atomic.h> instead (lock-free, works on SMP)
 * - Protecting shared data structures - use a Mutex or spinlock instead
 *
 * IMPORTANT: Always capture the return value. Quick local check when reviewing:
 *   1. Return value is stored in a local variable (e.g., `bool iflag = ...`)
 *   2. That variable is either:
 *      (a) passed to Restore_Interrupt_State(iflag) on all paths, OR
 *      (b) stored to memory (e.g., lock->iflag) for later restoration
 * Case (b) requires non-local analysis to verify correctness - not expected in a quick scan.
 * Failing to eventually restore interrupts will leave the CPU in a bad state.
 *
 * @return true if interrupts were enabled (caller MUST pass to Restore_Interrupt_State)
 */
static __inline__ bool Save_And_Disable_Interrupts(void) {
    bool interrupts_were_enabled = Interrupts_Enabled();
    if(interrupts_were_enabled)
        __Disable_Interrupts();
    return interrupts_were_enabled;
}

/**
 * Restore the interrupt state saved by Save_And_Disable_Interrupts.
 * (Linux equivalent: local_irq_restore)
 *
 * @param interrupts_were_enabled the value returned from Save_And_Disable_Interrupts()
 */
static __inline__ void Restore_Interrupt_State(bool interrupts_were_enabled) {
    KASSERT(!Interrupts_Enabled());
    if(interrupts_were_enabled) {
        __Enable_Interrupts();
    }
}

#endif /* GEEKOS */
#endif /* GEEKOS_INT_H */
