# AI-Tutored Programming Projects

This directory contains materials for running programming projects as AI-tutored learning experiences, where students work collaboratively with Claude to understand and implement complex systems.

---

## Overview

Traditional programming projects ask students to implement features given a specification. This approach augments that with structured dialogue:

- **Understand before implementing**: Students read and discuss relevant code before modifying it
- **Concept-code connection**: Every implementation is grounded in understanding the underlying concepts
- **Debugging as learning**: Errors are opportunities to practice investigation and hypothesis-testing
- **Flexible coding involvement**: Students choose how much code to write vs. have written for them

---

## Materials in This Directory

| File/Directory | Audience | Purpose |
|----------------|----------|---------|
| [guidelines/](guidelines/) | Claude (AI Tutor) | Specific guidance for how Claude should behave |
| [learning-triggers.md](learning-triggers.md) | Claude (AI Tutor) | Protocol for responding to hook-detected learning opportunities |
| [student-preferences.md](student-preferences.md) | Students | Record your background and preferences |
| [code-review-checklist.md](code-review-checklist.md) | Students | Checklist for reviewing your own code |
| `project0/`, `project1/`, ... | Students, Claude | Project-specific guides and progress tracking |
| [templates/](templates/) | Instructors | Templates for setting up new projects |

**Note**: Instructor-only materials (educational philosophy, component guide templates, session audits) are in `instructor/`.

---

## For Students

### Getting Started

1. **Set your output mode to "learning" or "explanatory"** - This activates Claude's tutor behaviors
2. **Tell Claude which project you're working on** - e.g., "I'm starting Project 1: Fork and Exec"
3. **Have a conversation** - Claude will guide you through understanding and implementing
4. **Ask questions** - Don't just accept explanations; engage actively
5. **Try debugging yourself first** - When things go wrong, form hypotheses before asking for help

### What to Expect

- Claude will ask you questions, not just give you answers
- You'll read existing code before writing new code
- You'll be asked to explain things back to verify understanding
- Tests passing isn't the end - you should understand *why* they pass
- You choose how much coding you want to do yourself

### Your Role

You're not a passive recipient of code. You're a collaborator in understanding the system. The best learning happens when you:

- Question things that don't make sense
- Form your own hypotheses about how things work
- Try to predict what will happen before running code
- Think about edge cases and potential bugs
- Explain concepts back in your own words

---

## For Instructors

See `instructor/` for instructor-only materials:

- `instructor/architecture/educational-approach.md` - Philosophy and principles
- `instructor/architecture/component-guide-template.md` - Template for creating component guides
- `instructor/performing-audit.md` - How to audit tutoring sessions
- `instructor/reset-instructions.md` - How to reset for a new student
- `instructor/experiences/` - Session audits and experiment analyses

---

## For Claude (When Tutoring)

See [guidelines/](guidelines/) for all guidance on tutoring behavior.

---

## Philosophy Summary

> Students should never write code they don't understand, and should never understand concepts without seeing how they manifest in code.

The goal isn't to produce working code (though that happens). The goal is for students to:

1. Understand the concepts deeply
2. Connect concepts to real code
3. Develop debugging skills
4. Build confidence in their understanding
5. Learn to reason about correctness

Working code is a byproduct of understanding, not a substitute for it.
