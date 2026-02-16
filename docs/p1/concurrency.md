# Concurrency for Project 1: Fork and Pipes

This document covers the concurrency concepts you'll need for Project 1. Work through it before implementing pipes and reference counting.

**How to use this document:**
- Read the OSTEP chapters and examine the GeekOS code referenced
- Work through the questions with your AI tutor or on your own
- Fill in "Your Understanding" sections as you develop answers
- This becomes your reference during implementation

**Important**: These concepts are **prerequisites** for implementation. Your AI tutor will work through each section with you before writing code that depends on it. Don't skip ahead—understanding WHY patterns work is more valuable than just using them.

---

## Section Prerequisites

| Section | Required before implementing... |
|---------|--------------------------------|
| 1. Why Synchronization Matters | Any shared data structure |
| 2. Mutexes | Pipe structure |
| 3. Condition Variables | Pipe_Read, Pipe_Write (blocking) |
| 4. Producer-Consumer Pattern | Full pipe implementation |
| 5. Atomic Operations | Fork (file inheritance), Close (refcount) |

---

## 1. Why Synchronization Matters
> **Prerequisite for**: Understanding why any of the following patterns are necessary

GeekOS runs on **two CPUs** (SMP=2). This means two threads can execute kernel code *simultaneously* on different processors.

### Explore

**Read**: [OSTEP Chapter 28: Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf), section 28.1-28.3

**Examine**: Run this thought experiment:

```
CPU 0                              CPU 1
─────                              ─────
read refCount (value: 1)           read refCount (value: 1)
increment to 2                     increment to 2
write refCount = 2                 write refCount = 2

Result: refCount = 2, but TWO increments happened!
```

### Consider

1. If a `struct File` has `refCount = 1`, and two processes call `Fork()` simultaneously (on different CPUs), each trying to increment the refCount, what could go wrong?

2. Why doesn't "being careful" or "doing it quickly" solve this problem?

### Your Understanding

```
Why we need synchronization for shared data:



```

---

## 2. Mutexes: Blocking Locks
> **Prerequisite for**: Pipe structure design, any `Mutex_Lock`/`Mutex_Unlock` usage

For Project 1, you'll use **mutexes** to protect pipe state.

### Explore

**Read**: [OSTEP Chapter 28: Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf), sections 28.4-28.5

**Examine**: `include/geekos/synch.h` - find `struct Mutex` and its operations

**Examine**: How `Mutex_Lock()` and `Mutex_Unlock()` are used in existing GeekOS code:
```bash
grep -n "Mutex_Lock\|Mutex_Unlock" src/geekos/*.c
```

### Consider

1. What does `Mutex_Lock()` do if another thread already holds the mutex?

2. What happens if you forget to call `Mutex_Unlock()`?

3. Looking at the mutex structure, how does it know which thread owns it?

### Your Understanding

```
What a mutex provides:


When to use a mutex:


```

---

## 3. Condition Variables: Waiting for Things to Happen
> **Prerequisite for**: Pipe_Read, Pipe_Write (any blocking/waiting logic)

Pipes need threads to **wait** (block) when:
- A reader finds an empty buffer (wait for data)
- A writer finds a full buffer (wait for space)

Condition variables let threads sleep until something changes.

### Explore

**Read**: [OSTEP Chapter 30: Condition Variables](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf) - this entire chapter is essential

**Examine**: `include/geekos/synch.h` - find `struct Condition` and its operations

### Consider

1. Why must you hold a mutex when calling `Cond_Wait()`?

2. What happens to the mutex *during* `Cond_Wait()`? (Hint: if the mutex stayed locked, could another thread ever signal?)

3. When `Cond_Wait()` returns, what do you know about:
   - The mutex?
   - The condition you were waiting for?

### The Critical Question: `while` vs `if`

Study these two patterns:

```c
// Pattern A
Mutex_Lock(&mutex);
if (buffer_is_empty()) {
    Cond_Wait(&dataAvailable, &mutex);
}
// use data from buffer
Mutex_Unlock(&mutex);

// Pattern B
Mutex_Lock(&mutex);
while (buffer_is_empty()) {
    Cond_Wait(&dataAvailable, &mutex);
}
// use data from buffer
Mutex_Unlock(&mutex);
```

**Consider**:
- In Pattern A, after `Cond_Wait()` returns, is the buffer *guaranteed* to have data? What if another thread ran first and consumed it?
- Why does Pattern B re-check the condition?

**Read**: OSTEP Chapter 30, section on "Mesa semantics" vs "Hoare semantics"

### Signal vs Broadcast

```c
Cond_Signal(&cond);     // Wake ONE waiting thread
Cond_Broadcast(&cond);  // Wake ALL waiting threads
```

**Consider**: If three threads are waiting for data, and you write one byte, should you use Signal or Broadcast?

### Your Understanding

```
The condition variable contract:
- Before Cond_Wait:
- During Cond_Wait:
- After Cond_Wait:

Why while() not if():


When to use Signal vs Broadcast:


```

---

## 4. The Producer-Consumer Pattern
> **Prerequisite for**: Pipe structure design, EOF detection, broken pipe detection

Pipes ARE the producer-consumer problem. Writers produce data; readers consume it.

### Explore

**Read**: [OSTEP Chapter 30](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf), producer-consumer examples

**Examine**: Think about what state a pipe needs:
- A buffer to hold data
- Track how much data is in the buffer
- Track read/write positions
- Know when to block readers (buffer empty)
- Know when to block writers (buffer full)
- Know when there are no more writers (EOF)
- Know when there are no more readers (broken pipe)

### Design Exercise

Before looking at any implementation, sketch the structure of a pipe:

```c
struct Pipe {
    // What fields would you include?





};
```

**Consider**:
1. How many mutexes does a pipe need?
2. How many condition variables? What does each represent?
3. How do you track whether any readers/writers still exist?

### Your Understanding

```
Pipe structure design:


Why these condition variables:


How to detect EOF (no more writers):


How to detect broken pipe (no more readers):


```

---

## 5. Reference Counting with Atomic Operations
> **Prerequisite for**: Fork (file descriptor inheritance), Close (proper cleanup)

When `Fork()` creates a child process, parent and child share open files. Both hold references to the same `struct File`. We need to track how many references exist so we only free the file when the *last* reference closes.

### The Problem

```c
// In Fork(): child inherits parent's open file
childContext->file_descriptor_table[i] = parentFile;
parentFile->refCount++;  // DANGER: not atomic!
```

If two processes fork simultaneously, both incrementing the same refCount, the race condition from Section 1 can occur.

### Explore

**Read**: [OSTEP Chapter 28](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf), section 28.4 on hardware support

**Examine**: `include/geekos/atomic.h` - find `Atomic_Increment()` and `Atomic_Decrement()`

### How Atomic Operations Work

GeekOS uses GCC's `__atomic` builtins, which compile to efficient hardware instructions on any supported architecture.

On x86, the processor provides the `LOCK` prefix, which makes the next instruction atomic across all CPUs. Combined with `XADD` (exchange-and-add), we get atomic increment/decrement:

```asm
; Atomic decrement: atomically subtract 1 and return new value
lock xaddl %eax, (%edx)   ; atomically: old = *ptr; *ptr += eax; eax = old
```

When you write:
```c
return __atomic_sub_fetch(&refCount, 1, __ATOMIC_SEQ_CST);  // Atomic decrement
```

The compiler generates:
```asm
movl    $-1, %eax           ; eax = -1
lock xaddl %eax, (%edx)     ; atomic exchange-and-add
subl    $1, %eax            ; adjust to get new value (xadd returns OLD)
```

The `lock` prefix ensures no other CPU can access that memory location between reading and writing.

### Memory Ordering

The `__ATOMIC_SEQ_CST` parameter specifies **sequential consistency** - the strongest memory ordering guarantee. This ensures:
- All threads see operations in a consistent global order
- No reordering of operations around atomic accesses
- This is the safest choice and matches what most programmers expect

Other memory ordering options exist (`__ATOMIC_ACQUIRE`, `__ATOMIC_RELEASE`, `__ATOMIC_RELAXED`) for advanced use cases where performance is critical, but sequential consistency is recommended for educational purposes.

### Consider

1. Why is `lock xaddl` sufficient for SMP safety, when a simple `incl` would not be?

2. In `Close()`, why do we check `if (Atomic_Decrement(&file->refCount) == 0)` before freeing?

3. What would happen if we used a non-atomic decrement and two threads called `Close()` simultaneously on a file with refCount=2?

### Your Understanding

```
What makes an operation "atomic":


The reference counting pattern:
- On Fork (inherit file):
- On Close:
- When to actually free:


```

---

## 6. Putting It Together: Pipe Implementation Sketch

Before implementing, trace through these scenarios:

### Scenario 1: Reader blocks, then writer adds data

```
Time    Reader (CPU 0)              Writer (CPU 1)
────    ──────────────              ───────────────
 1      Mutex_Lock
 2      buffer empty, Cond_Wait
 3      (releases mutex, sleeps)
 4                                  Mutex_Lock
 5                                  add data to buffer
 6                                  Cond_Signal(dataAvailable)
 7                                  Mutex_Unlock
 8      (wakes up, re-acquires mutex)
 9      buffer not empty, read data
10      Mutex_Unlock
```

### Scenario 2: Writer closes, reader gets EOF

Trace what happens when:
1. Writer calls `Close()` on write end of pipe
2. Reader is blocked in `Cond_Wait()` for data
3. How does reader learn there will never be more data?

### Scenario 3: Reader closes, writer gets EPIPE

Trace what happens when:
1. Reader calls `Close()` on read end of pipe
2. Writer tries to write data
3. How does writer learn no one will read the data?

### Your Trace

```
Scenario 2 trace:




Scenario 3 trace:




```

---

## 7. Quick Reference (fill in as you learn)

| Situation | What to Use | Pattern |
|-----------|-------------|---------|
| Protect pipe buffer | | |
| Reader waits for data | | |
| Writer waits for space | | |
| Track file references | | |
| Increment refCount in Fork | | |
| Decrement refCount in Close | | |

### Your Rules

```
1. Before using a shared resource:

2. When waiting for a condition:

3. When a condition might have changed:

4. When sharing a file between processes:

```

---

## After Completing Project 1

After submitting, you'll receive a supplement with:
- Recommended patterns for the concepts above
- Common mistakes and how to avoid them
- Discussion of any differences from your approach

Compare it with your notes here to reinforce your understanding.
