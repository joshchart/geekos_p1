# Project 1: Fork and Exec - Learning Path

**Goal**: Implement `Fork()` and `Execl()` system calls, enabling process creation and program execution with proper file descriptor sharing.

**Time estimate**: 6-10 hours across multiple sessions

**Prerequisites**: Project 0 (Pipes) must be completed first

---

## Project Context

This project teaches core Unix process model concepts:

- **Fork**: Creates a child process as a copy of the parent
- **Execl**: Replaces a process's code with a new program
- **File sharing**: Parent and child share open files (not copies!)
- **Reference counting**: Tracking file usage so resources are freed correctly

By the end, you'll understand how shells work: when you type `ls | grep foo`, the shell forks twice, sets up a pipe (from P0!), and execs the programs.

---

## Component Order

Work through these components in order. Each builds on the previous:

| Order | Component | What you'll learn |
|-------|-----------|-------------------|
| 1 | [Reference Counting](component-refcount.md) | Why and how files are shared |
| 2 | [Fork](component-fork.md) | Process creation, copying state |
| 3 | [Blocking Pipes](component-blocking-pipe.md) | Concurrent-safe pipes with mutexes/condvars |
| 4 | [Execl](component-execl.md) | Loading programs, setting up execution |

**Prerequisites from Project 0**:
- Working pipe implementation (Pipe_Create, Pipe_Read, Pipe_Write, Pipe_Close)
- Understanding of the VFS layer and File_Ops dispatch

**Why this order?**

- **Reference counting** is the simplest and provides infrastructure for sharing files across fork
- **Fork** is the main event - creating child processes that can run concurrently
- **Blocking pipes** makes pipes safe for concurrent access (fork creates the concurrency)
- **Execl** builds on fork to complete the process creation story

**How to approach each component**:

1. **Read the component guide** - Understand requirements and design considerations
2. **Plan collaboratively** - Work with Claude to develop a design (recorded in `plan.md`)
3. **Implement** - Refer back to component guide for detailed patterns and code examples
4. **Test** - Run tests and debug together
5. **Reflect** - Record significant learnings in `experiences.md`

---

## Key Files Overview

These are the main files you'll work with:

| File | Purpose |
|------|---------|
| `include/geekos/vfs.h` | `struct File` definition (add refCount) |
| `src/geekos/vfs.c` | File operations including `Close()` |
| `src/geekos/syscall.c` | System call handlers (`Sys_Fork`, `Sys_Execl`) |
| `src/geekos/user.c` | User process management |
| `src/geekos/userseg.c` | Address space operations |
| `include/geekos/kthread.h` | Process/thread structures |

**Note**: `src/geekos/pipe.c` was implemented in Project 0. You may need to verify it still works with the reference counting changes.

---

## Before You Start

Make sure you can:

1. **Build GeekOS**: `cd build && make`
2. **Run GeekOS**: `make run` (exit with Ctrl-A then X)
3. **Understand the shell**: GeekOS boots to `/c/shell.exe`
4. **Verify P0 works**: Run `/c/pipe-p1.exe` to confirm pipes work

Recommended reading:
- OSTEP Chapter 5: Process API (fork, exec, wait)
- OSTEP Chapter 39: Files and Directories
- `docs/p1/project1.md` - Official project specification
- `docs/p1/project1_slides.md` - Lecture slides with hints

---

## Tests

Each component has associated tests:

| Test | What it checks |
|------|---------------|
| `fork-p1` | Basic fork functionality |
| `forkpipe` | Pipe inheritance across fork (uses P0 pipes!) |
| `forkexec` | Execl with inherited file descriptors |
| `forkbomb` | Resource exhaustion handling |

Run tests from the shell inside GeekOS:
```
/c/fork-p1.exe
/c/forkpipe.exe
```

---

## Connection to Project 0

Project 1 builds directly on Project 0:

| P0 Component | P1 Usage |
|--------------|----------|
| `struct Pipe` | Still used; no changes needed |
| `struct File` | **Modified**: Add `refCount` field |
| `Pipe_Close()` | Now may be called multiple times if file is shared |
| `forkpipe` test | Tests that pipes work across fork |

**Key change**: In P0, each File had exactly one owner. In P1, Files can be shared via fork, so we add reference counting to track ownership.

---

## Getting Help

When you're stuck:

1. **Re-read the relevant code** - Often the answer is in existing patterns
2. **Form a hypothesis** - "I think the bug is because X"
3. **Test your hypothesis** - Add Print() statements, try edge cases
4. **Ask questions** - But try to articulate what you've tried

The goal is understanding, not just passing tests. If you're unsure *why* something works, that's worth exploring.

---

## Reference Documentation

- `docs/p1/project1.md` - Full project specification
- `docs/p1/project1_slides.md` - Lecture slides with implementation hints
- `docs/geekos-architecture.md` - Overall system architecture
- `docs/os-concepts.md` - OS concepts with OSTEP chapter links
