# Component: Pipes (Non-Blocking)

**Prerequisites**: Basic C, understanding of pointers and structs
**Builds toward**: Project 1 (Fork will share pipes between parent/child)

**How to use this guide**:
1. **During planning**: Read this to understand requirements, design considerations, and overall structure
2. **During implementation**: Refer back for detailed code patterns, edge cases, and common pitfalls

---

## Concept Overview

A pipe is a unidirectional communication channel. Data written to one end can be read from the other end. Pipes are the mechanism behind shell commands like `ls | grep foo`.

In this project, pipes are **non-blocking**:
- Read returns immediately (with data, `EWOULDBLOCK`, or EOF)
- Write returns immediately (with bytes written, `0` for full, or `EPIPE`)

This simplifies implementation - no need for condition variables or blocking. The focus is on:
- Understanding the VFS layer and file operations
- Designing kernel data structures
- Managing resources (allocation, cleanup)

**No synchronization needed in P0**: Without `fork()`, only one process can access the pipe, so there are no race conditions to protect against. We'll add synchronization in P1 when processes can share pipes.

---

## Learning Objectives

By the end of this component, students should be able to:

1. Explain the VFS layer's role and how File_Ops provides polymorphism
2. Design a kernel data structure (`struct Pipe`) to hold pipe state
3. Implement file operations that work through the VFS dispatch mechanism
4. Handle edge cases: EOF, broken pipe, buffer full/empty
5. Use `Copy_To_User`/`Copy_From_User` for kernel-user data transfer

---

## Code to Read First

Before implementing, students should read and understand:

| File | Section | What to notice |
|------|---------|----------------|
| `include/geekos/vfs.h` | `struct File` (lines ~86-102) | Fields: `ops`, `fsData`, `filePos`, `mode` |
| `include/geekos/vfs.h` | `struct File_Ops` | Function pointer table for file operations |
| `src/geekos/vfs.c` | `Read()` function | How it dispatches to `file->ops->Read()` |
| `src/geekos/pipe.c` | `Pipe_Read_Ops`, `Pipe_Write_Ops` | Two separate ops tables for read/write ends |
| `src/geekos/syscall.c` | `Sys_Read()`, `Sys_Write()` | How syscalls get file from fd and call VFS |

**Reading questions** (use to check understanding):

- What does `file->ops->Read(file, buf, len)` do? Why the indirection?
- Why are there two different `File_Ops` structs for pipes (`Pipe_Read_Ops` and `Pipe_Write_Ops`)?
- What is `fsData` used for in `struct File`?
- How does a user-space `read()` call eventually reach `Pipe_Read()`?

---

## Architecture Overview

```
pipe(&r, &w) returns r=3, w=4

struct User_Context
[0][1][2][3][4][5]...  ← File descriptor table
          |   |
          v   v
   struct File    struct File
   ops=Pipe_Read_Ops  ops=Pipe_Write_Ops
   fsData             fsData
          \          /
           v        v
         struct Pipe
         readers = 1 | writers = 1
         pointer to data buffer
                  |
                  v
            buffered data
```

Key insight: Both File structs point to the **same** Pipe struct via `fsData`. This is how the two ends communicate.

---

## Implementation Overview

### struct Pipe design

Create a `struct Pipe` to hold:
- **Data buffer**: Fixed size (32KB suggested) or dynamically allocated
- **Buffer state**: Start/end positions or read/write indices
- **Reader count**: How many open read file descriptors
- **Writer count**: How many open write file descriptors

### Key functions to implement

| Function | Purpose | Called by |
|----------|---------|-----------|
| `Pipe_Create()` | Allocate pipe and two File structs | `Sys_Pipe()` |
| `Sys_Pipe()` | Create pipe, add to fd table, return fds to user | System call |
| `Pipe_Read()` | Read from buffer, handle empty/EOF | VFS dispatch |
| `Pipe_Write()` | Write to buffer, handle full/EPIPE | VFS dispatch |
| `Pipe_Close()` | Decrement reader/writer count, free if last | VFS dispatch |

### Buffer design options

**Circular buffer (recommended)**:
- Fixed size array
- `readPos` and `writePos` indices
- Data available = `writePos - readPos` (mod buffer size)
- Handles wrap-around efficiently

**Simple linear buffer**:
- Shift data when reading (inefficient but simpler)
- Or track start/end and reallocate

---

## Implementation Details

**Note on synchronization**: You do NOT need mutexes, spinlocks, or condition variables in P0. Without `fork()`, only one process accesses the pipe, so there's no concurrent access. We'll add synchronization in P1 when pipes can be shared between parent and child processes.

### Pipe_Create

1. Allocate `struct Pipe` with `Malloc()`
2. Initialize buffer (fixed size or initial allocation)
3. Set `readers = 1`, `writers = 1`
4. Allocate two `struct File` objects (use `Allocate_File()`)
5. Set read file: `ops = &Pipe_Read_Ops`, `fsData = pipe`
6. Set write file: `ops = &Pipe_Write_Ops`, `fsData = pipe`
7. Return both files through output parameters

### Sys_Pipe

1. Call `Pipe_Create()` to get read and write File pointers
2. Add each File to the process's file descriptor table
3. Use `Copy_To_User()` to write the fd integers to user-provided addresses
4. Handle errors (allocation failure, fd table full)

### Pipe_Read (Non-blocking)

```
Get pipe from file->fsData
if (buffer has data):
    Copy min(requested, available) bytes to user buffer
    Update buffer read position
    return bytes copied
else if (writers exist):
    return EWOULDBLOCK
else:
    return 0  // EOF - no writers, no data
```

### Pipe_Write (Non-blocking)

```
Get pipe from file->fsData
if (no readers):
    return EPIPE  // Broken pipe
if (buffer has space):
    Copy min(requested, available_space) bytes from user buffer
    Update buffer write position
    return bytes written
else:
    return 0  // Buffer full (fixed size)
    // OR return ENOMEM if dynamic allocation fails
```

### Pipe_Close

1. Determine if this is read end or write end (check `file->ops`)
2. Decrement appropriate counter (`readers` or `writers`)
3. If both counters reach 0:
   - Free the data buffer
   - Free the pipe struct
4. Free the File struct (or let VFS handle it)

---

## Student Contribution Opportunities

These are good places for students to write code themselves:

| Code section | Complexity | Why it's educational |
|--------------|------------|---------------------|
| `struct Pipe` definition | Low | Designing data structures |
| Circular buffer logic | Medium | Classic data structure implementation |
| `Pipe_Read()` implementation | Medium | Understanding non-blocking semantics |
| `Pipe_Write()` implementation | Medium | Symmetric to read, reinforces pattern |
| `Pipe_Close()` cleanup | Medium | Resource management, multiple conditions |

**Note**: Ask student preference before assuming they want to code.

---

## Understanding Checks

Questions to verify comprehension (not a quiz - use conversationally):

### Before implementation

- "What's the difference between a file descriptor (integer) and a struct File?"
- "Why do we need separate reader and writer counts in the Pipe?"
- "What should `Read()` return if the pipe is empty but writers still exist?"
- "How does `Pipe_Read()` get called when user code calls `read()`?"

### After implementation

- "Walk me through what happens when someone creates a pipe"
- "What happens if I write 100 bytes then read 50?"
- "What happens if the writer closes their end while there's still data?"
- "Why do we check `writers` count in `Pipe_Read()`?"

---

## Testing

**Available tests**:
- `pipe-p1` - Basic pipe creation and read/write
- `pipe-p2` - More pipe operations
- `pipe-p4` - Additional edge cases

**Testing checkpoint**: After implementing `Pipe_Create()` and `Sys_Pipe()`:
- Run `pipe-p1` - it should pass the first assertion (pipe creation works)

**What tests DON'T cover** (for thinking about correctness):
- Multiple pipes simultaneously
- Large data transfers exceeding buffer size
- Repeated open/close cycles (memory leaks)

**How to run**:
```bash
# In QEMU shell after booting:
/c/pipe-p1.exe
```

---

## Common Mistakes and Debugging

**Typical errors**:

| Symptom | Likely cause | How to investigate |
|---------|--------------|-------------------|
| Kernel crash on pipe create | NULL pointer, bad malloc | Add Print() after each allocation |
| Data corruption | Buffer index errors | Print read/write positions |
| Wrong data returned | Circular buffer wrap-around bug | Test with buffer-sized writes |
| Memory leak | Not freeing on close | Add Print() in Pipe_Close |
| EWOULDBLOCK when data exists | Wrong condition in Pipe_Read | Print buffer state |

**Debugging strategies for this component**:

- Add `Print("Pipe_Create: pipe=%p\n", pipe);` to trace allocation
- Print buffer state: `Print("Read: pos=%d, avail=%d\n", readPos, available);`
- Trace the full path: syscall → VFS → Pipe_Read

---

## VFS Dispatch Pattern

This is worth understanding deeply - it's how GeekOS (and real OS kernels) achieve polymorphism in C:

```c
// In vfs.c, the Read() function does:
int Read(struct File *file, void *buf, ulong_t len) {
    // Check file has a Read operation
    if (file->ops->Read == NULL)
        return EACCESS;
    // Dispatch to the specific implementation
    return file->ops->Read(file, buf, len);
}
```

The `file->ops` pointer is set when the file is created:
- For pipes: points to `Pipe_Read_Ops` or `Pipe_Write_Ops`
- For disk files: points to filesystem-specific ops
- For devices: points to device-specific ops

This is the C equivalent of virtual methods in object-oriented languages.

---

## Connections

**Relates to concepts from**:

- File systems and the file abstraction
- Producer-consumer problem (simplified here - non-blocking)
- Resource management and cleanup
- The Unix "everything is a file" philosophy

**Will be used by**:

- Fork (child inherits pipe file descriptors)
- Shell pipelines (`cmd1 | cmd2`)
- Inter-process communication

---

## Learning Opportunities

These are teachable moments likely to arise during this component. When you encounter one, consider whether to engage the student based on their familiarity and involvement preferences.

**Format**: Each opportunity is phrased as a question to help identify the moment, without giving away the answer. Tags indicate severity and topic.

---

### Buffer Design

**[major] [design] [performance]**
Conceptually, how should the buffer for the pipe work? We could always have the next byte to be read at position 0, but would that be efficient?

**[major] [design]**
Once we've decided on the conceptual design of the buffer, what fields/state should we use to track the current state of the buffer?

**[minor] [design]**
How should we handle the cases where there isn't enough data/space in the buffer to handle a specific call to read or write?

**[minor] [performance]**
Is it OK to copy bytes one at a time, or is there something more efficient we could be doing? Would that code be more complicated, and would the complexity be worth it?

---

### Resource Management

**[major] [memory] [design]**
In `Pipe_Create`, we need to allocate several things (pipe struct, buffer, two File structs). If one allocation fails partway through, what should we do with the allocations that succeeded?

**[major] [memory]**
In `Pipe_Close`, how do we know when it's safe to free the pipe and its buffer? What if only one end has been closed?

**[minor] [design]**
Why do we need separate `readers` and `writers` counts instead of just a single reference count?

---

### VFS Integration

**[major] [design-patterns]**
Why are there two different `File_Ops` structs (`Pipe_Read_Ops` and `Pipe_Write_Ops`) instead of one? What problem does this solve?

**[minor] [design]**
The `fsData` field is `void*`. What does that allow us to do? What are the tradeoffs of using `void*`?

---

### Edge Cases

**[minor] [debugging]**
What should `Pipe_Read` return if the buffer is empty but writers still exist? What if there are no writers?

**[minor] [debugging]**
What should `Pipe_Write` do if there are no readers? Why is this different from a full buffer?

---

## Notes for the Tutor

**Good entry questions**:
- "Have you used pipes in shell commands before? Like `ls | grep`?"
- "What do you think a 'file descriptor' is?"

**Common misconceptions**:
- Students may not realize both File structs point to the SAME Pipe
- The `fsData` field being `void*` can be confusing - it's just a generic pointer
- Students sometimes try to call `Pipe_Read` directly instead of going through VFS

**"How does Pipe_Read get called?"** - This often confuses students:
1. User calls `read(fd, buf, len)` in their program
2. libc's `read()` makes syscall → `Sys_Read()`
3. `Sys_Read()` gets File from fd table, calls `Read(file, ...)`
4. VFS `Read()` calls `file->ops->Read(file, ...)`
5. Since `ops` is `Pipe_Read_Ops`, this dispatches to `Pipe_Read()`

Trace this path with the student using Print statements if they're confused.

**Copy_To_User confusion**:
- Students may try to copy directly to user pointers
- Explain: kernel and user have separate address spaces
- The user's pointer `0x1000` isn't the kernel's `0x1000`

**If student is stuck on circular buffer**:
- Draw it out on paper
- Start with a simple linear buffer, optimize later
- The 32K fixed buffer means wrap-around is relatively rare
