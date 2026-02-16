# Operating Systems Concepts Reference

This document provides a brief overview of key OS concepts relevant to GeekOS projects, with links to the course textbook **Operating Systems: Three Easy Pieces (OSTEP)** for deeper reading.

**Textbook**: https://pages.cs.wisc.edu/~remzi/OSTEP/

---

## Virtualization: CPU

### Processes and Threads

A **process** is an executing instance of a program, consisting of:
- **Address space**: code (text), data, heap, and stack segments
- **Execution state**: registers, program counter, stack pointer
- **Resources**: open files, sockets, etc.

A **thread** is an independent execution context within a process. Multiple threads share the process's address space but have their own stacks and register state.

**OSTEP Reading**:
- [Processes](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-intro.pdf) - What is a process?
- [Process API](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-api.pdf) - fork(), exec(), wait()
- [Direct Execution](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-mechanisms.pdf) - How the OS runs processes

### CPU Scheduling

The OS scheduler decides which process/thread runs on the CPU. Key concepts:
- **Ready queue**: processes waiting for CPU time
- **Context switch**: saving/restoring process state
- **Preemption**: forcibly taking CPU from a running process

**OSTEP Reading**:
- [CPU Scheduling](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-sched.pdf) - Basic scheduling algorithms
- [Multi-level Feedback Queue](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-sched-mlfq.pdf) - Priority-based scheduling
- [Multi-CPU Scheduling](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-sched-multi.pdf) - SMP considerations

---

## Virtualization: Memory

### Address Spaces

Each process has a **virtual address space** that the OS maps to physical memory. This provides:
- **Isolation**: processes cannot access each other's memory
- **Abstraction**: processes see a contiguous address space

**OSTEP Reading**:
- [Address Spaces](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-intro.pdf) - Virtual memory concepts
- [Memory API](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-api.pdf) - malloc(), free(), memory errors

### Segmentation and Paging

Two approaches to memory virtualization:
- **Segmentation**: divide address space into variable-sized segments (code, data, stack)
- **Paging**: divide address space into fixed-size pages

GeekOS uses segmentation for early projects and adds paging in Project 4.

**OSTEP Reading**:
- [Segmentation](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-segmentation.pdf) - Segment-based addressing
- [Introduction to Paging](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-paging.pdf) - Page tables, TLBs
- [Swapping: Mechanisms](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-beyondphys.pdf) - Paging to disk

---

## Concurrency

### Synchronization Primitives

When multiple threads share data, synchronization is required:
- **Locks/Mutexes**: ensure mutual exclusion
- **Condition Variables**: allow threads to wait for conditions
- **Semaphores**: generalized synchronization counters

**OSTEP Reading**:
- [Concurrency and Threads](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-intro.pdf) - Why threads?
- [Locks](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf) - Implementing mutual exclusion
- [Condition Variables](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf) - Waiting and signaling
- [Semaphores](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-sema.pdf) - Dijkstra's semaphores

### Concurrency Bugs

Common pitfalls in concurrent programming:
- **Deadlock**: circular waiting on resources
- **Race conditions**: outcome depends on timing
- **Priority inversion**: low-priority thread blocks high-priority

**OSTEP Reading**:
- [Concurrency Bugs](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-bugs.pdf) - Common issues and debugging

---

## Persistence: File Systems

### Files and Directories

The file system provides:
- **Persistent storage**: data survives reboots
- **Naming**: hierarchical directory structure
- **Access control**: permissions and ownership

**OSTEP Reading**:
- [Files and Directories](https://pages.cs.wisc.edu/~remzi/OSTEP/file-intro.pdf) - The file system interface
- [File System Implementation](https://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf) - How file systems work

### I/O Devices

The OS interacts with hardware through:
- **I/O ports**: CPU instructions to read/write device registers
- **Interrupts**: devices signal completion asynchronously
- **DMA**: direct memory access for bulk transfers

**OSTEP Reading**:
- [I/O Devices](https://pages.cs.wisc.edu/~remzi/OSTEP/file-devices.pdf) - Device interaction
- [Hard Disk Drives](https://pages.cs.wisc.edu/~remzi/OSTEP/file-disks.pdf) - Disk structure and scheduling

---

## Inter-Process Communication

### Pipes

Pipes provide a unidirectional byte stream between processes:
- Created with `pipe()` system call
- One end for reading, one for writing
- Commonly used with `fork()` for parent-child communication

### Signals

Signals are software interrupts delivered to processes:
- Asynchronous notification mechanism
- Each signal has a default action (terminate, ignore, etc.)
- Processes can register custom signal handlers

**OSTEP Reading**:
- [Process API](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-api.pdf) - Covers fork, exec, and basics of IPC

---

## Hardware Concepts (x86)

### CPU Modes

x86 processors have multiple privilege levels:
- **Ring 0 (Kernel mode)**: full hardware access, runs OS code
- **Ring 3 (User mode)**: restricted access, runs application code

Mode transitions occur via:
- **System calls**: user to kernel (intentional)
- **Interrupts/Exceptions**: user to kernel (hardware/software events)
- **Return from interrupt**: kernel to user

### Interrupts

Interrupts transfer control to the OS:
- **Hardware interrupts**: external events (timer, keyboard, disk)
- **Software interrupts/traps**: intentional (system calls)
- **Exceptions**: errors (page fault, divide by zero)

The **Interrupt Descriptor Table (IDT)** maps interrupt numbers to handler addresses.

---

## Further Reading

The complete OSTEP textbook is freely available at:
https://pages.cs.wisc.edu/~remzi/OSTEP/

Key chapters for each GeekOS project:

| Project | Relevant OSTEP Chapters |
|---------|------------------------|
| P1 (Fork/Pipe) | cpu-api, cpu-intro |
| P2 (Signals) | cpu-api (signals section) |
| P3 (Semaphores/Scheduling) | threads-sema, cpu-sched, cpu-sched-mlfq |
| P4 (Virtual Memory) | vm-paging, vm-tlbs, vm-beyondphys |
| P5 (File System) | file-intro, file-implementation |
