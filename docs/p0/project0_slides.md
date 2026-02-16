# Project 0: Pipes - Implementation Guide

## System Calls

- A system call is the programmatic way in which a computer program requests a service from the kernel of the operating system it is executed on.
- Calling function (`pipe`, `read`, `write`) in a user executable (`pipe-p1.c`) will end up automatically calling its corresponding system call (`Sys_Pipe`, `Sys_Read`, `Sys_Write`). The binding of all system calls is in `src/libc/fileio.c`

**Flow of Pipe Create:**
```
pipe-p1.c → fileio.c → syscall.c → pipe.c
```

## Pipe System Call

- A pipe is a system call that creates a **unidirectional communication link between two file descriptors**
- A file descriptor is a **number that uniquely identifies an open file** in a computer's operating system
- `int Pipe(int *fd_read, int *fd_write)` takes two arguments: each is a pointer to an integer location
- When Pipe returns successfully, it would have created a pipe and filled the two locations with file descriptors (integers), one pointing to the reading end and the other to the writing end

## Key Structs

- `struct File` in `vfs.h`
- `struct FileOps` in `vfs.h`
- `struct Pipe` - you need to create this one

## Pipe_Create()

(Referred to as `Pipe()` in project spec)

**Architecture diagram:**
```
pipe(&r, &w) returns r=3, w=4

struct User_Context
[0][1][2][3][4][5]
          ↓   ↓
   struct File    struct File
   ops=readPipeOps ops=writePipeOps
   fsData          fsData
          ↘      ↙
        struct Pipe
        readers=1 | writers=1
        pointer to data buffer
               ↓
         buffered data
```

**Implementation steps:**
1. Two File double pointers (`READ_FILE` and `WRITE_FILE`) have been passed to populate the file struct
2. Create new `struct File` instance using `Malloc()` or `Allocate_File()`
3. Initialize necessary fields in the file struct
4. There are `File_Ops` defined in the `pipe.c` file
5. Need to have your own pipe struct to hold data and other variables of importance
   - The data buffer could be a fixed 32K or dynamically allocated buffer
6. Use `fsData` (void*) in file to point to the instance of your pipe struct
7. Check for appropriate error conditions wherever necessary
8. Return 0 if successful

## Sys_Pipe()

This is what is called when `Pipe()` command is executed in the test files (user mode):

1. Create the pipe (call `Pipe_Create()`)
2. Add files to the descriptor table
   - Look at `add_file_to_descriptor_table` function
3. Use `Copy_To_User(ulong_t destInUser, const void *srcInKernel, ulong_t bufSize)` to copy the file descriptors to the user addresses stored in the state registers
4. Remember the addresses in the state registers are memory addresses in user space; the code you are writing is in kernel space
5. Return 0 if successful, remember to check for error conditions throughout this function

## Pipe_Read()

(Referred to as `Read()` in project spec)

**Goal:** Reads data from the pipe into the buffer

**Inputs:**
- `num_bytes` you have to read from the pipe
- A buffer to copy data into
- A file struct pointer (`File *f`) which was created in `Pipe_Create()`

**Implementation:**
1. Check for appropriate error conditions:
   - Pipe has writers but no data → return `EWOULDBLOCK`
   - Pipe has no writers and no data → return 0
2. Copy the data into the buffer (it's a `void *`)
   - You can use `memcpy`
   - If there is data, `Read()` returns at most as much data as it was asked for
   - If there is not enough data, return as much data as the pipe has
3. Delete the data from the pipe's buffer (remove the data that user has just read out or mark the data you have read out as invalid)
4. Return number of bytes copied

## Pipe_Write()

(Referred to as `Write()` in project spec)

**Goal:** Copy data from buffer into the pipe

**Inputs:** Same params as `Read()`; buffer is the source

**Implementation:**
- Implement the buffer like a queue; write appends data, does NOT overwrite
- If there is a reader and the pipe has space for data, `Pipe_Write()` returns the number of bytes written

**Error conditions:**
- No reader → return `EPIPE`
- Fixed size buffer (suggested 32K): if buffer is full → return 0
- Dynamically allocated buffer: if `Malloc()` fails → return `ENOMEM`

## Pipe_Close()

(Referred to as `Close()` in project spec)

1. Identify if function is called on the read side or the write side and then act appropriately by closing the side on which it was called
2. Destroy data if there is no reader but there is still data
3. Pipe can also be destroyed if there are no readers and no writers

## VFS Layer

How does user's `read` call `Pipe_Read`:

```
read() (src/user/pipe-p1.c)
    ↓ interrupt, context switch
sys_read() (src/geekos/syscall.c)
    ↓
Read() (src/geekos/vfs.c)
    ↓
Pipe_Read() (src/geekos/pipe.c)
```

- You may want to read over those functions after context switch to help you debug your code
- Pay attention to how file ops are used
- Same routine for write and close

## Testing

- We provided `pipe-p1`, `pipe-p2`, and `pipe-p4` programs that you can execute in GeekOS
- Check `src/user/pipe-p1.c` for the test details
- You are encouraged to write your own tests

**Testing checkpoint:** After implementing `Pipe_Create()` and `Sys_Pipe()`, your code should be able to create a pipe. Try to run `pipe-p1` and it should pass the first assertion without any error.
