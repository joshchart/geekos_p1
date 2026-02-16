# Project 1: Fork & Exec - Slides Reference

**Minimum Requirements: 5 public tests**

## Test Distribution

- Public tests – 6 tests | 27 points
- Release tests – 3 tests | 15 points
- Secret tests – 7 tests | 35 points

## Process and Thread

### Process
The instance of a computer program that is being executed. It contains not only the executable itself but also the resources assigned to that process.

### Thread
A smallest unit that can be scheduled onto CPU, it shares with other threads belonging to the same process its memory and other OS resources.

### Key Difference
The most important difference is that threads (of the same process) run in a shared memory space, while processes run in separate memory spaces.

## Process Control Block (PCB)

A data structure in the operating system kernel containing the information needed to manage the processes.

**Why is PCB needed:** Scheduling, context switch, etc.

## PCB in GeekOS

- **struct User_Context** (user.h)
  - Memory related data, file_descriptor_table, etc.

- **struct Kernel_Thread** (kthread.h)
  - Kernel stack, scheduling information, parent information, etc.
  - A user process has its corresponding kernel thread.

## Registers, Kernel Stack and Interrupt State

When a user process calls a system call, its registers are pushed into its kernel stack and enters kernel mode.

- **Record the context:** when it returns to user mode, the values are popped out to restore the values of registers.
- **Passing arguments:** Arguments for syscalls are written into registers first and then they will be written into kernel stack. Then syscall functions will be able to get these arguments by looking at its kernel stack.
- **Interrupt_State** is a pointer pointing to the top of the kernel stack right after we enter the syscalls. This is why Interrupt_State is passed in as an argument for all system calls, so that the syscalls will be able to get the arguments.

## Useful Functions

### From userseg.c
```c
struct User_Context *Create_User_Context(ulong_t size)
```
Create a new user context of given size. Ignore the LDT part, it will be mentioned in following projects.

### From kthread.c
```c
struct Kernel_Thread *Create_Thread(int priority, bool detached)
```
Create an instance of kthread. Pay attention to how stackPage (kernel stack) is allocated. Look at Init_Thread in the function to see how stack pointer (esp) is initialized.

```c
void Setup_User_Thread(struct Kernel_Thread *kthread, struct User_Context *userContext)
```
Attach the userContext to the kthread. Push initial values to the kernel stack so that when they are popped out to fill in the values of the registers, the values of the registers will be initialized properly to start a new user process (e.g. instruction pointer).

### From user.c
```c
void Attach_User_Context(struct Kernel_Thread *kthread, struct User_Context *context)
```
Associate the given user context with a kernel thread.

```c
void Detach_User_Context(struct Kernel_Thread *kthread)
```
Get rid of the old user context and destroy the UserContext.

```c
void Destroy_User_Context(struct User_Context *userContext)  // in userseg.c
```
Destroy an instance of User_Context.

### From sched.c
```c
void Make_Runnable_Atomic(struct Kernel_Thread *kthread)
```
Add the kthread to the running queue using an atomic operation so that it can be scheduled to run.

### From syscall.c
```c
Copy_User_String() and get_path_from_registers()
```
Can be used in Sys_Execl. Copy the path of the executable from the address given by the registers.

> **Note:** Go through the definitions of the following functions. Make sure the function is visible to other files if you need them.

## Create a New Process: Fork()

- After a Fork() call, the caller (the parent) creates a new process (the child) that is identical to itself.
- Fork returns different values to the parent and the child. It returns the child's PID to the parent, and 0 to the child.
- If the creation of a child process was unsuccessful, Fork() returns a negative integer (error code).
- After Fork() returns, both the parent and the child will continue executing the next instruction that follows Fork().

## Start a New Program: Exec()

- A process replaces the program it was running by calling Exec().
- Load the program into the current process space and runs it from the entry point.
- Exec does not return on success, returns error code if any unexpected error occurs.

## Functions to Implement/Modify

### In syscall.c
- `Sys_Fork`
- `Sys_Execl`
- `Sys_Exit`

### In vfs.c
- `Close`

### Other modifications
- Make modifications to `struct File` (in vfs.h) to support multiple processes.
- Probably modify previous pipe implementation (most likely you don't need to modify anything).

## Sys_Fork() Implementation

1. **Create a new user context (User_Context) and a new thread (Kernel_Thread)** for the child process.

2. **Copy the address space from parent to child.** To get the current thread, use the macro `CURRENT_THREAD`. Fork() creates an exact duplicate process (with some differences such as the new process having its own address space), so transfer data from parent process to child process appropriately (necessary components in struct User_Context, but leave ldtselector, csselector, dsselector, entryAddr, argBlockAddr, stackpointerAddr, etc. alone).

3. **Copy the FDT.** Files can be referenced by multiple processes, so keep track of reference counts for each file.
   - Think where the reference count should be initialized and incremented.
   - Might want to use mutexes to make increments atomic.

4. **Copy the kernel stack from parent to child** and fix the position of `Kernel_Thread::esp`.

5. **Fork() returns the child's pid for the parent** by directly returning the value. Returns 0 for the child - this return value needs to be stored in the EAX register of the child process.

6. **Add the created thread to the ready queue** so the scheduler can schedule. (Refer to `Start_User_Thread` in kthread.c to see how it is done, but you should not use it because it also does something else rather than just adding the thread to the queue.)

7. **Check for appropriate error conditions throughout** (mostly ENOMEM).

### Return Value Details

- X86 uses EAX register to return values, the EAX register of the child process needs to be changed to the returning value of the child process.
- When a process enters kernel mode, all registers are pushed into the kernel stack of its corresponding kernel thread. When it returns to user mode, the registers are recovered from the stack.
- To return the appropriate value for child process, the stack of the parent process needs to be copied to the child process and the corresponding position storing EAX should be modified.
- Refer to `Setup_Kernel_Thread` to see how the stack looks like when a newly kernel thread is set up.

## Child ESP

There are two ways to set the child esp:

1. Follow `setup_user_thread`, but instead of pushing dummy/initial values, retrieve the values from parent and put them on the child's stack page.

2. **The lazy way:** Copy over the entire parent kernel stack and calculate where the child esp should be and set it correctly.

### Kernel Stack Diagram

```
High Address
┌─────────────────────────────────────────────────────────────────────────┐
│                                                                         │
│  (Parent's kernel stack)          │    (Child's kernel stack copy)      │
│  Before Fork() call               │                                     │
│                                   │                                     │
│  ┌──────────────────┐             │    ┌──────────────────┐             │
│  │ Other stuff in   │ ▲           │    │ Parent's kernel  │             │
│  │ kernel stack     │ │           │    │ stack            │             │
│  ├──────────────────┤ │ PAGE_SIZE │    ├──────────────────┤ PAGE_SIZE   │
│  │ Register contents│ │           │    │ struct           │             │
│  │ (Interrupt_State)│ │           │    │ Interrupt_State  │             │
│  │ pushed into the  │ │           │    ├──────────────────┤             │
│  │ kernel stack     │ │           │    │                  │             │
│  ├──────────────────┤ │           │    │                  │             │
│  │                  │ │           │    ├──────────────────┤             │
│  │ Stack grows      │ │           │    │ Child's kernel   │             │
│  │ this way ↓       │ │           │    │ stack            │             │
│  │                  │ │           │    ├──────────────────┤             │
│  │                  │ │           │    │ struct           │             │
│  │                  │ │           │    │ Interrupt_State  │ PAGE_SIZE   │
│  │                  │ │           │    ├──────────────────┤             │
│  │                  │ │           │    │                  │             │
│  │                  │ │           │    │ ← set child esp  │             │
│  │                  │ │           │    │    here          │             │
│  │                  │ │           │    │                  │             │
│  └──────────────────┘ │           │    └──────────────────┘             │
│  Kernel_Thread->      │           │    Kernel_Thread->                  │
│  stackPage            ▼           │    stackPage                        │
│                                   │                                     │
│ Low Address                       │                                     │
└─────────────────────────────────────────────────────────────────────────┘
```

A new Kernel_Thread and User_Context is created for the child process, and the child has a copy of the parent's kernel stack.

## Return Value of Fork

- As mentioned earlier, Fork returns 0 to child and child's PID to parent.
- X86 uses eax to hold the return values.
- Two ways to find eax:
  1. Use esp and relative position
  2. Typecasting esp, then use `->eax`

## Sys_Execl() Implementation

1. **Copy the executable path and the command string** that user provided to kernel space (refer to other syscalls to see how to do this). Read the comments of Sys_Execl() to see what registers are used.

2. **Refer to Spawn() system call** that GeekOS currently uses on how to create a new process.

3. **Get rid of old user context** and follow Spawn() (in user.c) code to see what needs to be done.

4. **Look at appropriate man pages** to read about components that are preserved across a call to exec (FDT).

5. **Set esp to the initial position** (think about what the initial position should be).

6. **Use Setup_User_Thread** to attach the user context, init the stack. (Think about why we don't need to put it in the running queue)

7. **Check for appropriate error conditions throughout.**

## Sys_Exit() Implementation

> **Warning:** Make sure your code works and make a submission before start working on this function.

- You can either implement the following in Sys_Exit or Exit (kthread.c).
- Iterate through available threads and check if owner is the current thread. If so, take care of it. Follow through the logic of reaping a thread and appropriately determine conditions to mop children off (hint: use `Detach_Thread`).
- Refer to `Lookup_Thread` (kthread.c) to see how to iterate through all the threads.
- **Note that GeekOS does not allow lock re-entrant.**

## Close() in vfs.c

- Decrease the reference count
- When the reference count is 0, close the file.

## Locks

- `Mutex s_vfsLock` in vfs.c
- `Spin_Lock_t kthreadLock` in syscall.c
