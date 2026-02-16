# Maintaining Session Files

## Maintain Student Preferences

**Always read `session/student-preferences.md` at session start** to understand the student's background, preferences, and history.

**Update the file immediately when**:
- Student expresses a preference ("I'd like to try writing this myself")
- Student reveals background ("I've used semaphores in Java before")
- You learn something about their learning style (they respond well to hints vs. explanations)
- Completing a component or project (record progress and challenges)

**Ask about preferences**:
- At the **start of a new project**: "Would you like to revisit your learning preferences for this project?"
- When you notice a **mismatch**: If student wants more/less code to write, or different pacing
- **Periodically**: "How's the pacing? Should I adjust anything?"

**Example updates**:
```
# When student says "I'm pretty comfortable with C"
- Update "C experience: Comfortable with pointers, structs, manual memory management"

# When student says "Could you explain that in more detail?"
- Update "Explanation style: Prefers detailed explanations before diving in"

# After they struggle with a concept
- Add to "Challenges encountered": "Struggled with circular buffer indexing in pipes"
```

## Maintain Code Review Checklist

**Record student coding preferences in `session/code-review-checklist.md`** when students give instructions about how to write or review code.

**When to update:**
- Student identifies a pattern to watch for ("always check interrupt status is restored")
- Student requests a coding convention ("add comments when casting unsigned to signed")
- A debugging session reveals a category of bugs to avoid
- Student develops a preference through discussion

**Examples of checklist items:**
- "When casting from unsigned to signed int, precede with a comment explaining why it's safe"
- "Ensure interrupt-enabled status is the same at function entry and exit unless specifically documented"
- "If using Cond_Signal rather than Cond_Broadcast, add a comment explaining why"
- "Before guessing at bugs, add instrumentation to gather more information"

**Reference the checklist** when writing or reviewing code. This is the student's personal coding standards document.

## Maintain Project Files

**For each project, maintain two files**: `progress.md` (current status) and `experiences.md` (learning journal).

### progress.md Updates

**Update `session/project[N]/progress.md` when**:
- Starting or completing a component
- Starting or ending a session
- Tests pass or fail
- Encountering or resolving blockers

**Keep it current** - this is your "where are we now?" file. Update the "Last updated" timestamp.

### experiences.md Updates

**Add to `session/project[N]/experiences.md` when significant moments occur**:

**Quick Win** (add immediately):
- Student understands a concept in <5 minutes without confusion
- Example: "Understood reference counting immediately, mentioned C++ smart pointers"

**Conceptual Challenge** (add after resolution):
- Concept required >10 minutes of discussion or multiple explanations
- Example: "Confused about pipe readers vs refCount for 15 minutes, understood after diagram"

**Bug** (add after debugging):
- Took >5 minutes to debug OR revealed conceptual misunderstanding
- Include: context, symptom, root cause, time, learning
- Example: "Forgot mutex unlock in error path, system hung, 20 min to find"

**Successful Approach** (add when effective):
- Student explicitly praised approach, or showed notably better understanding
- Example: "Progressive disclosure worked well - saw structure before details"

**Confusing Presentation** (add when clarification needed):
- Required >5 minutes of clarification OR student expressed confusion
- Include suggested improvement for instructors
- Example: "vtable dispatch unclear, took 10 min to explain, suggest adding to guide"

**Don't record everything** - focus on moments that inform future learning or teaching. A typo fixed in 30 seconds doesn't need recording.

### Review Notes

**When student reviews material** (exam prep, project review):
- Add review notes to relevant entries in `experiences.md`
- Note current confidence level
- Note whether concept needs revisiting
- Example: "*[Review notes] Reviewed 3/10/26 (midterm prep): Student explained correctly. Confidence high.*"
