# Concurrency and Synchronization in GeekOS

This document serves as an index to concurrency concepts covered across GeekOS projects. Each topic links to the project document where it was introduced and where you developed your understanding.

**How this works:**
- Each project introduces concurrency concepts relevant to that project
- You work through questions and fill in your understanding in the project-specific document
- This index grows as you progress through projects, providing a unified reference
- After each project, you receive a supplement to compare your answers

---

## Quick Reference by Topic

### Foundational Concepts

| Topic | Introduced In | Section |
|-------|---------------|---------|
| Why synchronization matters (SMP) | [p1/concurrency.md](p1/concurrency.md) | Section 1 |
| Race conditions | [p1/concurrency.md](p1/concurrency.md) | Section 1 |

### Synchronization Primitives

| Topic | Introduced In | Section |
|-------|---------------|---------|
| Mutexes | [p1/concurrency.md](p1/concurrency.md) | Section 2 |
| Condition Variables | [p1/concurrency.md](p1/concurrency.md) | Section 3 |
| `while` vs `if` with Cond_Wait | [p1/concurrency.md](p1/concurrency.md) | Section 3 |
| Signal vs Broadcast | [p1/concurrency.md](p1/concurrency.md) | Section 3 |
| Atomic Operations | [p1/concurrency.md](p1/concurrency.md) | Section 5 |

### Patterns

| Topic | Introduced In | Section |
|-------|---------------|---------|
| Producer-Consumer | [p1/concurrency.md](p1/concurrency.md) | Section 4 |
| Reference Counting | [p1/concurrency.md](p1/concurrency.md) | Section 5 |

### Kernel Internals

| Topic | Introduced In | Section |
|-------|---------------|---------|
| Spinlocks | `include/geekos/lock.h`, `src/geekos/smp.c` | See code comments |
| `__atomic` builtins | `include/geekos/atomic.h` | See code comments |
| Interrupt disabling + Spinlocks | `Spin_Lock_Irq_Save()` in `lock.h` | See code comments |
| Interrupt disabling | `include/geekos/int.h` | `Save_And_Disable_Interrupts()` |
| Lock ordering | *Coming in a later project* | — |

---

## By Project

### Project 1: Fork and Pipes
**Document**: [p1/concurrency.md](p1/concurrency.md)
**Supplement**: [concurrency-supplement-p1.md](concurrency-supplement-p1.md) *(available after P1 completion)*

**Topics covered:**
1. Why Synchronization Matters — understanding SMP and race conditions
2. Mutexes — blocking locks for protecting shared state
3. Condition Variables — waiting for conditions, the `while` rule, Signal vs Broadcast
4. Producer-Consumer Pattern — the foundation of pipe implementation
5. Atomic Operations — reference counting for shared file descriptors

---

## OSTEP Reading Guide

| Chapter | Relevant To |
|---------|-------------|
| [Chapter 28: Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf) | P1: Sections 1, 2, 5 |
| [Chapter 30: Condition Variables](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf) | P1: Sections 3, 4 |
| [Chapter 32: Concurrency Bugs](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-bugs.pdf) | P1: Section 1 |

---

## Document History

| Version | Project | Topics Added |
|---------|---------|--------------|
| 1.0 | P1 | Mutexes, Condition Variables, Producer-Consumer, Atomic Ops |

*This index is updated at the start of each project to include new topics.*
