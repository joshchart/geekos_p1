# Learning Triggers: Hook Response Protocol

This document describes how to respond when a hook sends you a `[POSSIBLE_LEARNING_OPPORTUNITY]` message.

## Why This Exists

When solving problems, you naturally think through options, trade-offs, and pivots. These decision points are often the most valuable learning moments for students—but they can slip by unshared because the pattern feels "obvious" from training data (idiom blindness) or because you're focused on solving the problem.

A hook script monitors your thinking trace for "trigger phrases" that indicate decision points. When it detects them, it sends you a `[POSSIBLE_LEARNING_OPPORTUNITY]` message prompting you to pause and evaluate whether to involve the student.

---

## The Protocol

### When You Receive `[POSSIBLE_LEARNING_OPPORTUNITY]`

1. **Review** what you were just thinking about (the hook detected trigger phrases in your reasoning)
2. **Decide** if this is pedagogically valuable for the student
3. **Respond** with one of the markers below

### Response Markers

| Marker | When to Use |
|--------|-------------|
| `[LEARNING_OPPORTUNITY_IDENTIFIED]` | Worth discussing with student—pause and engage them |
| `[LEARNING_OPPORTUNITY_CONSIDERED]` | Evaluated but not pedagogically valuable—continue normally |

Include the **kind** and **confidence** from the trigger, plus a brief explanation.

### After Student Interaction

Add an outcome marker for audit:

```
[LEARNING_OPPORTUNITY_OUTCOME] Kind: pivot, Confidence: very_high.
Discussed the race condition options. Student correctly identified
that disabling preemption wouldn't work on SMP. Good engagement.
```

---

## Trigger Kinds and Confidence

The hook reports what triggered it. Use this to calibrate your response.

### Confidence Levels

| Level | Meaning | Action |
|-------|---------|--------|
| `very_high` | Almost always indicates a real decision point | Strong signal—evaluate seriously |
| `high` | Usually indicates valuable discussion | Worth careful consideration |
| `medium` | May or may not be interesting | Use judgment; often okay to skip |

### Trigger Kinds

| Kind | What It Signals | Examples |
|------|-----------------|----------|
| `pivot` | You hit a constraint and changed approach—the constraint itself is valuable learning | "but wait", "actually that won't work", "on second thought" |
| `option_enumeration` | You're weighing alternatives—students should participate | "the options are", "we could either", "alternatively" |
| `tradeoff` | Explicit cost/benefit analysis | "the trade-off is", "the downside is" |
| `rushing` | You may be skipping discussion to implement quickly | "let me just", "I'll just", "obviously" |
| `challenge` | Non-trivial complexity identified (may or may not be pedagogically interesting) | "the challenge is", "the tricky part is" |
| `realization` | Discovered something non-obvious | "the issue is", "I realize", "looking at this more carefully" |
| `uncertainty` | Design space where multiple approaches are valid | "I'm not sure if", "the question is whether" |

---

## Example: The "But Wait" Trigger

"But wait" is the most common pivot phrase (66 occurrences across 33 sessions). It typically catches important subtleties.

### Scenario

You're implementing Fork and your thinking includes:
> "I'll copy the parent's file descriptors to the child. But wait—if they share the same struct File, we need reference counting or the child closing a file would invalidate the parent's descriptor."

The hook analyzes your thinking trace, detects the trigger phrase, and sends you:
`[POSSIBLE_LEARNING_OPPORTUNITY] Detected: "but wait" (pivot, very_high)`

### Worth Discussing?

**Yes!** This is exactly the kind of insight students should understand:
- The naive approach (just copy pointers) has a subtle bug
- The constraint (shared resources need lifetime management) is general
- Reference counting is a pattern they'll use throughout OS work

### Response

```
[LEARNING_OPPORTUNITY_IDENTIFIED] Kind: pivot, Confidence: very_high.
The shared file descriptor issue reveals why Fork needs reference counting.
```

Then pause and engage the student:
> "I was about to copy file descriptors, but I realized there's a subtle issue. What happens if the child closes a file that the parent also has open? Can you think of what might go wrong?"

### Not Worth Discussing

If the "but wait" was about something trivial:
> "But wait—I need to include the header file first."

```
[LEARNING_OPPORTUNITY_CONSIDERED] Kind: pivot, Confidence: very_high.
The pivot was about a missing include, not a conceptual issue.
```

Continue normally.

---

## Decision Criteria

### Identify the opportunity when:

- The decision involves trade-offs the student should understand
- The constraint or insight is generalizable to other problems
- Multiple valid approaches exist and the student should think through them
- The student would benefit from understanding *why*, not just *what*

### Consider but skip when:

- The trigger was about syntax, typos, or trivial issues
- The concept is too advanced for the student's current level
- You already discussed this concept recently
- The student is stuck and needs momentum more than depth
- It's genuinely obvious and discussing it would feel patronizing

---

## Marker Placement

- **Thinking blocks** (preferred): Keeps markers out of conversation clutter
- **User-facing response**: When discussing the opportunity explicitly
- **Outcome markers**: In thinking or at end of response, after interaction

---

## See Also

- `instructor/thinking-trace-hooks/` — Full documentation of the hook system
- `tutor/guidelines/` — General tutoring guidelines (see README.md for index)
