# Component: Blocking Pipe (Concurrent-Safe Pipes)

**Prerequisites**: Reference counting complete, Fork implemented (creates the concurrency)
**Builds toward**: Full process model with proper inter-process communication

**How to use this guide**:
1. **During planning**: Read this to understand requirements and design considerations
2. **During implementation**: Refer back for detailed code patterns and edge cases

---

## Concept Overview

In Project 0, pipes were simple: read returns what's available (or 0 if empty), write puts data in buffer (or fails if full). But with Fork, we now have real concurrency - parent and child can access the same pipe simultaneously.

Two problems arise:

1. **Race conditions**: What if parent and child both try to read/write at the same time? The buffer pointers and counts could get corrupted.

2. **Blocking semantics**: Real pipes should *block* when empty (reader waits for data) or full (writer waits for space). The P0 non-blocking behavior is inadequate for real IPC.

This component adds mutex protection and condition variable signaling to pipes, making them safe for concurrent access and giving them proper blocking behavior.

---

## Learning Objectives

By the end of this component, students should be able to:

1. Identify why the P0 pipe implementation is unsafe for concurrent access
2. Explain the difference between spinlocks and mutexes, and why mutexes are appropriate here
3. Use condition variables to implement blocking semantics (wait/signal pattern)
4. Implement the classic producer-consumer pattern for pipe read/write

**Key concept: Mutexes and Condition Variables** — This component teaches blocking synchronization, distinct from the atomic operations in reference counting. Before implementing, ensure the student understands:
- Why mutexes allow sleeping (vs. spinlocks which busy-wait)
- The `while` loop pattern for condition variables (spurious wakeups)
- Why you must hold the mutex when calling `Cond_Wait`

→ *Invoke `tutor-atomics` skill before implementing — it covers mutex/condvar as well as atomics.*

---

## Code to Read First

Before implementing, students should read and understand:

| File | Section | What to notice |
|------|---------|----------------|
| `src/geekos/synch.c` | `Mutex_Lock/Unlock` | How mutexes work in GeekOS |
| `src/geekos/synch.c` | `Cond_Wait/Signal` | Condition variable implementation |
| `docs/p1/project1.md` | Section 1.6.3 | Mutex and condition variable guidance |
| `src/geekos/pipe.c` | Current implementation | What needs protection |

**Reading questions** (use to check understanding):

- What's the difference between `Spin_Lock` and `Mutex_Lock`?
- Why does `Cond_Wait` take a mutex as a parameter?
- In the current `Pipe_Read`, what happens if two threads call it simultaneously?
- Why do we use `while (!condition)` instead of `if (!condition)` with `Cond_Wait`?

---

## Implementation Overview

You need to add to `struct Pipe`:

```c
struct Mutex mutex;           /* Protects all pipe state */
struct Condition notEmpty;    /* Signaled when data becomes available */
struct Condition notFull;     /* Signaled when space becomes available */
```

Then modify:

1. **Pipe_Create**: Initialize mutex and condition variables
2. **Pipe_Read**: Lock mutex, wait if empty, read data, signal notFull, unlock
3. **Pipe_Write**: Lock mutex, wait if full, write data, signal notEmpty, unlock
4. **Pipe_Close**: Handle cleanup (wake any waiting threads?)

### Key decisions/considerations

- **Blocking vs. non-blocking**: Should read block forever if no writers exist? (No - return 0/EOF)
- **Partial reads/writes**: Can a read return less than requested if some data is available?
- **Close semantics**: If a reader is blocked and the last writer closes, what happens?

### The producer-consumer pattern

```c
// Reader (consumer)
Mutex_Lock(&pipe->mutex);
while (pipe->dataSize == 0 && pipe->writers > 0) {
    Cond_Wait(&pipe->notEmpty, &pipe->mutex);
}
// ... read data ...
Cond_Signal(&pipe->notFull);
Mutex_Unlock(&pipe->mutex);

// Writer (producer)
Mutex_Lock(&pipe->mutex);
while (pipe->dataSize == PIPE_BUFFER_SIZE && pipe->readers > 0) {
    Cond_Wait(&pipe->notFull, &pipe->mutex);
}
// ... write data ...
Cond_Signal(&pipe->notEmpty);
Mutex_Unlock(&pipe->mutex);
```

---

## Student Contribution Opportunities

These are good places for students to write code themselves:

| Code section | Complexity | Why it's educational |
|--------------|------------|---------------------|
| The wait condition in Pipe_Read | Medium | Understanding blocking semantics |
| The signal logic in Pipe_Write | Medium | Producer-consumer signaling |
| Close() wakeup logic | Medium | Edge case handling |

**Note**: Ask student preference before assuming they want to code.

---

## Understanding Checks

Questions to verify comprehension (not a quiz - use conversationally):

### Before implementation

- "What would happen if two processes both called Pipe_Read at the same time with our P0 code?"
- "Why can't we just disable interrupts instead of using a mutex here?"
- "What's the difference between Cond_Signal and Cond_Broadcast?"

### After implementation

- "Walk me through what happens when a reader blocks on an empty pipe, then a writer adds data"
- "Why do we check `pipe->writers > 0` in the read wait condition?"
- "What happens if both reader and writer are blocked? How do they make progress?"

---

## Testing

**How to test**:

- `forkpipe` - Tests pipe communication across fork
- Manual test: Parent writes, child reads (should block until data available)
- Manual test: Fill pipe completely, verify writer blocks

**What tests DON'T cover**:

- Exact timing of blocking/waking (hard to test deterministically)
- Stress testing with many concurrent readers/writers

**Build command**:
```bash
# In Docker container:
cd /geekos/build
make clean && make
```

---

## Common Mistakes and Debugging

**Typical errors**:

| Symptom | Likely cause | How to investigate |
|---------|--------------|-------------------|
| Deadlock (system hangs) | Forgot to unlock mutex on error path | Check all return paths |
| Data corruption | Not holding mutex during buffer access | Review lock/unlock placement |
| Reader never wakes | Signal before unlock? Wrong condition? | Add Print() to trace |
| Busy-waiting instead of blocking | Using `if` instead of `while` | Check loop structure |

**Debugging strategies**:

- Add `Print("Pipe_Read: waiting, dataSize=%d\n", pipe->dataSize);` before Cond_Wait
- Add `Print("Pipe_Write: signaling notEmpty\n");` after Cond_Signal
- Use `forkbomb` to stress-test cleanup paths

---

## Connections

**Relates to concepts from**:

- OSTEP Chapter 30: Condition Variables
- OSTEP Chapter 31: Semaphores
- Classic producer-consumer problem from any OS textbook

**Builds on**:

- Reference counting (atomics) - different synchronization tool, different use case
- Fork - creates the concurrent processes that access the pipe

**Will be used by**:

- Shell pipelines: `ls | grep foo` requires blocking pipes
- Any inter-process communication

---

## Learning Opportunities

These are teachable moments likely to arise during this component. When you encounter one, consider whether to engage the student based on their familiarity and involvement preferences.

**Format**: Each opportunity is phrased as a question to help identify the moment, without giving away the answer. Tags indicate severity and topic.

---

### Why Synchronization Now?

**[major] [concurrency] [design]**
Our P0 pipe worked fine without any locks. What changed? Why do we suddenly need synchronization?

**[minor] [concurrency]**
If only one CPU can run at a time (uniprocessor), do we still need mutexes? What about just disabling interrupts?

---

### Mutex vs Spinlock

**[major] [concurrency] [design]**
GeekOS has both `Spin_Lock` and `Mutex_Lock`. When should you use each? What's the key difference in behavior when the lock is held?

**[minor] [performance]**
A spinlock is "faster" in some sense (no context switch). Why is that actually a problem for pipe operations that might wait a long time?

---

### Condition Variable Patterns

**[major] [concurrency] [debugging]**
Why do we use `while (condition)` instead of `if (condition)` before `Cond_Wait`? What could go wrong with `if`?

**[major] [concurrency]**
Why does `Cond_Wait` need the mutex as a parameter? What does it do with the mutex, and why?

**[minor] [design]**
When should you use `Cond_Signal` vs `Cond_Broadcast`? What's the tradeoff?

---

### Producer-Consumer Pattern

**[major] [design]**
The reader waits when the buffer is empty; the writer waits when it's full. But what additional condition must the reader check before deciding to wait?

**[minor] [design]**
After writing data, we signal `notEmpty`. Should we signal before or after releasing the mutex? Does it matter?

---

### Close and Wakeup

**[major] [concurrency] [edge-cases]**
If a reader is blocked waiting for data and the last writer closes, what should happen? How does the reader know to stop waiting?

**[minor] [debugging]**
If we forget to wake up waiting threads when closing, what symptom would we see? How would you debug it?

---

### Deadlock Avoidance

**[minor] [concurrency] [debugging]**
What could cause a deadlock in our pipe implementation? How do we ensure the mutex is always released, even on error paths?

**[minor] [design]**
We have one mutex per pipe. Could we get better concurrency with finer-grained locking (e.g., separate locks for read and write)? What complications would that introduce?

---

## Notes for the Tutor

**Good entry questions**:
- "What would happen if parent and child both write to the pipe at the same time?"
- "Should Pipe_Read return immediately if the pipe is empty, or wait?"

**Common misconceptions**:
- Students may think the P0 pipe is already thread-safe (it's not!)
- Students may use spinlocks instead of mutexes (wrong for potentially long waits)
- Students may forget the `while` loop around Cond_Wait (spurious wakeups)
- Students may signal while still holding data that should be released first

**Connection to atomics skill**:
This component uses mutexes/condvars, which are covered in `tutor-atomics` along with atomic operations. The skill explains when to use each tool.

**If student is stuck on the wait condition**:
- Ask: "When should the reader stop waiting?"
- Draw the state machine: empty → waiting → data arrives → read → signal
- Point to the producer-consumer pattern in the overview
