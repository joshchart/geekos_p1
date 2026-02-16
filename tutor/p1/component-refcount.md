# Component: File Reference Counting

**Prerequisites**: Understanding of `struct File`, how files are opened/closed in GeekOS
**Builds toward**: Fork (which needs to share file descriptors between parent and child)

**How to use this guide**:
1. **During planning**: Read this to understand requirements and design considerations
2. **During implementation**: Refer back for detailed code patterns and edge cases

---

## Concept Overview

When a process opens a file, the kernel creates a `struct File` object to track the open file (position, mode, etc.). But what happens when `fork()` creates a child process? Both parent and child should share the *same* open file - if one seeks, the other should see the new position.

This means multiple processes can hold references to a single `struct File`. We need to track how many processes are using each file, so we only truly close it (freeing resources, flushing buffers) when the *last* process closes it.

This is the same concept as reference counting in memory management, garbage collection, or smart pointers - we're tracking "who's still using this resource" to know when it's safe to release.

---

## Learning Objectives

By the end of this component, students should be able to:

1. Explain why reference counting is needed for shared files
2. Identify where refCount should be incremented and decremented in the code
3. Reason about what could go wrong with concurrent access to refCount
4. Implement the reference counting logic in `Close()`

**Key concept: Atomic operations** — This component introduces SMP-safe programming with hardware atomics. Before implementing, ensure the student understands:
- Why disabling interrupts doesn't provide atomicity on SMP systems
- How `Atomic_Decrement()` uses hardware instructions for true atomicity
- The race condition that occurs with naive decrement-and-check

→ *Invoke `tutor-atomics` skill before explaining or implementing the Close() logic.*

---

## Code to Read First

Before implementing, students should read and understand:

| File | Section | What to notice |
|------|---------|----------------|
| `include/geekos/vfs.h` | `struct File` (lines 86-102) | Current fields - no refCount yet! |
| `src/geekos/vfs.c` | `Allocate_File()` (lines 481-496) | Where File objects are created |
| `src/geekos/vfs.c` | `Close()` (lines 406-417) | The TODO marker and current close logic |

**Important naming note**: When showing `Close()`, point out that `rc` stands for "**r**eturn **c**ode" (not reference count!). This is a common C idiom for error/status values (0 = success, negative = error). The naming can be confusing when discussing reference counting.

**Reading questions** (use to check understanding):

- What fields does `struct File` currently have? What does each one represent?
- When `Close()` is called right now, what happens? Does it always free the File?
- Look at the comment on line 411 - what does `TODO_P(PROJECT_FORK, ...)` mean?
- If two processes have the same File (after fork), and one calls `Close()`, what *should* happen?

---

## Implementation Overview

You need to make three changes:

1. **Add a `refCount` field** to `struct File` in `vfs.h`
2. **Initialize refCount to 1** in `Allocate_File()` when a File is created
3. **Manage refCount in `Close()`** - decrement it, and only do the actual close when it reaches 0

### Key decisions/considerations

- **Where to increment refCount**: Not here! That happens in Fork (a later component). This component just sets up the infrastructure.
- **Thread safety**: What if two threads call `Close()` on the same File simultaneously? One might read refCount as 1, start closing, while the other also reads it as 1...

### Common approaches

- **Atomic operations**: Use `Atomic_Decrement()` from `<geekos/atomic.h>` for lock-free, SMP-safe reference counting. This uses hardware atomic instructions (`lock xadd` on x86) that work correctly even when multiple CPUs access the same memory simultaneously.
- **Mutex**: Could use `s_vfsLock` (the VFS's global mutex), but that's heavy-handed for a single integer

**Important**: Do NOT use `Save_And_Disable_Interrupts()` for this! Disabling interrupts only affects the current CPU - on SMP systems, another CPU can still be running concurrently. Atomic operations are the correct solution for reference counting.

---

## Student Contribution Opportunities

These are good places for students to write code themselves:

| Code section | Complexity | Why it's educational |
|--------------|------------|---------------------|
| Adding `refCount` field to struct | Low | Understanding struct modification |
| Initializing refCount in `Allocate_File` | Low | Seeing where objects are born |
| Close() refCount logic | Medium | Core reference counting pattern |

**Note**: Ask student preference before assuming they want to code.

---

## Understanding Checks

Questions to verify comprehension (not a quiz - use conversationally):

### Before implementation

- "What problem are we solving? Why can't we just close files normally?"
- "If fork copies file descriptors but doesn't increment refCount, what goes wrong?"
- "Draw the scenario: Parent opens file, forks, child closes file, parent writes. What happens with vs. without refCount?"

### After implementation

- "Walk me through what happens when Close() is called with refCount = 2"
- "What invariant should refCount always satisfy?" (always >= 1 while File exists)
- "Why do we use `Atomic_Decrement` instead of a full mutex?" (lock-free is faster, sufficient for single-integer operations)
- "What would happen if we decremented refCount *after* checking if it's 1?"
- "Why can't we just disable interrupts instead of using atomic operations?" (SMP - other CPUs still running)

---

## Testing

**How to test**: Reference counting itself isn't directly testable until Fork is implemented. However:

- The code should still compile and run
- Existing file operations should still work (open, read, write, close)
- Single-process programs should behave identically to before

**What tests DON'T cover** (for thinking about correctness):

- Concurrent access (only matters in SMP, hard to trigger reliably)
- The increment side (tested when Fork is implemented)

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
| Compile error: unknown field | Forgot to add refCount to struct | Check `vfs.h` struct File |
| File freed while still in use | Decrementing without atomic protection | Add Print() to see refCount values |
| File never freed | Forgot to decrement, or wrong condition | Add Print() in Close() |
| Confusion about `rc` variable | Student thinks `rc` is reference count | Clarify: `rc` = return code (C idiom) |

**Debugging strategies for this component**:

- Add `Print("Close: refCount=%d\n", file->refCount);` at start of Close()
- Watch for double-free symptoms (strange crashes later)
- Test with simple single-process programs first

---

## Connections

**Relates to concepts from**:

- Memory management (malloc/free need similar lifetime tracking)
- OSTEP Chapter 39: Files and Directories
- Smart pointers in C++ (shared_ptr uses reference counting)
- Garbage collection reference counting algorithms

**Will be used by**:

- Fork (increments refCount when copying file descriptors)
- Pipe (pipes are also shared via file descriptors)
- Exec (preserves open files, so refCount stays the same)

---

## Learning Opportunities

These are teachable moments likely to arise during this component. When you encounter one, consider whether to engage the student based on their familiarity and involvement preferences.

**Format**: Each opportunity is phrased as a question to help identify the moment, without giving away the answer. Tags indicate severity and topic.

---

### Core Concept

**[major] [design]**
Why do we need reference counting at all? What would go wrong if we just closed files normally when Close() is called?

**[major] [design]**
Draw the scenario: Parent opens file, forks, child closes file, parent writes. What happens with vs. without reference counting?

---

### Concurrency

**[major] [concurrency] [debugging]**
What if two threads call Close() on the same File simultaneously? What could go wrong with a naive decrement-and-check?

**[major] [concurrency]**
Why do we use `Atomic_Decrement()` instead of just disabling interrupts? What's different about SMP systems?

**[minor] [concurrency] [performance]**
Could we use a mutex instead of atomic operations? What are the tradeoffs?

---

### Implementation

**[minor] [design]**
Where should we initialize refCount? What should the initial value be, and why?

**[minor] [debugging]**
The variable `rc` in Close() stands for "return code", not "reference count". Why might this naming be confusing?

---

### Connections

**[minor] [design]**
What other resources in an OS might need reference counting? (memory pages, device handles, etc.)

**[minor] [design]**
This is similar to `shared_ptr` in C++ or reference counting in Python/Swift. What patterns are the same?

---

## Notes for the Tutor

**Good entry questions**:
- "Have you heard of reference counting before? Where?"
- "What happens in C if you free() memory that's still being used?"

**Common misconceptions**:
- Students sometimes think each process gets a *copy* of the File, not a shared reference
- Students may not initially realize the race condition in decrement-and-check
- Some students want to use a full mutex - discuss why atomics suffice here
- **IMPORTANT**: Students (and tutors!) may think `Save_And_Disable_Interrupts()` provides SMP atomicity - it does NOT! It only disables interrupts on the current CPU. Other CPUs can still run concurrently. For reference counting, use `Atomic_Decrement()` from `<geekos/atomic.h>` which uses hardware atomic instructions.
- If a student mentions "atomic" operations, verify they understand the difference between disabling interrupts (per-CPU) vs. hardware atomic instructions (true SMP atomicity)

**C style questions that may arise**:
- `bool` vs `int` for booleans: C99 added `<stdbool.h>`, GeekOS uses it, prefer `bool` for clarity
- Variable declaration placement: C99+ allows declarations anywhere (not just top of block), modern style is near first use
- `file->ops->Close(file)` is a vtable pattern: function pointers for polymorphism in C

**If student is stuck on the Close() logic**:
- Ask them to write pseudocode first
- Remind them: "What two things need to happen atomically?"
- Point to similar patterns in the codebase (if any exist)

**Good follow-up after completion**:
- "Where will we increment refCount?" (leads naturally to Fork discussion)
- "What other resources in an OS might need reference counting?" (memory pages, device handles)
