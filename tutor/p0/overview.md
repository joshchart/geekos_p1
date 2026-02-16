# Project 0: Pipes - Learning Path

**Goal**: Implement the `Pipe()` system call and related operations (`Read()`, `Write()`, `Close()`), creating a unidirectional communication channel between file descriptors.

**Time estimate**: 4-6 hours across multiple sessions

---

## Project Context

This project is designed to familiarize you with the GeekOS development environment and VFS (Virtual File System) layer. You'll implement anonymous pipes - a fundamental Unix IPC (Inter-Process Communication) mechanism.

Key concepts you'll learn:
- **File descriptors**: Integer handles that reference open files/pipes
- **VFS layer**: How GeekOS abstracts different file types with common operations
- **struct File and File_Ops**: The vtable pattern for polymorphic file operations
- **Kernel/user boundary**: Using `Copy_To_User` and `Copy_From_User`

This project uses **non-blocking I/O** - reads and writes return immediately rather than waiting. This simplifies the implementation (no condition variables needed) while still teaching the core concepts.

---

## Component Order

This project has a single main component with logical sub-parts:

| Order | Component | What you'll learn |
|-------|-----------|-------------------|
| 1 | [Pipes](component-pipes.md) | VFS layer, file descriptors, kernel data structures |

### Sub-parts within Pipes

Work through these in order:

1. **struct Pipe design** - Designing kernel data structures
2. **Pipe_Create / Sys_Pipe** - Creating pipes and file descriptors
3. **Pipe_Read** - Reading from the pipe buffer
4. **Pipe_Write** - Writing to the pipe buffer
5. **Pipe_Close** - Cleanup and resource management

---

## Key Files Overview

These are the main files you'll work with:

| File | Purpose |
|------|---------|
| `include/geekos/vfs.h` | `struct File` and `struct File_Ops` definitions |
| `src/geekos/vfs.c` | VFS layer - trace `Sys_Read`, `Sys_Write`, `Sys_Close` |
| `src/geekos/pipe.c` | Your pipe implementation goes here |
| `include/geekos/pipe.h` | Pipe function declarations |
| `src/geekos/syscall.c` | System call handlers including `Sys_Pipe` |
| `src/libc/fileio.c` | User-space wrappers that invoke system calls |

---

## Before You Start

Make sure you can:

1. **Build GeekOS**: `cd build && make`
2. **Run GeekOS**: `make run` (exit with Ctrl-A then X)
3. **Understand the shell**: GeekOS boots to `/c/shell.exe`

Recommended reading:
- `docs/p0/project0.md` - Official project specification
- `docs/p0/project0_slides.md` - Lecture slides with implementation hints
- `docs/geekos-architecture.md` - VFS layer documentation

---

## Key Semantics (Non-Blocking)

**Important**: Project 0 pipes are **non-blocking**. This is different from traditional Unix pipes:

| Operation | Condition | Return Value |
|-----------|-----------|--------------|
| `Read()` | Writers exist, no data | `EWOULDBLOCK` |
| `Read()` | Data available | Number of bytes read (may be less than requested) |
| `Read()` | No writers, no data | `0` (EOF) |
| `Write()` | No readers | `EPIPE` |
| `Write()` | Space available | Number of bytes written |
| `Write()` | Fixed buffer full | `0` |
| `Write()` | Dynamic buffer, Malloc fails | `ENOMEM` |

---

## Tests

| Test | What it checks |
|------|---------------|
| `pipe-p1` | Basic pipe creation and simple read/write |
| `pipe-p2` | More complex pipe operations |
| `pipe-p4` | Additional edge cases |

Run tests from the shell inside GeekOS:
```
/c/pipe-p1.exe
/c/pipe-p2.exe
```

---

## Looking Ahead

After completing Project 0, you'll have:
- Working pipe infrastructure
- Understanding of the VFS layer
- Experience with GeekOS kernel development

In **Project 1**, you'll build on this by:
- Adding reference counting to `struct File` (for sharing after fork)
- Implementing `Fork()` and `Exec()` system calls
- Making pipes work across parent/child processes

The reference counting added in P1 will let parent and child processes share the same pipe - when one closes their end, the pipe stays open for the other.

---

## Getting Help

When you're stuck:

1. **Trace the code path** - Follow a system call from user space through the kernel
2. **Use Print() statements** - Add debug output at key points
3. **Read the spec carefully** - P0's non-blocking semantics differ from traditional pipes
4. **Form a hypothesis** - "I think the bug is because X"

The goal is understanding the VFS architecture, not just passing tests.

---

## Reference Documentation

- `docs/p0/project0.md` - Full project specification
- `docs/p0/project0_slides.md` - Lecture slides with implementation hints
- `docs/geekos-architecture.md` - Overall system architecture
