# GeekOS Advanced Internals

This document provides low-level implementation details for students who want to understand GeekOS at the assembly and hardware level. This is supplementary material—see [geekos-architecture.md](geekos-architecture.md) for the conceptual overview.

---

## Table of Contents

1. [x86 Processor Architecture](#1-x86-processor-architecture)
2. [Memory Maps](#2-memory-maps)
3. [Context Switching](#3-context-switching)
4. [Key Data Structures](#4-key-data-structures)
5. [Important Functions Reference](#5-important-functions-reference)
6. [Device Driver Implementation](#6-device-driver-implementation)

---

## 1. x86 Processor Architecture

### Modes of Operation

**Real Mode (16-bit):**
- Used briefly during boot
- 1 MB address space (20-bit addresses)
- Address = (segment << 4) + offset
- No memory protection

**Protected Mode (32-bit):**
- GeekOS runs in this mode
- 4 GB address space (32-bit addresses)
- Memory protection via privilege levels
- Segmentation + optional paging

### Privilege Levels

```
Ring 0: Kernel mode (full hardware access)
Ring 1: Unused
Ring 2: Unused
Ring 3: User mode (restricted access)
```

### Segmentation

**Segment Selector (16-bit):**
```
Bits [15:3]: Index into GDT/LDT (13 bits)
Bit  [2]:    Table (0=GDT, 1=LDT)
Bits [1:0]:  Requested Privilege Level (RPL)
```

**Segment Descriptor (64-bit):**
- Base address: 32 bits (where segment starts)
- Limit: 20 bits (segment size)
- DPL: 2 bits (Descriptor Privilege Level)
- Type: 4 bits (code/data/system)
- Present: 1 bit
- Other flags (granularity, default operation size, etc.)

**Descriptor Tables:**
- **GDT (Global Descriptor Table):**
  - System-wide, pointed to by GDTR register
  - Entry 0: Null descriptor (required)
  - Entry 1: Kernel code (KERNEL_CS)
  - Entry 2: Kernel data (KERNEL_DS)
  - Entry 3+: TSS, user segments, LDTs
- **LDT (Local Descriptor Table):**
  - Per-process, pointed to by LDTR register
  - Contains user code/data segments
  - One LDT per user process

### Paging

**Linear to Physical Translation:**
```
Linear Address (32 bits):
  [31:22] Directory Index → Page Directory Entry → Page Table
  [21:12] Table Index     → Page Table Entry     → Physical Page
  [11:0]  Offset          → Byte within page

CR3 register: Physical address of page directory
```

**Page Directory/Table Entry (32-bit):**
```
[31:12] Physical address (20 bits, 4KB aligned)
[11:9]  Available for OS use
[8]     Global (G)
[7]     Page size (PS, PDE only: 0=4KB pages, 1=4MB pages)
[6]     Dirty (D, PTE only)
[5]     Accessed (A)
[4]     Cache disabled (PCD)
[3]     Write-through (PWT)
[2]     User/Supervisor (U/S: 0=kernel only, 1=user accessible)
[1]     Read/Write (R/W: 0=read-only, 1=writable)
[0]     Present (P: 0=page fault, 1=valid)
```

### Interrupts

**IDT (Interrupt Descriptor Table):**
- 256 entries (pointed to by IDTR)
- Each entry is an interrupt gate (64 bits)

**Interrupt Gate Structure:**
```
Bits [63:48]: Offset high (bits 31:16 of handler address)
Bits [47:32]: Segment Selector (usually KERNEL_CS)
Bits [31:16]: Offset low (bits 15:0 of handler address)
Bits [15:13]: DPL (who can invoke: 0=kernel only, 3=user can use INT)
Bits [12:8]:  Type (0xE = 32-bit interrupt gate)
Bits [7:0]:   Reserved/flags
```

**Interrupt Stack Frame (pushed by CPU):**
```
Same privilege (kernel→kernel):
  [EFLAGS] [CS] [EIP] [Error Code]

Privilege change (user→kernel):
  [Old SS] [Old ESP] [EFLAGS] [CS] [EIP] [Error Code]
```

### Registers

**General Purpose (32-bit):**
| Register | Convention |
|----------|------------|
| EAX | Accumulator, return value |
| EBX | Base, callee-saved |
| ECX | Counter, argument |
| EDX | Data, argument |
| ESI | Source Index, callee-saved |
| EDI | Destination Index, callee-saved |
| EBP | Base Pointer (stack frame) |
| ESP | Stack Pointer |

**Segment Registers (16-bit visible + 64-bit hidden cache):**
- CS: Code Segment
- DS, ES, FS, GS: Data Segments
- SS: Stack Segment

**Control Registers:**
| Register | Purpose |
|----------|---------|
| CR0 | Protected mode enable (PE), paging enable (PG) |
| CR2 | Page fault linear address |
| CR3 | Page directory base (physical address) |
| CR4 | Extended features (PSE, PAE, etc.) |

**System Registers:**
| Register | Purpose |
|----------|---------|
| GDTR | Points to GDT (48-bit: 16-bit limit + 32-bit base) |
| IDTR | Points to IDT (48-bit: 16-bit limit + 32-bit base) |
| LDTR | Points to LDT (16-bit selector into GDT) |
| TR | Points to TSS (16-bit selector into GDT) |
| EIP | Instruction Pointer |
| EFLAGS | Status flags (CF, ZF, SF, OF, IF, etc.) |

---

## 2. Memory Maps

### Physical Memory Layout at Boot

```
Address         Contents
───────         ────────
0x00000000      BIOS data area, Interrupt Vector Table
0x00001000      Available low memory
0x00007C00      Boot sector (loaded by BIOS, then relocated)
0x00010000      Kernel image (loaded by bootsect.asm)
                  .text (code)
                  .rodata (read-only data)
                  .data (initialized data)
                  .bss (uninitialized data)
                kernEnd (symbol marking end of kernel)
0x00090000      Boot sector (relocated, INITSEG)
0x00090200      setup.asm (SETUPSEG)
0x00090400      Setup stack
0x000A0000      ISA hole start (reserved for hardware)
0x000B8000      VGA text mode video memory
0x000C0000      Video BIOS ROM
0x000F0000      System BIOS ROM
0x00100000      Extended memory starts (1 MB mark)
                Initial thread structure
0x00101000      Initial kernel stack
                Kernel heap
0x00111000+     Free pages for allocation
...
0xFEC00000      IO APIC registers (memory-mapped)
0xFEE00000      Local APIC registers (memory-mapped)
```

### Kernel Virtual Address Space

With paging enabled, the kernel uses identity mapping (virtual = physical) for most addresses:

```
0x00000000 - 0x00001000    Unmapped (NULL pointer protection)
0x00001000 - 0x000A0000    Low memory (identity mapped)
0x000A0000 - 0x00100000    ISA hole (VGA at 0xB8000, identity mapped)
0x00100000 - 0x00A00000    Kernel code, data, heap (identity mapped)
0x00A00000 - 0xF0000000    Available for user process mappings
0xFEC00000 - 0xFEE00000    IO APIC (identity mapped, kernel only)
0xFEE00000 - 0xFFFFFFFF    Local APIC (identity mapped, kernel only)
```

### User Process Virtual Address Space

Each user process has its own page directory. Typical layout:

```
0x00000000 - 0x00001000    Unmapped (NULL pointer protection)
0x00001000 - 0x08000000    Code and data segments (loaded from ELF)
0x08000000 - 0xC0000000    Heap (grows upward via sbrk)
0xC0000000 - 0xEFFFFFFF    Stack (grows downward from high addresses)
0xF0000000 - 0xFFFFFFFF    Kernel mappings (inaccessible from user mode)
```

---

## 3. Context Switching

### Kernel Stack Layout

When a thread is not running, its processor state is saved on its kernel stack:

```
Kernel Stack (high addresses at top):
  ┌─────────────────────────┐
  │ [User SS]               │ ← Only present if interrupted from user mode
  │ [User ESP]              │ ← Only present if interrupted from user mode
  ├─────────────────────────┤
  │ [EFLAGS]                │
  │ [CS]                    │
  │ [EIP]                   │ ← Return address
  │ [Error Code]            │ ← Only for some interrupts (or fake 0)
  │ [Interrupt Number]      │
  ├─────────────────────────┤
  │ [EAX]                   │
  │ [EBX]                   │
  │ [ECX]                   │
  │ [EDX]                   │
  │ [ESI]                   │
  │ [EDI]                   │
  │ [EBP]                   │
  ├─────────────────────────┤
  │ [DS]                    │
  │ [ES]                    │
  │ [FS]                    │
  │ [GS]                    │
  └─────────────────────────┘ ← thread->esp points here
```

### Handle_Interrupt (lowlevel.asm)

This is the common entry point for all interrupts:

```nasm
Handle_Interrupt:
    ; CPU has already pushed (if from user mode): SS, ESP
    ; CPU has already pushed: EFLAGS, CS, EIP, [error code]
    ; Our stub pushed: [fake error code if needed], interrupt number

    ; Save complete processor state
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment
    mov ax, KERNEL_DS
    mov ds, ax
    mov es, ax

    ; Call C interrupt handler
    push esp                    ; Pointer to Interrupt_State struct
    mov eax, [esp + 4 + 48]     ; Get interrupt number from stack
    call [g_interruptTable + eax * 4]
    add esp, 4                  ; Pop argument

    ; Check if context switch needed
    mov eax, [g_preemptionDisabled + ecx*4]  ; ecx = CPU ID
    test eax, eax
    jnz .no_switch

    mov eax, [g_needReschedule + ecx*4]
    test eax, eax
    jz .no_switch

    ; Perform context switch
    call Switch_To_Next_Thread

.no_switch:
    ; Activate user context if present
    mov eax, [g_currentThreads + ecx*4]
    mov eax, [eax + USERCONTEXT_OFFSET]
    test eax, eax
    jz .no_user_context
    call Activate_User_Context

.no_user_context:
    ; Check for pending signals
    call Check_Pending_Signal

    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax

    ; Skip error code and interrupt number
    add esp, 8

    ; Return from interrupt
    iret
```

### Switch_To_Thread (lowlevel.asm)

Called when a thread voluntarily yields (e.g., from Schedule()):

```nasm
Switch_To_Thread:
    ; Entry: argument is pointer to next thread
    ; Stack: [next_thread_ptr] [return_address]

    ; Build an interrupt frame on current stack
    pushf                       ; EFLAGS
    push cs                     ; CS
    push dword [esp + 12]       ; EIP (return address)
    push dword 0                ; Fake error code
    push dword 0                ; Fake interrupt number
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    push ds
    push es
    push fs
    push gs

    ; Save current thread's stack pointer
    mov eax, [g_currentThreads + ecx*4]  ; ecx = CPU ID
    mov [eax + ESP_OFFSET], esp

    ; Load next thread
    mov eax, [esp + 64]         ; Get next_thread_ptr argument
    mov [g_currentThreads + ecx*4], eax
    mov esp, [eax + ESP_OFFSET]

    ; Activate user context if present
    mov eax, [eax + USERCONTEXT_OFFSET]
    test eax, eax
    jz .kernel_thread
    call Activate_User_Context

.kernel_thread:
    ; Check for pending signals
    call Check_Pending_Signal

    ; Restore registers and return
    pop gs
    pop fs
    pop es
    pop ds
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    add esp, 8                  ; Skip error/int#
    iret
```

### Getting Current Thread

```c
#define CURRENT_THREAD (g_currentThreads[Get_CPU_ID()])

int Get_CPU_ID(void) {
    ulong_t apicId;

    Disable_Interrupts();
    // Read Local APIC ID register at offset 0x20
    apicId = *((volatile ulong_t *)(LAPIC_BASE + LAPIC_ID));
    Enable_Interrupts();

    return apicId >> 24;  // APIC ID is in bits 31:24
}
```

---

## 4. Key Data Structures

### struct Interrupt_State

Defined in `include/geekos/int.h`. Matches the stack layout after an interrupt:

```c
struct Interrupt_State {
    // Pushed by our handler (Handle_Interrupt)
    uint_t gs;
    uint_t fs;
    uint_t es;
    uint_t ds;
    uint_t ebp;
    uint_t edi;
    uint_t esi;
    uint_t edx;
    uint_t ecx;
    uint_t ebx;
    uint_t eax;

    // Pushed by interrupt stub
    uint_t intNum;
    uint_t errorCode;

    // Pushed by CPU
    uint_t eip;
    uint_t cs;
    uint_t eflags;

    // Only present if privilege change (user→kernel)
    uint_t userEsp;
    uint_t userSs;
};
```

### struct Kernel_Thread

Defined in `include/geekos/kthread.h`:

```c
struct Kernel_Thread {
    ulong_t esp;                    // Saved stack pointer (offset 0)
    unsigned char *stackPage;       // Kernel stack base (4KB page)
    struct User_Context *userContext; // NULL for pure kernel threads

    int priority;                   // Thread priority
    int totalTime;                  // Total ticks consumed
    int numTicks;                   // Ticks in current quantum
    struct Thread_Queue *runQueue;  // Which queue thread is in (or NULL)

    int pid;                        // Process ID
    struct Kernel_Thread *owner;    // Parent thread
    struct Thread_Queue joinQueue;  // Threads waiting to join this one
    int exitCode;                   // Exit status
    bool alive;                     // Still running?
    bool detached;                  // No one will join?
    int refCount;                   // Reference count

    const void *tlocalData[MAX_TLOCAL_KEYS];  // Thread-local storage

    // Linked list pointers (via DEFINE_LINK macro)
    struct Kernel_Thread *prevThread_Queue;
    struct Kernel_Thread *nextThread_Queue;
    struct Kernel_Thread *prevAll_Thread_List;
    struct Kernel_Thread *nextAll_Thread_List;
};
```

### struct User_Context

Defined in `include/geekos/user.h`:

```c
struct User_Context {
    char name[MAX_PROC_NAME_LEN+1];     // Process name

    char *memory;                        // Base of user memory
    ulong_t size;                        // Size of user memory

    struct Segment_Descriptor *ldtDescriptor;  // Pointer into GDT
    ushort_t ldtSelector;                // GDT selector for LDT
    ushort_t csSelector;                 // User code segment selector
    ushort_t dsSelector;                 // User data segment selector
    ulong_t entryAddr;                   // Program entry point
    ulong_t stackPointerAddr;            // Initial user stack pointer
    ulong_t argBlockAddr;                // Command-line arguments

    pde_t *pageDir;                      // Page directory (if paging)
    struct File *file_descriptor_table[USER_MAX_FILES];

    int refCount;                        // Reference count

    // Signal handling (Project 2)
    signal_handler signalTable[MAXSIG];  // Registered handlers
    ulong_t signalPending;               // Pending signal bitmap
    ulong_t trampolineAddr;              // Signal return trampoline
};
```

### struct Thread_Queue

Generic doubly-linked list with spinlock:

```c
struct Thread_Queue {
    struct Kernel_Thread *head;
    struct Kernel_Thread *tail;
    Spin_Lock_t lock;
};
```

### struct VFS_File_Stat

File metadata returned by Stat():

```c
struct VFS_File_Stat {
    int size;                           // File size in bytes
    bool isDirectory;                   // Is this a directory?
    bool isSetuid;                      // Setuid bit set?
    struct VFS_ACL acls[VFS_MAX_ACL_ENTRIES];  // Access control
};
```

### struct VFS_Dir_Entry

Directory entry returned when reading directories:

```c
struct VFS_Dir_Entry {
    char name[VFS_MAX_NAME_LEN+1];      // File/directory name
    struct VFS_File_Stat stats;          // File metadata
};
```

---

## 5. Important Functions Reference

### Memory Management

```c
// Kernel heap allocation
void *Malloc(ulong_t size);
void Free(void *ptr);

// Physical page allocation (4KB pages)
void *Alloc_Page(void);
void Free_Page(void *page);

// Allocate contiguous pages
void *Alloc_Pageable_Page(pde_t *pageDir, ulong_t vaddr);
```

### Thread Management

```c
// Get current thread (macro)
#define CURRENT_THREAD (g_currentThreads[Get_CPU_ID()])

// Thread lifecycle
struct Kernel_Thread *Start_Kernel_Thread(
    Thread_Start_Func startFunc,
    ulong_t arg,
    int priority,
    bool detached,
    const char *name
);
void Exit(int exitCode);
int Join(struct Kernel_Thread *thread);

// Scheduling
void Make_Runnable(struct Kernel_Thread *thread);
void Yield(void);
void Schedule(void);
struct Kernel_Thread *Get_Next_Runnable(void);
```

### Synchronization

```c
// Interrupt control (WARNING: affects THIS CPU only - not SMP-safe for shared data!)
// For shared data like reference counts, use Atomic_Increment/Decrement from atomic.h
void Disable_Interrupts(void);
void Enable_Interrupts(void);
bool Save_And_Disable_Interrupts(void);      // Returns previous interrupt enabled state
void Restore_Interrupt_State(bool state);  // Restores previous interrupt enabled state

// Spinlocks (for short critical sections, interrupt handlers)
void Spin_Lock(Spin_Lock_t *lock);
void Spin_Unlock(Spin_Lock_t *lock);
void Spin_Lock_Init(Spin_Lock_t *lock);

// Mutexes (can block, for longer critical sections)
void Mutex_Init(struct Mutex *mutex);
void Mutex_Lock(struct Mutex *mutex);
void Mutex_Unlock(struct Mutex *mutex);
bool Mutex_Is_Held(struct Mutex *mutex);

// Condition variables
void Cond_Init(struct Condition *cond);
void Cond_Wait(struct Condition *cond, struct Mutex *mutex);
void Cond_Signal(struct Condition *cond);
void Cond_Broadcast(struct Condition *cond);

// Wait queues
void Wait(struct Thread_Queue *waitQueue);
void Wake_Up(struct Thread_Queue *waitQueue);
void Wake_Up_One(struct Thread_Queue *waitQueue);
```

### User/Kernel Data Transfer

```c
// Copy data between user and kernel space
int Copy_From_User(void *destInKernel, ulong_t srcInUser, ulong_t numBytes);
int Copy_To_User(ulong_t destInUser, void *srcInKernel, ulong_t numBytes);

// Convert user pointer to kernel pointer (segmentation-based)
void *User_To_Kernel(struct User_Context *context, ulong_t userPtr);
```

### Debugging

```c
// Kernel printf (outputs to VGA screen)
void Print(const char *fmt, ...);

// Assertions (panics if condition false)
#define KASSERT(condition) ...

// Dump state for debugging
void Dump_Interrupt_State(struct Interrupt_State *state);
void Dump_Thread_Queue(struct Thread_Queue *queue);
```

### I/O Ports

```c
// Read from I/O port
uchar_t In_Byte(ushort_t port);
ushort_t In_Word(ushort_t port);
ulong_t In_DWord(ushort_t port);

// Write to I/O port
void Out_Byte(ushort_t port, uchar_t value);
void Out_Word(ushort_t port, ushort_t value);
void Out_DWord(ushort_t port, ulong_t value);
```

---

## 6. Device Driver Implementation

### Timer (Local APIC)

Each CPU has a Local APIC with its own timer. The timer interrupt handler:

```c
void Timer_Interrupt_Handler(struct Interrupt_State *state) {
    int cpuid = Get_CPU_ID();
    struct Kernel_Thread *current = g_currentThreads[cpuid];

    // CPU 0 maintains global tick count
    if (cpuid == 0) {
        g_numTicks++;
    }

    // Update thread's tick count
    current->numTicks++;
    current->totalTime++;

    // Check if quantum expired
    if (current->numTicks >= g_Quantum) {
        g_needReschedule[cpuid] = true;
    }

    // Process alarms (CPU 0 only)
    if (cpuid == 0) {
        Check_Alarms();
    }
}
```

### Keyboard

**Port addresses:**
- `KB_DATA` (0x60): Data port (read scan codes)
- `KB_CMD` (0x64): Command/status port

```c
#define KB_DATA 0x60
#define KB_CMD  0x64
#define KB_OUTPUT_FULL 0x01
#define KB_KEY_RELEASE 0x80

static struct Thread_Queue s_keyboardWaitQueue;
static struct Key_Queue s_queue;
static Spin_Lock_t s_kbdLock;

void Keyboard_Interrupt_Handler(struct Interrupt_State *state) {
    uchar_t status = In_Byte(KB_CMD);

    if (status & KB_OUTPUT_FULL) {
        uchar_t scanCode = In_Byte(KB_DATA);
        bool release = (scanCode & KB_KEY_RELEASE) != 0;
        scanCode &= ~KB_KEY_RELEASE;

        if (Is_Modifier_Key(scanCode)) {
            Update_Modifier_State(scanCode, release);
        } else if (!release) {
            keycode_t keyCode = Translate_Scan_Code(scanCode);

            Spin_Lock(&s_kbdLock);
            if (!Is_Queue_Full(&s_queue)) {
                Enqueue_Key(&s_queue, keyCode);
                Wake_Up(&s_keyboardWaitQueue);
            }
            Spin_Unlock(&s_kbdLock);
        }
    }
}

keycode_t Wait_For_Key(void) {
    keycode_t key;
    bool iflag = Spin_Lock_Irq_Save(&s_kbdLock);

    while (Is_Queue_Empty(&s_queue)) {
        Spin_Unlock(&s_kbdLock);
        Wait(&s_keyboardWaitQueue);
        Spin_Lock(&s_kbdLock);
    }
    key = Dequeue_Key(&s_queue);

    Spin_Unlock_Irq_Restore(&s_kbdLock, iflag);
    return key;
}
```

### VGA Screen

**Video memory layout:**
- Base address: `0xB8000`
- Size: 25 rows × 80 columns × 2 bytes = 4000 bytes
- Each character: low byte = ASCII, high byte = attribute

```c
#define VIDMEM_ADDR 0xB8000
#define NUMROWS 25
#define NUMCOLS 80
#define CRT_ADDR_REG 0x3D4
#define CRT_DATA_REG 0x3D5

#define ATTRIB(bg, fg) (((bg) << 4) | (fg))

static ushort_t *s_videoMem = (ushort_t *)VIDMEM_ADDR;
static int s_row, s_col;
static uchar_t s_attr = ATTRIB(BLACK, GRAY);

void Put_Char(int ch) {
    switch (ch) {
    case '\n':
        s_row++;
        s_col = 0;
        break;
    case '\r':
        s_col = 0;
        break;
    case '\b':
        if (s_col > 0) s_col--;
        break;
    default:
        if (ch >= ' ' && ch <= '~') {
            int offset = s_row * NUMCOLS + s_col;
            s_videoMem[offset] = (s_attr << 8) | ch;
            s_col++;
        }
    }

    // Handle line wrap
    if (s_col >= NUMCOLS) {
        s_col = 0;
        s_row++;
    }

    // Handle scroll
    if (s_row >= NUMROWS) {
        Scroll_Screen();
        s_row = NUMROWS - 1;
    }

    Update_Cursor();
}

void Update_Cursor(void) {
    int pos = s_row * NUMCOLS + s_col;
    Out_Byte(CRT_ADDR_REG, 14);           // Cursor high register
    Out_Byte(CRT_DATA_REG, (pos >> 8));
    Out_Byte(CRT_ADDR_REG, 15);           // Cursor low register
    Out_Byte(CRT_DATA_REG, pos & 0xFF);
}
```

### IDE Disk Controller

**PIO (Programmed I/O) Read Operation:**

```c
#define IDE_DATA_REG          0x1F0
#define IDE_SECTOR_COUNT_REG  0x1F2
#define IDE_SECTOR_NUMBER_REG 0x1F3
#define IDE_CYLINDER_LOW_REG  0x1F4
#define IDE_CYLINDER_HIGH_REG 0x1F5
#define IDE_DRIVE_HEAD_REG    0x1F6
#define IDE_STATUS_REG        0x1F7
#define IDE_COMMAND_REG       0x1F7

#define IDE_BSY  0x80    // Busy
#define IDE_DRDY 0x40    // Drive ready
#define IDE_DRQ  0x08    // Data request

#define IDE_READ_SECTORS_CMD  0x20
#define IDE_WRITE_SECTORS_CMD 0x30

static void IDE_Read(int drive, int blockNum, void *buf) {
    int cylinder, head, sector;

    // Convert LBA to CHS
    LBA_To_CHS(drive, blockNum, &cylinder, &head, &sector);

    // Wait for drive ready
    while (In_Byte(IDE_STATUS_REG) & IDE_BSY)
        ;

    // Select drive and head
    Out_Byte(IDE_DRIVE_HEAD_REG, 0xA0 | (drive << 4) | head);

    // Set cylinder and sector
    Out_Byte(IDE_CYLINDER_LOW_REG, cylinder & 0xFF);
    Out_Byte(IDE_CYLINDER_HIGH_REG, (cylinder >> 8) & 0xFF);
    Out_Byte(IDE_SECTOR_NUMBER_REG, sector);
    Out_Byte(IDE_SECTOR_COUNT_REG, 1);

    // Issue read command
    Out_Byte(IDE_COMMAND_REG, IDE_READ_SECTORS_CMD);

    // Wait for data ready
    while (!(In_Byte(IDE_STATUS_REG) & IDE_DRQ))
        ;

    // Read 256 words (512 bytes)
    ushort_t *wordBuf = (ushort_t *)buf;
    for (int i = 0; i < 256; i++) {
        wordBuf[i] = In_Word(IDE_DATA_REG);
    }
}
```

**Request Thread Pattern:**

```c
static void IDE_Request_Thread(ulong_t arg) {
    while (true) {
        // Block until request available
        struct Block_Request *req = Dequeue_Request(&s_requestQueue);

        // Perform synchronous I/O
        if (req->type == BLOCK_READ) {
            IDE_Read(req->dev->unit, req->blockNum, req->buf);
        } else {
            IDE_Write(req->dev->unit, req->blockNum, req->buf);
        }

        // Signal completion
        Notify_Request_Completion(req, COMPLETED, 0);
    }
}
```

---

## Debugging Quick Reference

### GDB Commands

```
break Main                  # Break at function
break *0x10000              # Break at address
continue                    # Resume execution
step / next                 # Step into / over
print variable              # Print variable
print/x $eax                # Print register in hex
info registers              # Show all registers
backtrace                   # Show call stack
x/10wx 0xB8000              # Examine 10 words at address
```

### QEMU Monitor

Press `Ctrl-Alt-2` to access QEMU monitor:

```
info registers              # CPU registers
info mem                    # Page tables
info tlb                    # TLB contents
x /10wx 0xaddr              # Examine virtual memory
xp /10wx 0xaddr             # Examine physical memory
```

### Common Issues

| Symptom | Likely Cause |
|---------|--------------|
| Triple fault / reboot | Bad GDT, IDT, or stack overflow |
| GPF (interrupt 13) | Segment violation, bad privilege |
| Page fault (interrupt 14) | Check CR2 for fault address |
| Hang | Deadlock, check spinlock/mutex state |
| Wrong syscall result | Check Copy_From_User/Copy_To_User |
