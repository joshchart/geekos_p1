# Project 1: Fork and Exec

**Consult the submit server for deadline date and time**

You will implement the Fork() and Exec() operations, preserving the semantics of Pipe() across both: that both parent and child have the same file descriptors attached to the pipe, both parent and child may read and write to the pipe, and the exec'd program inherits the file descriptors of the process.

The primary goal of this assignment is to develop an understanding of the process control block (`struct Kernel_Thread`) and user context structures (`struct User_Context`).

## 1 Desired Semantics

In general, Fork() here should work as described in the man page for fork (fork(2)), with the following exceptions and specifics. GeekOS lacks many of the features that "real" fork() must make decisions about, such as signal inheritance, what to do about multiple threads, and resource limits.

- Fork() must return the child's pid in the parent and zero in the child. The Child's pid must be new and sequential. (Do not worry about overflow/wrap.)
- Global variables must start out the same, but each process updates its own copy.
- The stack should appear the same at the point of the Fork call, but each process has its own copy.
- File descriptions are shared: when either process updates the position in a file, that should update the position as seen by the other process.
- Fork() should return ENOMEM if a memory allocation fails, which it will if fork is called enough times.

### 1.1 Repository

The projects.h file includes flags that cause different TODO() macros in the distribution to fire.

### 1.2 Process Creation

The existing scheme for creating new processes is the Spawn() function. Spawn() is a bit like the Windows CreateProcess function, which creates a new process with a program name to implement. It does everything, which means it contains the core of both Fork() and Exec(). However, Spawn lacks a facility for creating the intermediate state: copying the address space from parent to child. It also lacks the functionality we seek with inheriting file descriptors. Finally, Spawn() creates the new process with an empty context rather than at the point of a fork.

Each process is represented by a "struct Kernel_Thread" typically named "kthread" as a local variable. There are kernel processes, which are really threads inside the kernel, since they all share the same address space. User processes have a "userContext" pointer set in the kthread. The userContext contains the stuff unique to user processes: their own address spaces, file descriptors, etc.

To Fork(), a new kthread must be created. Unlike in Spawn(), the contents of memory should not be the result of loading a program file, it should be a copy of the memory of the parent. In a real operating system with paging, this copy would occur lazily (and efficiently) via copy-on-write.

### 1.3 Copying the address space

Look in userseg.c to note how a user context can be created. The kernel has memcpy(). This one is pretty easy.

### 1.4 Inheriting file descriptors

Copy the file descriptor array from parent to child, then iterate over this list incrementing the reference count of each file. Most file descriptors will reference NULL.

### 1.5 Register Context

When a function makes a system call, the user process's registers are pushed onto the kernel's per-process stack. The kernel's stack is in the kthread "stackPage", while the stack pointer is esp. When cloning a process, the child should have substantially the same kernel stack, to represent substantially the same registers (one will be different).

**File Descriptor Table Diagram:**

After `pipe(&r, &w)` returns r=3, w=4:

```
struct User_Context (parent)     struct User_Context (child) [after fork]
[0][1][2][3][4][5]               [0][1][2][3][4][5]
         |   |                            |   |
         v   v                            v   v
    +------------+                   +------------+
    | struct File|                   | struct File|
    | ops=readPipeOps               | ops=writePipeOps
    | refCount=2 |                   | refCount=2 |
    | fsData     |                   | fsData     |
    +-----+------+                   +-----+------+
          |                                |
          v                                v
       +----------------------------------+
       |          struct Pipe             |
       | readers=1  |  writers=1          |
       | pointer to data buffer           |
       +----------------------------------+
                     |
                     v
              [buffered data]
```

There are two struct File objects for the pipe. The pipe object must track whether there are any readers or writers and keep buffered data, the File object must track how many processes (kthreads) hold a reference to the File.

### 1.6 Synchronization

With Fork(), particularly with SMP, comes the possibility that two of your processes may be running at the same time. If not careful about preventing concurrent access to the same operations, you may create a race condition that will cause data corruption at random; alternately, you may cause a deadlock condition - no processes can make progress.

In this section, "process" and "kernel thread" are used interchangeably, since the code you're writing is in the kernel, running in a thread (they share an address space in the kernel), on behalf of a process.

#### 1.6.1 The scalpel: Atomic operations

For simple operations like reference counting, atomic operations provide lock-free synchronization with minimal overhead:

```c
#include <geekos/atomic.h>

// Increment reference count (returns new value)
int newCount = Atomic_Increment(&file->refCount);

// Decrement and check if last reference
if (Atomic_Decrement(&file->refCount) == 0) {
    // Last reference - safe to free
    Free(file);
}
```

Available operations in `atomic.h`:
- `Atomic_Increment(int *value)` - Add 1, return new value
- `Atomic_Decrement(int *value)` - Subtract 1, return new value (asserts value > 0)
- `Atomic_Load(int *value)` - Read with acquire semantics
- `Atomic_Store(int *value, int new_value)` - Write with release semantics
- `Atomic_Compare_And_Swap(int *value, int *expected, int desired)` - CAS operation

These compile to single x86 instructions with the `lock` prefix, providing atomicity without disabling interrupts or acquiring locks. **Use atomics when a single read-modify-write operation is sufficient** (e.g., reference counting).

#### 1.6.2 The rock: Spinlocks

For short critical sections involving multiple operations, use spinlocks with interrupt control:

```c
#include <geekos/subsystem_locks.h>

bool iflag = Spin_Lock_Irq_Save(&someLock);
// ... critical section (keep it SHORT!) ...
Spin_Unlock_Irq_Restore(&someLock, iflag);
```

This pattern:
1. Saves the current interrupt state
2. Disables interrupts on this CPU (prevents preemption)
3. Acquires the spinlock (prevents other CPUs from entering)
4. On unlock, releases the spinlock and restores interrupt state

Named subsystem locks are defined in `subsystem_locks.h` (e.g., `netLock`, `ideLock`). These make code self-documenting about what each critical section protects.

**Why disable interrupts?** If a thread holding a spinlock is interrupted and put back on the ready queue, any other thread trying to acquire that spinlock would spin forever—the interrupted thread can't run to release it. **Always disable interrupts before acquiring a spinlock.**

**Keep it short!** Any code waiting for a spinlock spins with interrupts disabled. Spinlocks are efficient only for brief critical sections. From Wikipedia:

> Because they avoid overhead from operating system process rescheduling or context switching, spinlocks are efficient if threads are only likely to be blocked for a short period. For this reason, spinlocks are often used inside operating system kernels. However, spinlocks become wasteful if held for longer durations...

**Note on lists:** Each list in GeekOS (see list.h) is guarded by a spinlock. Simple list operations automatically grab and release that lock. Some operations, such as "get next in list," expect the lock to be held. Consult the list.h source to determine which functions are appropriate.

#### 1.6.3 The knife: Mutexes

For longer critical sections or when you might need to wait, use mutexes:

```c
Mutex_Lock(&someMutex);
// ... critical section (can be longer, may block) ...
Mutex_Unlock(&someMutex);
```

Unlike spinlocks, **acquiring a mutex may take a while**. If the mutex is held by another thread, `Mutex_Lock` (see synch.c) puts your thread on a wait queue and **swaps it out**—the CPU runs other threads instead of spinning. This is the key distinction: spinlocks busy-wait, mutexes sleep.

Using a mutex requires that interrupts be enabled—the calling code must be prepared to be swapped out. Shared data structures should be in a consistent state before calling `Mutex_Lock`, since another thread will run.

**Condition variables:** Mutexes also support conditions, which allow a thread to wait until signaled by another thread. This is useful when you need to wait for something to change (e.g., data becoming available in a pipe):

```c
struct Mutex mutex;
struct Condition condition;

// Initialization (once, at setup time)
Mutex_Init(&mutex);
Cond_Init(&condition);

// Waiting thread:
Mutex_Lock(&mutex);
while (!condition_is_true) {
    Cond_Wait(&condition, &mutex);  // Releases mutex, sleeps, re-acquires on wake
}
// ... condition is now true, we hold the mutex ...
Mutex_Unlock(&mutex);
```

Another thread signals when conditions change:
```c
Mutex_Lock(&mutex);
// ... modify shared state ...
Cond_Signal(&condition);    // Wake ONE waiting thread
// or: Cond_Broadcast(&condition);  // Wake ALL waiting threads
Mutex_Unlock(&mutex);
```

See synch.c for implementation details and vfs.c for usage examples.

**Use mutexes for concurrency associated with shared resources** (such as a pipe) owned by processes that may be suspended, especially user processes.

#### 1.6.4 Choosing the right tool

| Tool | Use when... | Waiting behavior | Can use conditions? |
|------|-------------|------------------|---------------------|
| **Atomic** | Single read-modify-write (refcount) | No waiting | No |
| **Spinlock** | Multiple operations, very short | Busy-waits (CPU spins) | No |
| **Mutex** | Longer sections, may need to wait | Sleeps (CPU freed) | Yes |

### 1.7 Rules and Hints

Follow the path of Spawn() and determine which features are part of Exec() and which features are part of Fork(). Most everything you need is already present.

**Alter `struct File` to add a reference count.** Alter Open(), Fork(), and Close() to set, increment, and decrement this reference count.

You may modify any function needed, regardless of whether a `TODO_P(PROJECT_FORK)` macro is present. (You may need to modify other functions.)

Most changes will be in **pipe.c** and **syscall.c**, possibly in **user.c** or **userseg.c**.

## 2 Tests

There are a few public tests: **fork-p1**, **forkpipe**, and **forkexec**. They are intended to cover the bulk of the functionality described here.

Expect "secret" tests to exercise other behavior described in this handout. "Secret" tests are likely to cover bizarre mistakes such as failing to replicate the data segment and failing to count references for pipes.

There is a "forkbomb" test which may be passed by containing the bomb: either it continues to fork indefinitely or all processes exit and return to a working shell.
