/*
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003,2013,2014 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 */
#include <geekos/pipe.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/errno.h>
#include <geekos/projects.h>
#include <geekos/int.h>


const struct File_Ops Pipe_Read_Ops =
    { NULL, Pipe_Read, NULL, NULL, Pipe_Close, NULL };
const struct File_Ops Pipe_Write_Ops =
    { NULL, NULL, Pipe_Write, NULL, Pipe_Close, NULL };

static ulong_t Min(ulong_t a, ulong_t b) {
    return a < b ? a : b;
}

int Pipe_Create(struct File **read_file, struct File **write_file) {
    struct Pipe *pipe;
    struct File *readPipe;
    struct File *writePipe;

    DONE_P(PROJECT_PIPE, "Create a pipe");

    if(read_file == 0 || write_file == 0)
        return EINVALID;

    *read_file = 0;
    *write_file = 0;

    pipe = (struct Pipe *)Malloc(sizeof(struct Pipe));
    if(pipe == 0)
        return ENOMEM;

    pipe->buffer = (char *)Malloc(PIPE_BUFFER_SIZE);
    if(pipe->buffer == 0) {
        Free(pipe);
        return ENOMEM;
    }

    pipe->capacity = PIPE_BUFFER_SIZE;
    pipe->readPos = 0;
    pipe->writePos = 0;
    pipe->count = 0;
    pipe->readers = 1;
    pipe->writers = 1;
    Mutex_Init(&pipe->mutex);
    Cond_Init(&pipe->dataAvailable);
    Cond_Init(&pipe->spaceAvailable);

    readPipe = Allocate_File(&Pipe_Read_Ops, 0, 0, pipe, O_READ, 0);
    if(readPipe == 0) {
        Free(pipe->buffer);
        Free(pipe);
        return ENOMEM;
    }

    writePipe = Allocate_File(&Pipe_Write_Ops, 0, 0, pipe, O_WRITE, 0);
    if(writePipe == 0) {
        Free(readPipe);
        Free(pipe->buffer);
        Free(pipe);
        return ENOMEM;
    }

    *read_file = readPipe;
    *write_file = writePipe;
    return 0;
}

int Pipe_Read(struct File *f, void *buf, ulong_t numBytes) {
    struct Pipe *pipe;
    ulong_t bytesToRead;
    ulong_t firstChunk;

    DONE_P(PROJECT_PIPE, "Pipe read");

    if(f == 0 || f->fsData == 0 || buf == 0)
        return EINVALID;

    if(numBytes == 0)
        return 0;

    pipe = (struct Pipe *)f->fsData;

    Mutex_Lock(&pipe->mutex);

    /* Block while buffer is empty but writers still exist */
    while (pipe->count == 0 && pipe->writers > 0)
        Cond_Wait(&pipe->dataAvailable, &pipe->mutex);

    if(pipe->count == 0) {      /* EOF: no data and no writers */
        Mutex_Unlock(&pipe->mutex);
        return 0;
    }

    bytesToRead = Min(numBytes, pipe->count);
    firstChunk = Min(bytesToRead, pipe->capacity - pipe->readPos);
    memcpy(buf, pipe->buffer + pipe->readPos, firstChunk);
    if(bytesToRead > firstChunk)
        memcpy((char *)buf + firstChunk, pipe->buffer, bytesToRead - firstChunk);

    pipe->readPos = (pipe->readPos + bytesToRead) % pipe->capacity;
    pipe->count -= bytesToRead;

    Cond_Signal(&pipe->spaceAvailable);
    Mutex_Unlock(&pipe->mutex);
    return (int)bytesToRead;
}

int Pipe_Write(struct File *f, void *buf, ulong_t numBytes) {
    struct Pipe *pipe;
    ulong_t freeBytes;
    ulong_t bytesToWrite;
    ulong_t firstChunk;

    DONE_P(PROJECT_PIPE, "Pipe write");

    if(f == 0 || f->fsData == 0 || buf == 0)
        return EINVALID;

    if(numBytes == 0)
        return 0;

    pipe = (struct Pipe *)f->fsData;

    Mutex_Lock(&pipe->mutex);

    if(pipe->readers == 0) {
        Mutex_Unlock(&pipe->mutex);
        return EPIPE;
    }

    /* Block while buffer is full but readers still exist */
    while (pipe->capacity - pipe->count == 0 && pipe->readers > 0)
        Cond_Wait(&pipe->spaceAvailable, &pipe->mutex);

    if(pipe->readers == 0) {
        Mutex_Unlock(&pipe->mutex);
        return EPIPE;
    }

    freeBytes = pipe->capacity - pipe->count;
    bytesToWrite = Min(numBytes, freeBytes);
    firstChunk = Min(bytesToWrite, pipe->capacity - pipe->writePos);
    memcpy(pipe->buffer + pipe->writePos, buf, firstChunk);
    if(bytesToWrite > firstChunk)
        memcpy(pipe->buffer, (char *)buf + firstChunk, bytesToWrite - firstChunk);

    pipe->writePos = (pipe->writePos + bytesToWrite) % pipe->capacity;
    pipe->count += bytesToWrite;

    Cond_Signal(&pipe->dataAvailable);
    Mutex_Unlock(&pipe->mutex);
    return (int)bytesToWrite;
}

int Pipe_Close(struct File *f) {
    struct Pipe *pipe;
    bool should_free;

    DONE_P(PROJECT_PIPE, "Pipe close");

    if(f == 0 || f->fsData == 0)
        return EINVALID;

    pipe = (struct Pipe *)f->fsData;

    Mutex_Lock(&pipe->mutex);

    if(f->ops == &Pipe_Read_Ops) {
        KASSERT(pipe->readers > 0);
        --pipe->readers;
    } else if(f->ops == &Pipe_Write_Ops) {
        KASSERT(pipe->writers > 0);
        --pipe->writers;
    } else {
        Mutex_Unlock(&pipe->mutex);
        return EINVALID;
    }

    /* Wake any blocked threads so they can detect the endpoint closed */
    Cond_Broadcast(&pipe->dataAvailable);
    Cond_Broadcast(&pipe->spaceAvailable);

    /* Capture inside lock: only the thread that makes both 0 will free */
    should_free = (pipe->readers == 0 && pipe->writers == 0);

    Mutex_Unlock(&pipe->mutex);

    if(should_free) {
        Free(pipe->buffer);
        Free(pipe);
    }

    return 0;
}
