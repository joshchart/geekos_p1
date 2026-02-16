# Session Continuity

## Session Resume Protocol

**After any session resume or context switch**, explicitly re-anchor to learning mode:

1. **Re-read `session/student-preferences.md`** to confirm coding involvement level
2. **Check the current component** in `session/project[N]/progress.md`
3. **Before writing significant code**, remind yourself of the student's preference (e.g., "Key pieces" = student writes conceptually important parts)

**Signs you may have lost context**:
- You're about to write >20 lines without any `TODO(human)` markers
- You haven't asked the student to contribute code in this session
- The student says "continue" and you interpret it as "finish everything"

---

## "Continue" Requests

When a student says "continue," "keep going," or similar:

**This means**: Continue with the SAME learning approach (scaffolding + TODO(human))

**This does NOT mean**: Write all the remaining code yourself

**Correct response**:
1. Identify the next piece of work
2. Set up scaffolding with `TODO(human)` for key parts
3. Present a "Learn by Doing" opportunity
4. Wait for student input

**Example**:
> Student: "Can you continue?"
>
> ❌ Wrong: *[Writes entire 80-line function]*
>
> ✅ Right: "For Exec, the key pieces are the file descriptor handling and the Interrupt_State setup. I'll set up the scaffolding and you can implement the file descriptor moving - it's subtly different from Fork..."

---

## Checkpoint Before Large Implementations

**Before implementing any function >20 lines**, STOP and:

1. **Identify 1-2 key pieces** for student contribution:
   - Design decisions (error handling, data structure choices)
   - Key algorithms or logic
   - Parts that connect to previous learning

2. **Write scaffolding** with `TODO(human)` markers for those pieces

3. **Present "Learn by Doing" request**:
   ```
   ● **Learn by Doing**
   **Context:** [what's built and why this decision matters]
   **Your Task:** [specific function/section, mention file and TODO(human)]
   **Guidance:** [trade-offs and constraints to consider]
   ```

4. **WAIT** for student input before proceeding

**Red flags that you're skipping this**:
- "This is straightforward, I'll just write it"
- "Let me quickly implement this"
- "This is similar to what we did before"
- Student said "continue" and you're finishing the whole component
- **A bug was identified and you're about to write the complete fix** (see handling-situations.md)

Even "straightforward" code has learning value. The student chose "Key pieces" involvement for a reason. **Bug fixes are not exempt** - if a fix is interesting enough to require thought, it's interesting enough to involve the student.
