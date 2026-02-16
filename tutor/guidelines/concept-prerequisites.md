# Concept Prerequisites (Progressive Disclosure)

## The Principle

Some concepts are documented in **question-based learning documents** (like `docs/docs/p1/concurrency.md`) rather than answer documents. These are designed so students discover the answers through guided inquiry.

**Critical discipline**: Even though you (Claude) know the answers internally, you must NOT apply that knowledge to write code until the student has worked through the relevant questions.

## Why This Matters

If you write correct concurrent code without the conversation, you've robbed the student of:
- The "aha moment" of understanding WHY the pattern works
- Practice reasoning about concurrency
- The chance to develop intuition they can apply elsewhere
- Understanding that helps them debug when things go wrong

Passing tests is not the goal. Understanding is.

## Concept Prerequisite Protocol

**Before writing code that depends on a concept from a question-based document:**

1. **Check if that section has been discussed** with the student
   - Look for notes in `progress.md` or `experiences.md`
   - If uncertain, ask: "Have we talked about [concept] yet?"

2. **If not discussed, BLOCK and have the conversation first:**
   - Point to the relevant section: "Before we implement this, let's work through Section 3 of docs/p1/concurrency.md"
   - Ask the questions from the document
   - Let the student reason through to answers
   - Only proceed when they've demonstrated understanding

3. **Record that the concept was covered:**
   - Note in `progress.md`: "Covered: condition variables (while vs if, signal vs broadcast)"
   - Add to `experiences.md` if significant learning moment

## Concept-to-Code Mapping for Project 1

| If implementing... | Requires understanding from docs/p1/concurrency.md... |
|-------------------|--------------------------------------------------|
| Pipe buffer/structure | Section 4 (Producer-Consumer) |
| Pipe_Read blocking | Section 3 (Condition Variables - while loop) |
| Pipe_Write blocking | Section 3 (Condition Variables - while loop) |
| Pipe close/EOF detection | Section 4 (reader/writer counts) |
| Cond_Broadcast vs Signal | Section 3 (Signal vs Broadcast) |
| File reference counting | Section 5 (Atomic Operations) |
| Fork file inheritance | Section 5 (Reference counting pattern) |

## Red Flags

You're violating this principle if you:
- Write `while (condition) { Cond_Wait(...) }` without having discussed why `while` not `if`
- Use `Cond_Broadcast` without discussing when Signal vs Broadcast applies
- Implement atomic refcount operations without discussing the race they prevent
- Write pipe blocking logic without the student understanding the producer-consumer pattern

## Example Interaction

**Wrong approach:**
> Student: "Let's implement Pipe_Read"
> Claude: *[Writes complete function with correct while-loop and condition variable usage]*

**Correct approach:**
> Student: "Let's implement Pipe_Read"
> Claude: "Before we write this, we need to handle the case where the buffer is empty. Have we worked through the condition variable questions in Section 3 of docs/p1/concurrency.md? Specifically, there's an important question about using `while` vs `if` that directly affects how we write this function."
> *[Works through Section 3 questions with student]*
> Claude: "Now that you understand why we need the while loop, let's implement Pipe_Read..."

## Tracking Coverage

In `progress.md`, maintain a section for concept coverage:

```markdown
## Concepts Covered (docs/p1/concurrency.md)
- [x] Section 1: SMP context (why locks matter) - 2024-01-15
- [x] Section 3: Condition variables (while vs if) - 2024-01-15
- [x] Section 3: Signal vs Broadcast - 2024-01-15
- [ ] Section 5: Atomic operations
- [ ] Section 5: Reference counting pattern
```

This creates accountability and helps resume sessions correctly.
