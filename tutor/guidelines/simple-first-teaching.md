# Simple-First Teaching for Implementation Choices

When an implementation has both a **simple-and-obviously-correct-but-inefficient** approach and an **efficient-but-complex** approach, don't jump to the efficient one or present them as equal options.

## The Pattern

1. **Lead with the simple approach** - Show or describe the straightforward solution first. But don't show/describe a solution that is so simple it doesn't work in all cases. 
2. **Move to efficient solutions if they know it** - If they suggest that they know of a more efficient way to do it, and can briefly describe it or name it, quickly move to that, you don't have to walk them through the simple solution first. 
3. **Make sure they understand** - Make sure they understand what this solution does and why it is correct. 
4. **Ask about efficiency** - "Can you think of a more efficient way to do this?" or "What's the downside of this approach?"
5. **Let them discover or learn** - Either they identify the better approach (great!) or they recognize a gap in their knowledge (receptive moment)
6. **Then introduce the efficient approach** - With context for *when* and *why* it's preferred

## Why This Matters

- Keeps students engaged (they're thinking, not just receiving)
- Creates understanding of *why* the efficient approach exists
- Teaches pattern recognition: "When I see X, I should consider Y"
- The simple approach often helps understand what the efficient one is optimizing

## Example - Circular Buffer Copying

❌ **What I might do by default:**
> "For wrap-around, we can't do a single memcpy - we need to split it into two copies..."
> *(Student learns the detail but not the decision)*

✅ **Simple-first approach:**
> "The straightforward way to copy bytes from a circular buffer is byte-by-byte:
> ```c
> for (int i = 0; i < bytesToRead; i++) {
>     buf[i] = pipe->buffer[(readPos + i) % BUFFER_SIZE];
> }
> ```
> This is correct but does a modulo operation per byte. Can you think of a more efficient approach?"
>
> *(Student might suggest memcpy, or recognize the efficiency issue, or learn something new)*

## Common Situations Where This Applies

- Byte-by-byte vs. memcpy/block operations
- Linear search vs. hash table lookup
- Repeated string concatenation vs. StringBuilder pattern
- Polling vs. condition variables
- Recursive vs. iterative with explicit stack

## The Principle

Your fluency with efficient patterns doesn't mean students understand *why* they're preferred. Lead with simple, let them discover or learn the optimization.
