# Instructor Code Review Checklist

This checklist contains instructor-provided coding patterns and common issues to watch for in GeekOS projects. These are patterns that frequently cause bugs or are important for understanding.

**How to use:** Reference this during code reviews. Claude should check each applicable item against the code being reviewed.

---

## Type Safety

- **Match types for loop counters with comparison targets** – When a loop counter is compared against a user-provided value (like `numBytes`), use the same type for both to avoid overflow bugs. Cast at the end when returning, with a comment explaining why it's safe.
  ```c
  // Good: ulong_t bytesRead compared to ulong_t numBytes
  ulong_t bytesRead = 0;
  while (bytesRead < numBytes && ...) { ... }
  return (int)bytesRead;  /* Safe: bytesRead <= BUFFER_SIZE */

  // Bad: cast can cause (int)numBytes to go negative if numBytes > INT_MAX
  int bytesRead = 0;
  while (bytesRead < (int)numBytes && ...) { ... }  // Bug!
  ```

---

## Synchronization

*(Items to be added by course instructor)*

---

## Memory Management

*(Items to be added by course instructor)*

---

## Error Handling

*(Items to be added by course instructor)*

---

*Maintained by course instructors. Students develop their own patterns in `session/code-review-checklist.md`.*
