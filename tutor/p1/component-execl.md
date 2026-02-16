# Component: Execl

**Prerequisites**: Fork (completed), understanding of how programs are loaded
**Builds toward**: Complete process creation (fork + execl pattern)

**How to use this guide**:
1. **During planning**: Read this to understand requirements, design considerations, and overall structure
2. **During implementation**: Refer back for detailed code patterns, edge cases, and common pitfalls

---

## Concept Overview

`Execl()` replaces the current process's program with a new one. The process ID stays the same, but everything else changes:
- New code, data, and stack from the executable file
- Registers reset to start the new program
- **But**: file descriptors are preserved (this is how shell I/O redirection works!)

After a successful execl, there is no "return" — the old program is gone. The new program just starts running. Execl only returns if something goes wrong (file not found, out of memory, etc.).

The typical Unix pattern is:
```c
if (fork() == 0) {
    // Child process
    execl("/bin/ls", "ls", "-l", NULL);
    // Only reaches here if execl failed
    exit(1);
}
// Parent continues here
```

---

## Learning Objectives

By the end of this component, students should be able to:

1. Explain why file descriptors are preserved across execl
2. Trace through the `Spawn()` function to understand program loading
3. Implement execl by reusing program loading logic from Spawn
4. Properly clean up the old user context before loading the new program
5. Set up the new program's entry point and stack

---

## Code to Read First

Before implementing, students should read and understand:

| File | Section | What to notice |
|------|---------|----------------|
| `src/geekos/user.c` | `Spawn()` | The complete process creation flow — execl reuses much of this |
| `src/geekos/user.c` | `Spawn_Program()` | Lower-level program loading |
| `src/geekos/elf.c` | `Parse_ELF_Executable()` | How ELF files are parsed |
| `src/geekos/userseg.c` | `Load_User_Program()` | Loading program into user memory |
| `src/geekos/kthread.c` | `Setup_User_Thread()` | Setting up entry point and stack |

**Reading questions** (use to check understanding):

- In `Spawn()`, what happens to the file descriptor table? (It's initialized to empty)
- Where does `Spawn()` get the program's entry point from?
- What is `Setup_User_Thread()` responsible for?
- What's the difference between `Spawn()` and what execl needs to do?

---

## Implementation Overview

`Sys_Execl()` needs to:

1. **Copy the program path and command string** from user space to kernel space
2. **Save the current file descriptor table** (we need to preserve it)
3. **Load the new program** (reuse `Load_User_Program()` or similar)
4. **Replace the old user context** with the new one
5. **Restore the file descriptor table** to the new context
6. **Set up the kernel stack** for the new program entry (use `Setup_User_Thread()`)
7. **Do NOT return** — the new program starts running when the syscall "returns"

### What's different from Spawn?

| Spawn | Execl |
|-------|-------|
| Creates new Kernel_Thread | Reuses existing Kernel_Thread |
| Creates new User_Context | Creates new User_Context, destroys old |
| Empty file descriptor table | **Inherits** file descriptor table |
| Adds thread to run queue | Thread is already running |

### The key insight

Execl is like Spawn, but:
- We already have a `Kernel_Thread` (the current one)
- We need to keep the file descriptors
- We don't add to the run queue — we're already running

### Getting parameters from registers

The syscall receives:
- `state->ebx` — user address of program path
- `state->ecx` — length of program path
- `state->edx` — user address of command string
- `state->esi` — length of command string

Use `Copy_From_User()` or look at how other syscalls copy strings from user space.

---

## Student Contribution Opportunities

These are good places for students to write code themselves:

| Code section | Complexity | Why it's educational |
|--------------|------------|---------------------|
| Copying path from user space | Low | Standard syscall pattern |
| Saving/restoring file descriptors | Medium | Understanding what's preserved |
| Calling the program loading functions | Medium | Reusing existing code |
| Error handling | Medium | Cleanup on failure |

**Note**: Ask student preference. The main challenge is understanding the flow and reusing existing code correctly.

---

## Understanding Checks

Questions to verify comprehension (not a quiz — use conversationally):

### Before implementation

- "After execl, does the process have the same PID? The same file descriptors? The same global variables?"
- "Why is it important that file descriptors are preserved?"
- "What happens if execl fails? Where does control return to?"

### After implementation

- "Walk me through what happens from the execl call to the new program's main()"
- "If we forgot to preserve file descriptors, what would break?"
- "What would happen if the executable file doesn't exist?"
- "Does Setup_User_Thread create a new kernel stack?"

---

## Testing

**Available tests**:
- `forkexec` - Fork then execl (the standard pattern)

**What tests DON'T cover** (for thinking about correctness):
- Execl without fork (rare but valid)
- File descriptor inheritance with specific files
- Memory leaks from old user context

**How to run**:
```bash
# In QEMU shell after booting:
/c/forkexec.exe
```

---

## Common Mistakes and Debugging

**Typical errors**:

| Symptom | Likely cause | How to investigate |
|---------|--------------|-------------------|
| Page fault after execl | Kernel stack not set up correctly | Check Setup_User_Thread call |
| File descriptors lost | Forgot to save/restore them | Print FDT before and after |
| Old program continues | Didn't set up new entry point | Check EIP in Interrupt_State |
| Memory leak | Didn't destroy old User_Context | Check Destroy_User_Context call |
| Path not found | Didn't copy path from user space correctly | Print the path |

**Debugging strategies for this component**:

- Add `Print("Execl: loading %s\n", path)` to see what's being loaded
- Check that `Setup_User_Thread()` is called with the new context
- Print the new entry point address

---

## Connections

**Relates to concepts from**:

- OSTEP Chapter 5: Process API
- ELF executable format (briefly)
- How shells implement commands
- I/O redirection (`cmd > file` works because file descriptors survive execl)

**Will be used by**:

- Shell implementation
- Complete fork+execl pattern for running programs

---

## Learning Opportunities

These are teachable moments likely to arise during this component. When you encounter one, consider whether to engage the student based on their familiarity and involvement preferences.

**Format**: Each opportunity is phrased as a question to help identify the moment, without giving away the answer. Tags indicate severity and topic.

---

### Design and Reuse

**[major] [design]**
Spawn() already loads and runs programs. What parts can we reuse for execl? What's different?

**[minor] [design]**
If there's a function that seems useful (like something in Spawn), but it's declared `static`, what are our options?

---

### File Descriptor Preservation

**[major] [design]**
Why is it important that file descriptors are preserved across execl? How does shell I/O redirection depend on this?

**[minor] [design]**
We need to save the file descriptor table before destroying the old context, then restore it to the new context. What's the cleanest way to do this?

---

### Error Handling

**[major] [design] [debugging]**
If execl fails (file not found, out of memory), what should happen? Should the old program continue running?

**[major] [design]**
If we destroy the old user context before loading the new program, and loading fails, what happens? How should we order the operations?

---

### The "No Return" Nature

**[minor] [design]**
Execl "doesn't return" on success. What does this actually mean? Where does the old code go?

**[minor] [debugging]**
If the old program continues running after execl, what went wrong? Where should we look?

---

### Stack Setup

**[minor] [design]**
Setup_User_Thread doesn't create a new kernel stack - it reuses the existing one. Why does this make sense for execl?

**[minor] [debugging]**
If the new program crashes immediately after execl, what might be wrong with how we set up the entry point?

---

### Unix Philosophy

**[minor] [design]**
Why does Unix have separate fork and execl instead of one combined call like Windows' CreateProcess? What patterns does this enable?

---

## Notes for the Tutor

**Good entry questions**:
- "Why do you think Unix has separate fork and execl instead of one combined call?"
- "How does shell I/O redirection work? (e.g., `ls > file.txt`)"

**The Unix philosophy insight**:
The fork+execl separation enables powerful patterns:
- Fork, then redirect file descriptors, then execl
- Fork, then set up pipes between processes, then execl
- This modularity is more flexible than Windows's CreateProcess

**Common misconceptions**:
- Students sometimes think execl creates a new process. It doesn't — it replaces the current one.
- Students may not realize that "not returning" means the old code is completely gone.
- The file descriptor preservation is the key insight for understanding Unix I/O.

**If student is confused about what to reuse from Spawn**:
- Walk through Spawn step by step
- Identify which parts create new things (don't need) vs. load program (do need)
- The program loading and `Setup_User_Thread` are the key reusable pieces

**Error handling is important**:
- If program loading fails, the old program should continue running
- This means: load new program first, only destroy old context on success
- Or: be very careful about the order of operations

**Setup_User_Thread does NOT create a new kernel stack**:
- It reuses the existing Kernel_Thread
- It pushes initial values onto the kernel stack
- When the syscall "returns," these values load the new program
