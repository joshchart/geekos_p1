# Component: Fork

**Prerequisites**: Reference counting (completed), understanding of `struct Kernel_Thread` and `struct User_Context`
**Builds toward**: Execl (fork + execl is how shells run programs)

**How to use this guide**:
1. **During planning**: Read this to understand requirements, design considerations, and overall structure
2. **During implementation**: Refer back for detailed code patterns, edge cases, and common pitfalls

---

## Concept Overview

`Fork()` creates a new process that is a copy of the calling process. After fork:
- There are two processes running the same code
- They have separate copies of memory (global variables, stack)
- They share open files (if one seeks, both see the new position)
- The parent gets the child's PID as the return value; the child gets 0

This is the Unix process creation model. Combined with `Execl()`, it's how every program you run gets started. The shell forks, and the child runs a new program via Execl.

The tricky parts:
1. **Copying the address space** — the child needs its own memory
2. **Sharing files** — the file descriptors point to shared `struct File` objects
3. **Setting up return values** — fork "returns twice" with different values
4. **Kernel stack magic** — the child's kernel stack must look like the parent's at the syscall point

---

## Learning Objectives

By the end of this component, students should be able to:

1. Explain what state is copied vs. shared in fork
2. Trace through the kernel stack to understand how fork "returns twice"
3. Implement address space copying using existing GeekOS functions
4. Correctly increment reference counts for shared files
5. Calculate the child's ESP based on the parent's stack layout

---

## Code to Read First

Before implementing, students should read and understand:

| File | Section | What to notice |
|------|---------|----------------|
| `include/geekos/kthread.h` | `struct Kernel_Thread` | Fields like `stackPage`, `esp`, `userContext` |
| `include/geekos/user.h` | `struct User_Context` | Memory pointer, size, file descriptor table |
| `src/geekos/kthread.c` | `Create_Thread()` | How kernel threads are allocated |
| `src/geekos/kthread.c` | `Setup_User_Thread()` | How the kernel stack is set up for user processes |
| `src/geekos/userseg.c` | `Create_User_Context()` | How user memory is allocated |
| `src/geekos/user.c` | `Spawn()` | The existing process creation path (fork will reuse parts) |

**Reading questions** (use to check understanding):

- What is `stackPage` in `Kernel_Thread`? What is it used for?
- What is `esp` in `Kernel_Thread`? What does it point to during a system call?
- Look at `Setup_User_Thread()` — what gets pushed onto the kernel stack?
- In `User_Context`, what is the `memory` field? How big is it?
- Look at `file_descriptor_table` — how many file descriptors can a process have?

---

## Implementation Overview

`Sys_Fork()` needs to:

1. **Create a new `Kernel_Thread`** for the child (use `Create_Thread()`)
2. **Create a new `User_Context`** for the child (use `Create_User_Context()`)
3. **Copy the parent's user memory** to the child (`memcpy`)
4. **Copy relevant `User_Context` fields** (but NOT selectors, entry point, etc.)
5. **Copy the file descriptor table** and increment `refCount` for each open file
6. **Copy the kernel stack** from parent to child
7. **Set the child's ESP** to point to the right place in its stack
8. **Set the child's return value (EAX)** to 0
9. **Add the child to the run queue** (use `Make_Runnable_Atomic()`)
10. **Return the child's PID** to the parent

### Key decisions/considerations

- **What NOT to copy in User_Context**: `ldtDescriptor`, `ldtSelector`, `csSelector`, `dsSelector` — these are set up by `Create_User_Context()` for the child's own segments.
- **What to copy**: `size`, `name`, the actual memory contents, the file descriptor table.
- **ESP calculation**: The child's ESP should be at the same offset from its `stackPage` as the parent's ESP is from the parent's `stackPage`.

### The kernel stack / ESP puzzle

When fork is called, the parent's registers are pushed onto its kernel stack. The structure at `esp` is an `Interrupt_State`. To make the child resume at the same point:

1. Copy the parent's entire kernel stack to the child
2. Calculate where the child's ESP should be:
   ```
   child_esp = child->stackPage + (parent_esp - parent->stackPage)
   ```
   Or equivalently:
   ```
   offset = (parent->stackPage + PAGE_SIZE) - parent->esp
   child_esp = (child->stackPage + PAGE_SIZE) - offset
   ```
3. Find the `Interrupt_State` in the child's stack and set `eax` to 0

### Return value diagram

```
Parent's kernel stack:              Child's kernel stack (after copy):
┌────────────────────┐              ┌────────────────────┐
│  ...               │              │  ...               │
├────────────────────┤              ├────────────────────┤
│  Interrupt_State   │              │  Interrupt_State   │
│  - eax = ???       │              │  - eax = 0         │  ← Modified!
│  - other regs      │              │  - other regs      │
├────────────────────┤              ├────────────────────┤
│  ...               │              │  ...               │
└────────────────────┘              └────────────────────┘
     ↑ parent->esp                       ↑ child->esp (calculated)
```

When the child returns from the syscall, its `eax` (which holds the return value) will be 0.

---

## Student Contribution Opportunities

These are good places for students to write code themselves:

| Code section | Complexity | Why it's educational |
|--------------|------------|---------------------|
| ESP calculation | Medium | Understanding stack layout |
| File descriptor copying + refCount | Medium | Connecting to reference counting component |
| Setting child's EAX to 0 | Low-Medium | Understanding return value mechanism |
| Overall Sys_Fork structure | High | Putting all the pieces together |

**Note**: Ask student preference. Some may want to implement the whole thing; others may want guidance on the ESP calculation.

---

## Understanding Checks

Questions to verify comprehension (not a quiz — use conversationally):

### Before implementation

- "After fork, if the child modifies a global variable, does the parent see it? Why or why not?"
- "After fork, if the child seeks in a file, does the parent's file position change? Why?"
- "How does fork return different values to parent and child if it's the same code?"

### After implementation

- "Walk me through what happens when the child is scheduled for the first time"
- "If we forgot to set the child's EAX to 0, what would the child see as fork's return value?"
- "What would happen if we forgot to increment refCount for inherited files?"
- "What if `Create_User_Context()` or `Create_Thread()` returns NULL?"

---

## Testing

**Available tests**:
- `fork-p1` - Basic fork functionality
- `forkpipe` - Fork with pipes (tests file sharing)
- `forkexec` - Fork then Execl (tests complete flow)
- `forkbomb` - Resource exhaustion (tests ENOMEM handling)

**What tests DON'T cover** (for thinking about correctness):
- Exact memory layout correctness (hard to test automatically)
- Race conditions in file descriptor access
- Very large processes (memory limits)

**How to run**:
```bash
# In QEMU shell after booting:
/c/fork-p1.exe
/c/forkpipe.exe
```

---

## Common Mistakes and Debugging

**Typical errors**:

| Symptom | Likely cause | How to investigate |
|---------|--------------|-------------------|
| Page fault in child | ESP calculated wrong, or memory not copied | Print ESP values, check memcpy size |
| Child returns garbage (not 0) | Didn't set EAX in child's Interrupt_State | Print child's EAX before returning |
| Files not shared properly | Forgot to increment refCount | Check refCount after fork |
| Child has wrong globals | User memory not copied correctly | Print memory address/size |
| Crash after child exits | Double-free of File (refCount wrong) | Check Close() refCount logic |
| KASSERT failure | Null pointer from allocation failure | Check return values of Create_* functions |

**Debugging strategies for this component**:

- Add `Print("Fork: parent esp=%x stackPage=%x\n", ...)` to see stack layout
- Add `Print("Fork: child esp=%x\n", child->esp)` to verify calculation
- Print the child's Interrupt_State fields to verify EAX=0
- Use `forkbomb` to test ENOMEM handling (should fail gracefully)

---

## Connections

**Relates to concepts from**:

- OSTEP Chapter 5: Process API (fork, wait)
- Process control blocks and context switching
- Virtual address spaces (each process has its own)
- Copy-on-write optimization (not implemented here, but real systems use it)

**Will be used by**:

- Execl (typically called right after fork in child)
- Shell implementation (fork + run new program for each command)
- Pipe implementation testing (forkpipe test)

---

## Learning Opportunities

These are teachable moments likely to arise during this component. When you encounter one, consider whether to engage the student based on their familiarity and involvement preferences.

**Format**: Each opportunity is phrased as a question to help identify the moment, without giving away the answer. Tags indicate severity and topic.

---

### Resource Management

**[major] [memory] [design]**
If a call requires making several allocations (thread, user context, memory), and one can fail partway through, what's a good way to structure the code to handle cleanup?

**[major] [memory]**
When a forked child process exits, what happens to its User_Context? How do we know it's safe to free?

**[minor] [memory]**
We're copying the parent's entire memory. Is there a more efficient approach? (Hint: copy-on-write)

---

### Stack and ESP

**[major] [design] [debugging]**
The child's ESP needs to point to "the same place" in its stack as the parent's ESP points to in the parent's stack. What does "the same place" mean here? How do we calculate it?

**[minor] [debugging]**
If we calculated ESP wrong, what symptoms would we see? How would we debug this?

---

### Return Values

**[major] [design]**
Fork "returns twice" with different values. How is this possible? Where does the return value actually come from?

**[minor] [design]**
Why does the child get 0 and the parent get the child's PID? What would happen if it were reversed?

---

### File Descriptor Sharing

**[major] [design]**
After fork, if the child seeks in a file, does the parent's file position change? Why or why not?

**[minor] [design]**
Why are files shared but memory isn't? What's fundamentally different about them?

---

### Testing and Verification

**[major] [debugging] [testing]**
If fork-p1 passes, do we know that our fork code is correct? What else should we be checking for?

**[minor] [debugging]**
If a test isn't working and the problem isn't obvious, what can we do to get more information before changing code?

---

### Orphan Processes

**[major] [design] [memory]**
When a parent process exits, what should happen to its children? How do we find them all?

**[major] [concurrency] [design]**
If we need to iterate through the thread list to find orphans, but detaching a thread modifies the list, how do we handle this safely?

---

## Notes for the Tutor

**Good entry questions**:
- "Have you used fork() in a Unix programming class? What did you notice about it?"
- "Why do you think Unix separates process creation from running a new program?"

**Common misconceptions**:
- Students sometimes think fork creates a *reference* to parent memory, not a copy. Clarify: separate address spaces, copied at fork time.
- The "returns twice" concept is confusing. Draw a timeline showing parent and child as separate execution flows.
- Students may not realize the kernel stack is per-process. Each process has its own stackPage.

**The ESP calculation is often the hardest part**:
- Draw the stack layout on paper
- Emphasize that ESP is an address, and we need to find the corresponding address in the child's stack
- The offset from the top of the stack page should be the same

**If student is stuck on the EAX modification**:
- Show them the `Interrupt_State` structure
- Explain that when the syscall returns, registers are popped from this structure
- The child's `Interrupt_State` is somewhere in its kernel stack — same offset as parent's

**Why does fork copy memory instead of sharing?**
- Process isolation: one process shouldn't affect another's memory
- Real systems use copy-on-write for efficiency (pages are shared until modified)
- GeekOS keeps it simple with immediate copy

**Why are files shared but memory isn't?**
- Files are external resources (disk state)
- Sharing makes pipelines work: parent writes, child reads
- Memory is internal process state — sharing would violate isolation
