# GeekOS Architecture Reference

This document provides a comprehensive overview of GeekOS internals, covering hardware abstraction, kernel components, and key data structures.

**Related Reading**: See [os-concepts.md](os-concepts.md) for general OS concepts with OSTEP textbook links.

---

## Table of Contents

1. [Hardware and Devices](#1-hardware-and-devices)
2. [Booting and Kernel Initialization](#2-booting-and-kernel-initialization)
3. [Kernel Threads](#3-kernel-threads)
4. [User Processes](#4-user-processes)
5. [Synchronization](#5-synchronization)
6. [Scheduling](#6-scheduling)
7. [Interrupt Handling](#7-interrupt-handling)
8. [Virtual Filesystem](#8-virtual-filesystem)
9. [PFAT Filesystem](#9-pfat-filesystem)
10. [Block Devices](#10-block-devices)
11. [Buffer Cache](#11-buffer-cache)

---

## 1. Hardware and Devices

### Hardware Configuration

GeekOS runs on an emulated x86 SMP (Symmetric Multi-Processing) system:

```
+-------+  +-------+  +-------+  +-------+
|  CPU  |  |  CPU  |  |  CPU  |  |  CPU  |  +-----+
| Local |  | Local |  | Local |  | Local |  | DMA |
| APIC  |  | APIC  |  | APIC  |  | APIC  |  +--+--+
+---+---+  +---+---+  +---+---+  +---+---+     |
    |          |          |          |         |
----+----------+----------+----------+---------+---- System Bus
    |          |          |          |         |
+---+---+  +---+---+  +---+---+  +---+---+  +--+--+
|  RAM  |  |  VGA  |  |IO APIC|  | Timer |  | IDE |
|       |  |Monitor|  |       |  |Keybrd |  +-----+
+-------+  +-------+  +-------+  +-------+  |diskc|
                                            |diskd|
                                            +-----+
```

**Key Components**:
- **x86 CPUs**: SMP configuration with Local APICs
- **Local APIC**: Receives interrupts from IO-APIC, handles inter-processor interrupts
- **IO-APIC**: Routes interrupts from I/O devices to Local APICs
- **diskc**: Boot disk with PFAT filesystem containing kernel and user programs
- **diskd**: Secondary disk for filesystem projects
- Emulated by **QEMU**

### x86 CPU Modes

| Mode | Description |
|------|-------------|
| **Real Mode** | 16-bit (8086 compatible), 1MB address space, entered at power-up |
| **Protected Mode** | 32-bit, 4GB address space, privilege levels, **GeekOS runs here** |

**Protected Mode Features**:
- 4 privilege levels: Ring 0 (kernel) through Ring 3 (user)
- Segmentation with optional paging
- 256 interrupt vectors

### x86 Registers

**General Purpose (32-bit)**:
- `eax`, `ebx`, `ecx`, `edx`, `esi`, `edi`

**Stack/Frame Pointers**:
- `esp`: Stack pointer
- `ebp`: Frame pointer

**Instruction Pointer**:
- `eip`: Current instruction address

**Segment Registers (16-bit selectors)**:
- `cs`: Code segment
- `ss`: Stack segment
- `ds`, `es`, `fs`, `gs`: Data segments

**System Registers**:
- `gdtr`: Global Descriptor Table register (48-bit)
- `idtr`: Interrupt Descriptor Table register (48-bit)
- `ldtr`: Local Descriptor Table register (16-bit selector)
- `tr`: Task State Segment register (16-bit selector)
- `eflags`: Status flags (carry, zero, interrupt enable, etc.)
- `cr0-cr4`: Control registers (paging enable, etc.)

### x86 Addressing

- **Address space**: 4GB (32-bit)
- **Segment**: Contiguous chunk of address space
- Address = Segment Selector (16-bit) + Offset (32-bit)
- Segment selector indexes into **Global Descriptor Table (GDT)** or **Local Descriptor Table (LDT)**
- Descriptor contains: base address, limit, privilege level, type

### x86 Interrupts

- 256 interrupt vectors: 0-31 hardware reserved, 32-255 available
- Interrupt indexes into **Interrupt Descriptor Table (IDT)**
- Each entry is an **interrupt gate** containing handler address

**Interrupt Handling**:
- If handler privilege = CPU privilege: push `eflags`, `cs`, `eip`, (error code)
- If handler privilege < CPU privilege (kernel handles user interrupt):
  - Switch to kernel stack (from TSS)
  - Push `ss`, `esp` (user stack info)
  - Push `eflags`, `cs`, `eip`, (error code)
- `IRET` instruction reverses this process

### Device Drivers

#### Timer (LAPIC)

Each CPU has a Local APIC timer (interrupt 32):

```c
Timer_Interrupt_Handler(istate):
    id = Get_CPU_ID()
    ct = get_current_thread()
    if id == 0:
        ++g_numTicks                    // Global tick counter
    ++ct.numTicks
    if ct.numTicks >= g_Quantum:
        g_needReschedule[id] = true
```

**Key Variables**:
- `g_numTicks`: Global tick counter
- `g_Quantum`: Time slice (default: 4 ticks)

#### Keyboard

- **Ports**: `KB_DATA` (0x60), `KB_CMD` (0x64)
- **Interrupt**: IRQ 1

```c
Keyboard_Interrupt_Handler(istate):
    if byte available:
        convert scancode to keycode
        add to s_queue
        wakeup(s_keyboardWaitQueue)

Wait_For_Key():
    disable interrupts
    while s_queue empty:
        wait(s_keyboardWaitQueue)
    return keycode
```

#### VGA Screen

- **Video memory**: `0xB8000` (25 rows x 80 columns)
- **Ports**: `0x3D4`, `0x3D5` for cursor control

#### IDE Disk

- 2 hard disks, 256-byte blocks
- PIO mode (programmed I/O)

```c
IDE_Read(drive, blocknum, buffer):
    convert blocknum to cylinder/head/sector
    write to control registers
    read 256 words from data register

IDE_Request_Thread():
    forever:
        req = dequeue from request queue    // blocking
        IDE_Read/Write(req)                 // synchronous
```

---

## 2. Booting and Kernel Initialization

### Boot Sequence

1. **BIOS**: Configures one CPU as primary (id 0), loads boot sector from diskc block 0
2. **bootsect.asm**: Loads kernel image into memory, jumps to setup.asm
3. **setup.asm**:
   - Gets memory size
   - Enters protected mode
   - Sets segment registers to `KERNEL_CS`/`KERNEL_DS`
   - Jumps to `Main()` in main.c

### Main() Initialization (CPU 0)

**Phase 1**:
- Initialize GDT, GDTR
- Organize memory into 4K pages (`g_pageList`, `s_freeList`)
- Initialize kernel heap
- Initialize TSS
- Initialize IDT (syscall entry at user privilege level)
- Start secondary CPUs (SMP)

**Phase 2**:
- Initialize scheduler: create Main, Idle, and Reaper threads
- Initialize trap handlers (stack exception, GPF, syscall)
- Initialize devices (APIC, keyboard, IDE, DMA)
- Initialize PFAT filesystem
- Release secondary CPUs from spin-wait
- Mount root filesystem (`/c` on ide0)
- Spawn initial process (`/c/shell.exe`)

### Secondary CPU Initialization

```c
start_secondary_cpu (setup.asm):
    enter protected mode
    set segment registers
    set esp to CPUs[i].stack
    jump to Secondary_Start()

Secondary_Start() (smp.c):
    init GDT (share CPU 0's GDT)
    init TSS for this CPU
    init IDT for this CPU
    init scheduler (create Main and Idle threads)
    init traps and LAPIC
    signal CPU 0 that initialization is complete
    Exit(0)  // enters scheduler
```

---

## 3. Kernel Threads

### Thread Context

A kernel thread consists of:
- `struct Kernel_Thread`: Thread control block
- Stack page (4KB)

### struct Kernel_Thread

Key fields:
- `esp`: Current stack pointer
- `stackPage`: Pointer to stack memory
- `userContext`: Pointer to User_Context (NULL for pure kernel threads)
- `numTicks`, `totalTime`: Scheduling statistics
- `priority`: Thread priority
- `pid`: Process ID
- `exitCode`: Exit status
- `owner`: Parent thread
- `joinQueue`: Threads waiting to join this one
- `alive`, `detached`, `refCount`: Lifecycle management

### Thread Queues

| Queue | Purpose |
|-------|---------|
| `s_allThreadList` | All threads in the system |
| `s_runQueue` | Ready/runnable threads |
| `s_graveyardQueue` | Terminated threads awaiting cleanup |
| Various `waitQueues` | Threads blocked on mutexes, conditions, I/O |
| `g_currentThreads[MAX_CPUS]` | Currently running thread per CPU |

### Creating Kernel Threads

```c
Start_Kernel_Thread(startfunc, arg, priority, detached, name):
    Create_Thread:
        allocate Kernel_Thread struct and stack page
        init fields: stackPage, esp, numTicks, pid
        add to s_allThreadList

    Setup_Kernel_Thread:
        configure stack for initial execution:
        // Stack (bottom to top):
        //   startfunc arg
        //   Shutdown_Thread address
        //   startfunc address
        //   0 (eflags), KERNEL_CS (cs), Launch_Thread (eip)
        //   fake error code, interrupt number
        //   fake general-purpose registers
        //   KERNEL_DS (ds, es), 0 (fs, gs)

    add thread to run queue
```

### Getting Current Thread

```c
CURRENT_THREAD:
    disable interrupts
    ct = g_currentThreads[Get_CPU_ID()]
    restore interrupts
    return ct
```

---

## 4. User Processes

### User Process Context

A user process adds to the kernel thread:
- `struct User_Context`: User address space and resources

### struct User_Context

Key fields:
- `name[]`: Process name
- `ldt[2]`: Local Descriptor Table (code segment, data segment)
- `ldtDescriptor`: Pointer to LDT descriptor in GDT
- `memory`, `size`: User address space
- `ldtSelector`, `csSelector`, `dsSelector`: Segment selectors
- `entryAddr`: Program entry point
- `argBlockAddr`: Command-line arguments location
- `stackPointerAddr`: Initial stack pointer
- `file_descriptor_table[]`: Open files

### Spawning a User Process

```c
Spawn(program, cmd, kthread, background):
    read executable from filesystem (VFS)
    parse ELF header, extract segment info

    mem = malloc(program size + argblock + stack)
    copy program segments into mem

    create User_Context:
        memory = mem
        setup LDT entries
        set entry point, argblock, stack addresses

    kthread = Start_User_Thread(userContext)
```

### Starting a User Thread

```c
Start_User_Thread(uc, detached):
    Create_Thread:
        allocate Kernel_Thread and stack
        add to s_allThreadList

    Setup_User_Thread:
        kthread.userContext = uc
        configure kernel stack as if interrupted from user mode:
        // Stack (bottom to top):
        //   uc.ds (user ss), uc.stackaddr (user esp)
        //   eflags (interrupts enabled), uc.cs, uc.entryaddr
        //   fake error code, interrupt number
        //   general-purpose registers
        //   uc.ds (ds, es, fs, gs)

    add thread to run queue
```

### User/Kernel Memory Copy

```c
User_To_Kernel(userContext, userPtr):
    return userContext.memory + userPtr

Copy_From_User(dstInKernel, srcInUser, size):
    srcInKernel = User_To_Kernel(context, srcInUser)
    memcpy(dstInKernel, srcInKernel, size)

Copy_To_User(dstInUser, srcInKernel, size):
    dstInKernel = User_To_Kernel(context, dstInUser)
    memcpy(dstInKernel, srcInKernel, size)
```

---

## 5. Synchronization

### Interrupt Enable/Disable

```c
Disable_Interrupts():       // CLI instruction
Enable_Interrupts():        // STI instruction

// Linux equivalent: local_irq_save / local_irq_restore
Save_And_Disable_Interrupts():
    was_enabled = interrupts_enabled()
    if was_enabled:
        Disable_Interrupts()
    return was_enabled

Restore_Interrupt_State(was_enabled):
    if was_enabled:
        Enable_Interrupts()
```

**Warning - SMP**: Interrupt disable only affects the current CPU. On SMP systems, other CPUs continue running concurrently. Do NOT use `Save_And_Disable_Interrupts()` for protecting shared data like reference counts—use `Atomic_Increment()`/`Atomic_Decrement()` from `<geekos/atomic.h>` instead.

### Spinlocks

Used for short critical sections, especially in interrupt handlers. GeekOS spinlocks use GCC `__atomic` builtins for portability and clarity:

```c
// Pseudocode for Spin_Lock (see src/geekos/smp.c for actual implementation)
Spin_Lock(x):
    while atomic_exchange(&x->lock, 1, ACQUIRE) != 0:
        // Lock was held - spin-wait without writing (reduces cache traffic)
        while atomic_load(&x->lock, RELAXED) != 0:
            pause()   // CPU hint: we're spin-waiting

Spin_Unlock(x):
    atomic_store(&x->lock, 0, RELEASE)
```

**Memory Ordering**:
- `ACQUIRE` on lock: ensures subsequent reads see values at least as recent
- `RELEASE` on unlock: ensures previous writes are visible before unlock
- `RELAXED` for spin-wait: no ordering needed, just checking

**Important**: Always disable interrupts before acquiring a spinlock to prevent deadlock. Use the combined variant for convenience:

```c
bool iflag = Spin_Lock_Irq_Save(&lock);
// ... critical section ...
Spin_Unlock_Irq_Restore(&lock, iflag);
```

**Common Spinlocks**:
- `kthreadLock`: Protects thread lists
- `run_queue_spinlock`: Protects run queue
- `mutex->guard`: Protects mutex state
- List locks (every `DEFINE_LIST` creates a spinlock)

**Related Headers**:
- `include/geekos/lock.h`: Spinlock types and functions
- `include/geekos/atomic.h`: Atomic operations for reference counting

### Wait and Wakeup

```c
Wait(waitq):
    disable interrupts
    Spin_Lock(waitq.lock)
    add current thread to waitq
    Schedule_And_Unlock(waitq.lock)
    restore interrupts

Wake_Up(waitq):
    disable interrupts
    Spin_Lock(waitq.lock)
    move all threads from waitq to run queue
    Spin_Unlock(waitq.lock)
    restore interrupts

Wake_Up_One(waitq):
    if waitq not empty:
        move first thread to run queue
```

### Mutex

```c
struct Mutex { state, guard (spinlock), owner, waitq }

Mutex_Lock(x):
    disable interrupts
    Spin_Lock(x.guard)
    if x.state is locked:
        add current thread to x.waitq
        Schedule_And_Unlock(x.guard)
    else:
        x.state = locked
        Spin_Unlock(x.guard)
    x.owner = current thread
    restore interrupts

Mutex_Unlock(x):
    disable interrupts
    Spin_Lock(x.guard)
    if x.waitq not empty:
        x.owner = waitq.front
        wakeup waitq.front
    else:
        x.state = unlocked
    Spin_Unlock(x.guard)
    restore interrupts
```

### Condition Variable

```c
struct Condition { waitq }

Cond_Wait(cv, mutex):
    disable interrupts
    Spin_Lock(mutex.guard)
    add current thread to cv.waitq
    // Release mutex without restoring interrupts
    Mutex_Unlock_Internal(mutex)
    Schedule_And_Unlock(mutex.guard)
    restore interrupts
    Mutex_Lock(mutex)    // Re-acquire mutex

Cond_Signal(cv):
    wakeup cv.waitq.front

Cond_Broadcast(cv):
    wakeup all in cv.waitq
```

---

## 6. Scheduling

### Scheduling Flags

Checked at every potential context switch:
- `g_preemptionDisabled[MAX_CPUS]`: Prevents preemption when true
- `g_needReschedule[MAX_CPUS]`: Set by timer interrupt when quantum expires

### Schedule()

```c
Schedule():
    // Called when thread voluntarily yields
    // Thread is already in run queue or a wait queue

    g_preemptionDisabled[this_cpu] = false
    next = remove thread from run queue
    Switch_To_Thread(next)

Schedule_And_Unlock(spinlock):
    // Same as Schedule() but releases spinlock first
    Spin_Unlock(spinlock)
    Schedule()
```

### Context Switch

```c
Switch_To_Thread(next):
    // Called from Schedule(), interrupts disabled
    // Current thread already saved in a queue

    save current state to interrupt frame on stack
    make next the current thread
    activate user context if present (update LDT, TSS)
    check for pending signals
    restore registers from next's stack
    IRET
```

---

## 7. Interrupt Handling

### Handle_Interrupt (lowlevel.asm)

```c
Handle_Interrupt():
    // Entered via interrupt gate, hardware pushed:
    //   (user ss/esp if from user mode)
    //   eflags, cs, eip, (error code)

    push general-purpose and segment registers
    call C interrupt handler with pointer to state

    if !g_preemptionDisabled && g_needReschedule:
        move current thread to run queue
        save current esp to thread struct
        get next thread from run queue
        make it current

    activate user context if present
    process pending signals if any
    restore registers
    IRET
```

---

## 8. Virtual Filesystem

### VFS Layer

The Virtual Filesystem provides a uniform interface to different filesystem implementations.

**Key Data Structures**:

```c
struct Filesystem {
    ops         // Format, Mount functions
    fsname      // e.g., "pfat", "gosfs"
}

struct Mount_Point {
    ops         // Open, Create_Directory, Stat, etc.
    pathpfx     // Mount path, e.g., "/c"
    dev         // Block device
    fsdata      // Filesystem-specific data
}

struct File {
    ops         // FStat, Read, Write, Close, etc.
    filepos     // Current position
    endpos      // File length
    fsdata      // Filesystem-specific data
    mode        // Access mode
    mountpoint  // Containing filesystem
}
```

**Static Variables**:
- `s_vfsLock`: Mutex protecting VFS structures
- `s_fileSystemList`: Registered filesystem types
- `s_mountPointList`: Mounted filesystems

### VFS Operations

```c
Register_Filesystem(fsname, ops):
    add to s_fileSystemList

Mount(devname, pathpfx, fstype):
    fs = find fstype in s_fileSystemList
    Open_Block_Device(devname)
    mp = create Mount_Point
    fs.ops.Mount(mp)
    add mp to s_mountPointList

Open(path, mode, file):
    split path into mountpoint prefix and suffix
    mp = find mountpoint
    mp.ops.Open(mp, path, mode, file)
```

---

## 9. PFAT Filesystem

PFAT is a simple FAT-like filesystem used for the boot disk.

### Data Structures

```c
struct PFAT_Instance {      // in Mount_Point.fsdata
    fsinfo          // Boot sector
    fat             // File Allocation Table
    rootDir         // Root directory entries
    lock            // Protects fileList
    fileList        // Open files
}

struct PFAT_File {          // in File.fsdata
    entry           // Directory entry
    numBlocks       // File size in blocks
    fileDataCache   // Cached file data
    lock            // Guards concurrent access
}

struct bootSector {
    magic
    fileAllocationOffset/Length
    rootDirectoryOffset/Count
    setupStart/Size
    kernelStart/Size
}

struct directoryEntry {
    flags           // readOnly, hidden, directory, etc.
    time, date
    firstBlock, fileSize
    acls
}
```

### PFAT Operations

```c
PFAT_Mount(mp):
    pfi = allocate PFAT_Instance
    pfi.fsinfo = read boot sector from block 0
    pfi.fat = read FAT blocks
    pfi.rootDir = read root directory
    PFAT_Register_Paging_File(mp, pfi)
    mp.ops = {PFAT_Open, PFAT_Open_Directory, ...}
    mp.fsdata = pfi

PFAT_Read(file, buf, nbytes):
    lock file
    traverse FAT to find file blocks
    read blocks into cache if needed
    copy from cache to buf
    update filepos
    unlock file
    return bytes read
```

---

## 10. Block Devices

### Data Structures

```c
struct Block_Request {
    dev             // Target device
    type            // BLOCK_READ or BLOCK_WRITE
    blocknum        // Block number
    state           // PENDING, COMPLETED, ERROR
    errorcode
    satisfied       // Condition variable
}

struct Block_Device {
    name
    ops             // Open, Close, Get_Num_Blocks
    unit
    inUse
    waitqueue
    reqqueue        // Pending requests
}
```

### Block I/O

```c
Block_Read(dev, blocknum, buf):
    lock device
    req = create Block_Request
    add req to dev.requestQueue
    signal(s_blockdevRequestCond)    // Wake server
    while req.state == PENDING:
        Cond_Wait(req.satisfied, lock)
    unlock device
    return req.errorcode

// Server thread (per device)
Device_Request_Thread():
    forever:
        req = dequeue(dev.requestQueue)  // blocking
        perform I/O
        Notify_Request_Completion(req)
```

---

## 11. Buffer Cache

The buffer cache reduces disk I/O by caching recently accessed blocks.

### Data Structures

```c
struct FS_Buffer {
    fsblocknum      // Block number (if in use)
    data            // 4K data page
    flags           // dirty, inuse
}

struct FS_Buffer_Cache {
    dev             // Block device
    fsblocksize     // Block size
    numCached       // Current buffer count
    bufferList      // LRU list
    mutex
    cond            // Wait for free buffer
}
```

### Buffer Cache Operations

```c
Get_FS_Buffer(cache, blocknum, buf):
    lock cache
    if blocknum in cache:
        wait until not in use
        mark in use
        return buffer

    if at max capacity and all in use:
        return ENOMEM

    if under capacity:
        allocate new buffer
    else:
        evict LRU buffer (sync if dirty)

    read block into buffer
    unlock cache
    return buffer

Sync_FS_Buffer_Cache(cache):
    for each dirty buffer:
        write to disk
        mark clean
```

---

## Directory Structure Reference

```
geekos/
+-- build/              # Build system (run make here)
+-- include/
|   +-- geekos/         # Kernel headers
|   +-- libc/           # User-space headers
+-- src/
|   +-- geekos/         # Kernel source
|   |   +-- main.c      # Kernel entry point
|   |   +-- kthread.c   # Thread management
|   |   +-- syscall.c   # System call handlers
|   |   +-- userseg.c   # User address space
|   |   +-- sched.c     # Scheduler
|   |   +-- synch.c     # Synchronization primitives
|   |   +-- mem.c       # Physical memory
|   |   +-- vfs.c       # Virtual filesystem
|   |   +-- pfat.c      # PFAT filesystem
|   |   +-- signal.c    # Signal handling
|   |   +-- pipe.c      # Pipe implementation
|   +-- user/           # User-space programs
|   +-- libc/           # User-space C library
|   +-- common/         # Shared code
```

---

## Project Feature Flags

Projects are enabled in `include/geekos/projects.h`:

| Flag | Project |
|------|---------|
| `PROJECT_FORK` | Process forking |
| `PROJECT_PIPE` | Pipe IPC |
| `PROJECT_BACKGROUND_JOBS` | Background job control |
| `PROJECT_SIGNALS` | Signal handling |
| `PROJECT_SEMAPHORES` | Semaphores |
| `PROJECT_SCHEDULING` | Custom scheduler |
| `PROJECT_VIRTUAL_MEMORY_A/B` | Paging |
| `PROJECT_FS`, `PROJECT_GOSFS`, etc. | Filesystems |

Set flags to `true` to enable functionality.
