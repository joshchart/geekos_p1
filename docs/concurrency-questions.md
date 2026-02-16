# Concurrency and Synchronization in GeekOS

This document guides exploration of concurrency and synchronization in GeekOS. Rather than providing answers, it raises questions for you to investigate through textbook reading, code examination, and discussion.

**How to use this document:**
- Work through sections relevant to your current project
- Read the referenced OSTEP chapters and examine the GeekOS code
- Discuss your understanding with your AI tutor or instructor
- Fill in the "Your Understanding" sections as you develop answers
- This becomes your personalized reference for the project

---

## 1. The SMP Context

GeekOS runs on a multiprocessor system with **two CPUs** (SMP=2).

### Question: What does disabling interrupts protect against?

**Explore:**
- Read: [OSTEP Chapter 28: Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf), especially section 28.4
- Examine: `src/geekos/int.c` - `Save_And_Disable_Interrupts()` and `Restore_Interrupt_State()`

**Consider:**
1. If CPU 0 disables interrupts, what happens on CPU 1?
2. Draw a scenario where two CPUs access shared data, even with interrupts disabled on one
3. What *does* interrupt disabling protect against?

**Your Understanding:**
```
[Fill in as you work through the material]


```

---

## 2. Synchronization Primitives

GeekOS provides several synchronization mechanisms. For each, investigate when and how to use it.

### 2.1 Spinlocks

**Explore:**
- Read: [OSTEP Chapter 28: Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf), sections 28.5-28.14
- Examine: `src/geekos/synch.c` - `Spin_Lock()` and `Spin_Unlock()`
- Examine: Uses of spinlocks in `src/geekos/kthread.c`

**Consider:**
1. What instruction does `Spin_Lock()` use? Why that instruction?
2. What happens if an interrupt fires while you hold a spinlock, and the interrupt handler tries to acquire the same spinlock?
3. What should you do before acquiring a spinlock to prevent the scenario in question 2?
4. Why should critical sections protected by spinlocks be short?

**Your Understanding:**
```
When to use spinlocks:

Pattern for safe spinlock usage:

Why interrupts matter:

```

### 2.2 Mutexes

**Explore:**
- Read: [OSTEP Chapter 28: Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf)
- Examine: `src/geekos/synch.c` - `Mutex_Lock()` and `Mutex_Unlock()`
- Compare: How does `Mutex_Lock()` differ from `Spin_Lock()`?

**Consider:**
1. What does a mutex do when the lock is already held? How does this differ from a spinlock?
2. Look at `Mutex_Lock()` - are interrupts enabled or disabled while holding a mutex?
3. Given your answer to question 2, what operations should you avoid while holding a mutex?

**Your Understanding:**
```
Mutex vs Spinlock - when to use each:

What you can/cannot do while holding a mutex:

```

### 2.3 Condition Variables

**Explore:**
- Read: [OSTEP Chapter 30: Condition Variables](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf)
- Examine: `src/geekos/synch.c` - `Cond_Wait()`, `Cond_Signal()`, `Cond_Broadcast()`
- Examine: `src/geekos/kthread.c` - `Wait()` and `Wake_Up()` (a simpler wait/wake pattern)

**Consider:**
1. Why must you hold a mutex when calling `Cond_Wait()`?
2. What happens to the mutex during `Cond_Wait()`?
3. When you wake up from `Cond_Wait()`, what do you know about:
   - The mutex?
   - The condition you were waiting for?
4. Why might the condition be false even after waking up? (Hint: "Mesa semantics")
5. Given question 4, should you use `if` or `while` to check the condition?
6. When is `Cond_Signal()` sufficient vs when do you need `Cond_Broadcast()`?

**Your Understanding:**
```
The contract for condition variables:

Why while() instead of if():

Signal vs Broadcast:

```

---

## 3. Key Principles

### 3.1 Preserving Interrupt State

**Explore:**
- Examine: Functions in `src/geekos/syscall.c` that use `Save_And_Disable_Interrupts()`
- Examine: What `Save_And_Disable_Interrupts()` returns and how `Restore_Interrupt_State()` uses it

**Consider:**
1. Why does `Save_And_Disable_Interrupts()` return a value?
2. What should you do on error paths (early returns)?
3. Why not just call `Enable_Interrupts()` at the end of every function?

**Your Understanding:**
```
Pattern for interrupt state preservation:

Why this matters:

```

### 3.2 Avoiding Deadlock

**Explore:**
- Read: [OSTEP Chapter 32: Concurrency Bugs](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-bugs.pdf), section 32.3
- Search for uses of multiple locks in GeekOS (e.g., `kthreadLock`, `run_queue_spinlock`)

**Consider:**
1. What are the four conditions required for deadlock?
2. If Thread A holds lock X and wants lock Y, and Thread B holds lock Y and wants lock X, what happens?
3. How does "lock ordering" prevent deadlock?
4. If you introduce a new lock that might be held with existing locks, what should you document?

**Your Understanding:**
```
The four conditions for deadlock:

Lock ordering strategy:

```

### 3.3 Sleeping with Locks Held

**Consider:**
1. What happens if a thread holding a spinlock goes to sleep?
2. What if the thread that would wake it up needs that spinlock?

**Your Understanding:**
```
Why you should never sleep while holding a spinlock:

```

---

## 4. Common Concurrency Bugs

### 4.1 Atomicity Violations (Check-Then-Act)

**Explore:**
- Read: [OSTEP Chapter 32: Concurrency Bugs](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-bugs.pdf), section 32.2

**Consider this code:**
```c
if (ptr != NULL) {
    use(ptr);
}
```

1. What could go wrong if another thread runs `ptr = NULL;` between the check and the use?
2. How would you fix this?
3. Can you find any check-then-act patterns in GeekOS? Are they protected?

**Your Understanding:**
```
The check-then-act problem:

How to fix it:

```

### 4.2 Order Violations

**Explore:**
- Read: [OSTEP Chapter 32: Concurrency Bugs](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-bugs.pdf), section 32.2

**Consider:**
1. If Thread 1 initializes data and Thread 2 uses it, what ensures Thread 2 doesn't run first?
2. How can condition variables enforce ordering?

**Your Understanding:**
```
How order violations occur:

How to enforce ordering:

```

---

## 5. Patterns to Develop

As you work through the project, develop patterns for:

### 5.1 Protecting Shared Data with a Spinlock

```c
// Develop your pattern here:


```

### 5.2 Producer-Consumer with Condition Variables

```c
// Develop your pattern here:


```

### 5.3 Safe Initialization Ordering

```c
// Develop your pattern here:


```

---

## 6. GeekOS-Specific Notes

### Built-in List Locking

**Explore:**
- Examine: `include/geekos/list.h` - `DEFINE_LIST` and `IMPLEMENT_LIST` macros
- Examine: How thread queues are manipulated in `src/geekos/sched.c`

**Consider:**
1. Do GeekOS list operations have built-in locking?
2. Do you still need to disable interrupts before using them?

**Your Understanding:**
```
List locking in GeekOS:

```

### Subsystem Locks

**Explore:**
- Read `include/geekos/subsystem_locks.h`

**Consider:**
1. What subsystem locks are defined?
2. Which are separate locks vs aliases for `globalLock`?
3. Why might named locks be better than a single global lock?

**Your Understanding:**
```
Subsystem locks I found:

Benefits of named locks:

```

---

## 7. Your Quick Reference

As you develop understanding, fill in this reference for your own use:

| Situation | Mechanism | Your Pattern |
|-----------|-----------|--------------|
| Per-CPU data only | | |
| Short critical section, shared data | | |
| Longer critical section (no blocking) | | |
| Wait for condition | | |

### Your Rules of Thumb

1. Before acquiring a spinlock:
2. While holding a spinlock:
3. Condition variable wait pattern:
4. Multiple locks:
5. Check-then-act:

---

## 8. Further Reading

- [OSTEP Chapter 28: Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf)
- [OSTEP Chapter 30: Condition Variables](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf)
- [OSTEP Chapter 32: Concurrency Bugs](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-bugs.pdf)
- [OSTEP Multi-CPU Scheduling](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-sched-multi.pdf)
