/*
 * Atomic operations using GCC __atomic builtins.
 *
 * These provide lock-free atomicity for simple increment/decrement operations,
 * useful for reference counting and other concurrent data access.
 *
 * Memory ordering notes:
 * - __ATOMIC_SEQ_CST: Sequential consistency (strongest, safest, easiest to reason about)
 * - __ATOMIC_ACQUIRE: For lock acquisition - ensures subsequent reads see recent values
 * - __ATOMIC_RELEASE: For lock release - ensures previous writes are visible
 * - __ATOMIC_RELAXED: No ordering guarantees (fastest, use with caution)
 *
 * For educational purposes in GeekOS, we use __ATOMIC_SEQ_CST for general operations
 * as it's the easiest to reason about and matches programmer intuition.
 */

#ifndef GEEKOS_ATOMIC_H
#define GEEKOS_ATOMIC_H

#include <geekos/kassert.h>

/**
 * Atomically increment an integer and return the NEW value.
 * Lock-free on all modern x86 processors.
 *
 * Generated x86 assembly (approximately):
 *   lock xaddl $1, (%rdi)   ; atomic fetch-and-add
 *   addl $1, %eax           ; adjust to get new value
 */
static __inline__ int Atomic_Increment(int *value) {
    return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST);
}

/**
 * Atomically decrement an integer and return the NEW value.
 * Asserts that the value was positive before decrement.
 * Lock-free on all modern x86 processors.
 *
 * Typical usage for reference counting:
 *   if (Atomic_Decrement(&obj->refCount) == 0) {
 *       // Last reference - safe to free
 *       Free(obj);
 *   }
 *
 * Generated x86 assembly (approximately):
 *   movl    $-1, %eax
 *   lock xaddl %eax, (%rdi)    ; Atomic exchange-and-add
 *   subl    $1, %eax           ; Adjust to get new value
 */
static __inline__ int Atomic_Decrement(int *value) {
    int old_value = __atomic_fetch_sub(value, 1, __ATOMIC_SEQ_CST);
    KASSERT(old_value > 0);  /* Was positive before decrement */
    return old_value - 1;     /* Return new value */
}

/**
 * Atomically read a value with acquire semantics.
 * Ensures subsequent reads see values at least as recent as this load.
 *
 * Use this when reading a value that might be concurrently modified
 * by another CPU/thread.
 */
static __inline__ int Atomic_Load(int *value) {
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

/**
 * Atomically write a value with release semantics.
 * Ensures previous writes are visible before this store completes.
 *
 * Use this when writing a value that might be concurrently read
 * by another CPU/thread.
 */
static __inline__ void Atomic_Store(int *value, int new_value) {
    __atomic_store_n(value, new_value, __ATOMIC_RELEASE);
}

/**
 * Atomically compare and swap.
 * If *value == expected, sets *value = desired and returns true.
 * Otherwise, sets *expected = *value and returns false.
 *
 * This is the fundamental building block for lock-free algorithms.
 */
static __inline__ bool Atomic_Compare_And_Swap(int *value, int *expected, int desired) {
    return __atomic_compare_exchange_n(value, expected, desired,
                                       false,  /* not weak */
                                       __ATOMIC_SEQ_CST,
                                       __ATOMIC_SEQ_CST);
}

#endif /* GEEKOS_ATOMIC_H */
