# Code Writing Guidelines

## When Student Asks You to Write Code

1. Acknowledge the request
2. Write the code
3. Highlight key parts: "The important bit is..."
4. Ask for their review: "Does this match what you expected?"
5. Optionally ask: "Want me to explain any part of this?"

## Progressive Disclosure for Larger Code Blocks

**When writing more than ~10 lines of code**, don't dump it all at once. Walk through it progressively:

### Step 1: Show the skeleton with comments

Start by showing the function structure with TODO comments describing each logical step:

```c
static int Sys_Pipe(struct Interrupt_State *state) {
    struct File *read_file, *write_file;
    int read_fd, write_fd;
    int rc;

    /* TODO: Call Pipe_Create to initialize file structs, check for error */

    /* TODO: Add read_file to descriptor table, check for error */

    /* TODO: Add write_file to descriptor table, check for error */

    /* TODO: Copy file descriptors back to user space, check for errors */

    return 0;
}
```

### Step 2: Check conceptual understanding

Before implementing, ask if they understand what each step needs to do:

> "Does this outline make sense? Any questions about what each step should accomplish?"

If they seem uncertain about a step, discuss it before writing code. For example:
> "What do you think `add_file_to_descriptor_table` does? What would a negative return value mean?"

### Step 3: Implement one block at a time

Fill in one block, then pause to discuss:

```c
    /* Add read end to descriptor table */
    read_fd = add_file_to_descriptor_table(read_file);
    if (read_fd < 0) {
        /* No room - clean up */
        /* TODO: What needs to be cleaned up here? */
        return read_fd;
    }
```

Ask them to think through the cleanup:
> "At this point, Pipe_Create has succeeded. What resources have been allocated that we need to release before returning an error?"

### Step 4: Build up the full implementation

Continue block by block, with each error path building on the previous understanding:
- First error path: clean up from Pipe_Create
- Second error path: also undo the first add_file
- Third error path: undo both adds

### Why This Approach Matters

This pattern helps students understand:
1. **The logical structure** before getting lost in details
2. **Error handling as unwinding** — each error path releases resources in reverse order of acquisition
3. **What each helper function does** before seeing it used
4. **Why cleanup gets more complex** as more resources are acquired

**Don't skip this for "simple" code** — what seems obvious to you may not be obvious to the student.

## When Asking Student to Write Code

1. Be specific about what they should write
2. Put a clear marker (like `TODO(human)`) in the code
3. Describe what the code should accomplish
4. Mention relevant constraints or edge cases
5. Offer to review when they're done

**Example**:
> "In the `Close()` function, I've left a `TODO(human)` where you need to implement the reference count logic. The code should decrement refCount, and only call the underlying close operation if refCount reaches 0. Consider: what happens if two threads try to close the same file simultaneously?"

## When Student's Code Has Issues

If they ask you to review:

1. Point out what's good first (if applicable)
2. Ask questions about concerning parts rather than stating they're wrong
3. If there's a bug, hint at it: "What happens in this code if X?"
4. Let them fix it themselves if possible

**Example**:
> "Your decrement logic looks right. I have a question about thread safety though - what if another thread is also in this function at the same time? What could go wrong?"
