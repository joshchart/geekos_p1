# Session Context: AI-Tutored GeekOS

**Purpose**: This file provides context for Claude when starting a new session. It should be read at the beginning of each conversation.

**Last updated**: *(date)*

---

## Current Status

### Getting Started

This is a fresh session. Claude should follow these steps:

1. **Check onboarding status** in `session/student-preferences.md`:
   - Look at the "Session Onboarding" checkboxes
   - If any items are unchecked, complete onboarding first (see below)

2. **Determine project to work on**:
   - Scan for `docs/p*/` directories (e.g., `docs/p0/`, `docs/p1/`, `docs/p2/`)
   - Available projects:
     - **Project 0** (`docs/p0/`): Pipes - VFS layer introduction
     - **Project 1** (`docs/p1/`): Fork and Exec - Process creation (requires P0)
   - If student hasn't completed earlier projects, start with the earliest
   - If unclear, ask the student which project they'd like to work on

3. **Initialize project files**:
   - Create `session/project[N]/` directory if needed
   - Copy templates for `progress.md` and `experiences.md`
   - Read project docs (`docs-p[N]/`) to populate component list
   - Update this file with project status

### Onboarding Checklist

If onboarding is incomplete, discuss these with the student:

- [ ] **Claude makes mistakes**: Review code carefully before submitting. Claude can introduce bugs or misunderstand requirements.
- [ ] **Usage limits**: There may be rate limits. Start assignments early to avoid last-minute issues.
- [ ] **Joint effort**: This is collaborative learning. Ask questions, push back, and guide the direction.

After discussing, mark the checkboxes in `session/student-preferences.md`.

---

## Project Status

**Current project**: *(Not yet selected)*

**Status**: *(Pending project selection)*

**Project progression**:
- Project 0 (Pipes): ⏸️ Not started
- Project 1 (Fork/Exec): ⏸️ Not started (requires P0)

---

## Next Steps

1. Complete onboarding (if needed)
2. Select project (P0 if new, or continue from where student left off)
3. Read project documentation
4. Begin first component

---

## Key Files

| File | Purpose | When to Read/Update |
|------|---------|---------------------|
| `CLAUDE.md` | Main instructions | Read once per session |
| `session/nextSteps.md` | Session context | Read at start, update when changing direction |
| `session/student-preferences.md` | Student profile | Read at start, update when preferences revealed |
| `session/code-review-checklist.md` | Coding standards | Reference when writing/reviewing code |
| `session/project[N]/progress.md` | Current status | Read at start, update frequently |
| `session/project[N]/experiences.md` | Learning journal | Read at start, add significant moments |
| `tutor/guidelines/` | Tutoring approach | Reference specific files as needed |
| `tutor/templates/` | Session reset templates | Use when starting fresh |

---

## Notes for Claude

- **Check onboarding first** - Don't assume the student knows how to work with you
- **Follow the tutoring approach**: Ask before assuming, progressive disclosure, let students discover
- **This is education, not just completion** - understanding matters more than passing tests
- **The student drives pacing** - some move fast, some slow, both are fine
- **docs-pN/ is source of truth** - initialize session files from project documentation
- **Project order matters** - P0 must be done before P1, P1 before P2, etc.
