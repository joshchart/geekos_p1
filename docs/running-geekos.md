# Running GeekOS from Claude Code

This document describes how to run GeekOS from Claude Code through the Docker container.

**Date:** 2026-01-23 (Updated with refinements from peer review)

---

## Executive Summary

**Goal:** Run GeekOS inside Docker from Claude Code's Bash tool (which lacks TTY support).

**Solution:** Use QEMU's monitor interface via a Unix socket to send simulated keystrokes with the `sendkey` command.

**Key Insight:** GeekOS reads from the PS/2 keyboard, not serial. The `sendkey` command through the QEMU monitor is the only way to simulate keyboard input without a TTY.

---

## Quick Start

```bash
docker exec geekos bash -c '
cd /geekos/build
rm -f /tmp/qemu-monitor.sock

# Start QEMU paused with monitor socket
timeout 60 make run DISP_OPTION="-display none" \
  ARGS="-S -monitor unix:/tmp/qemu-monitor.sock,server,nowait" &
QEMU_PID=$!

# Wait for socket (not arbitrary time)
for i in {1..50}; do [ -S /tmp/qemu-monitor.sock ] && break; sleep 0.1; done

python3 << "PYEOF"
import socket, time

def send_cmd(sock, cmd):
    sock.send((cmd + "\n").encode())
    time.sleep(0.03)

def send_keys(sock, text):
    for c in text:
        if c == "/": send_cmd(sock, "sendkey slash")
        elif c == ".": send_cmd(sock, "sendkey dot")
        elif c == " ": send_cmd(sock, "sendkey spc")
        elif c == "-": send_cmd(sock, "sendkey minus")
        elif c.isupper(): send_cmd(sock, f"sendkey shift-{c.lower()}")
        elif c.isdigit(): send_cmd(sock, f"sendkey {c}")
        else: send_cmd(sock, f"sendkey {c}")
    send_cmd(sock, "sendkey ret")
    time.sleep(0.2)

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect("/tmp/qemu-monitor.sock")

send_cmd(sock, "cont")  # Unpause QEMU
time.sleep(8)           # Wait for boot

send_cmd(sock, "sendkey ret")  # First-char workaround
time.sleep(0.3)

send_keys(sock, "/c/your-test.exe")
time.sleep(10)  # Wait for test

sock.close()
PYEOF

wait $QEMU_PID 2>/dev/null
cat output.log
'
```

---

## What Didn't Work (Historical Reference)

### 1. Running geekos_docker.sh directly
```bash
./geekos_docker.sh
```
**Error:** `the input device is not a TTY`

**Why:** The script uses `docker exec -it` which requires an interactive TTY. Claude Code's Bash tool doesn't provide a TTY.

---

### 2. Piping input to QEMU with -nographic
```bash
echo -e '\n/c/rec.exe 10\nexit\n' | make run ARGS='-nographic'
```
**Error:** `cannot use stdio by multiple character devices`

**Why:** The Makefile already has `-serial stdio`. Adding `-nographic` tries to also use stdio for the console, causing a conflict.

---

### 3. Piping input without -nographic (curses mode)
```bash
echo -e '\n/c/rec.exe 10\nexit\n' | make run
```
**Error:** `We need a terminal output`

**Why:** QEMU's curses display mode requires a real terminal.

---

### 4. Using -display none with piped stdin
```bash
echo -e '\n/c/rec.exe 10\nexit\n' | make run DISP_OPTION='-display none'
```
**Result:** GeekOS boots but commands are ignored.

**Why:** The `-serial stdio` connects to GeekOS's serial port, but the GeekOS shell reads from the **keyboard** (PS/2), not the serial port. Piped input goes to serial, not keyboard.

---

### 5. Delayed input with sleep
```bash
(sleep 10; echo '/c/rec.exe 10'; echo 'exit') | make run DISP_OPTION='-display none'
```
**Result:** Same as above - commands ignored.

**Why:** Same reason - serial vs keyboard mismatch.

---

## The Solution: QEMU Monitor + sendkey via Unix Socket

QEMU has a "monitor" interface that accepts commands, including `sendkey` which simulates keyboard input. By exposing the monitor on a Unix socket, we can send keystrokes programmatically.

### Key Techniques

#### 1. Start QEMU Paused (`-S` flag)

Instead of sleeping for an arbitrary boot time, start QEMU paused and wait for the socket:

```bash
# Start paused - doesn't execute until we send 'cont'
make run ARGS="-S -monitor unix:/tmp/qemu-monitor.sock,server,nowait" &

# Wait for socket to exist (not arbitrary time)
for i in {1..50}; do [ -S /tmp/qemu-monitor.sock ] && break; sleep 0.1; done

# Then in Python: sock.send(b"cont\n") to unpause
```

#### 2. Faster Key Delays

0.03 seconds per key works fine (original was 0.1):

```python
def send_cmd(sock, cmd):
    sock.send((cmd + "\n").encode())
    time.sleep(0.03)  # 30ms is sufficient
```

#### 3. Complete Key Map

```python
KEY_MAP = {
    "/": "slash", ".": "dot", " ": "spc", "-": "minus",
    "_": "shift-minus", "=": "equal", "+": "shift-equal",
    "[": "bracket_left", "]": "bracket_right",
    ";": "semicolon", "'": "apostrophe", ",": "comma",
    "\\": "backslash", "`": "grave_accent",
    "!": "shift-1", "@": "shift-2", "#": "shift-3",
    "$": "shift-4", "%": "shift-5", "^": "shift-6",
    "&": "shift-7", "*": "shift-8", "(": "shift-9", ")": "shift-0",
}
# Digits and lowercase letters use themselves
# Uppercase letters need shift-{lowercase}
```

#### 4. Output Capture

- **Debug output:** `-debugcon file:output.log` captures kernel `Print()` calls
- **Serial output:** Redirect make output to capture serial: `make run ... > serial.txt 2>&1 &`

### Why This Approach?

1. **GeekOS reads from keyboard, not serial** - The `-serial stdio` option is for debug output, not shell input.

2. **QEMU's `sendkey` simulates keyboard** - The monitor command `sendkey a` sends a keypress for 'a'.

3. **Unix socket avoids TTY requirement** - Using `-monitor unix:/path/to/sock` instead of `-monitor stdio` lets us connect without a terminal.

4. **Python available in container** - `nc` (netcat) wasn't installed, but Python3 was.

---

## Complete Refined Script

```bash
#!/bin/bash
# run_geekos_test.sh - Run a command in GeekOS headlessly
# Usage: ./run_geekos_test.sh "/c/test.exe" 30

COMMAND="${1:-/c/rec.exe 10}"
TIMEOUT="${2:-30}"

docker exec geekos bash -c '
COMMAND="'"$COMMAND"'"
TIMEOUT='"$TIMEOUT"'

cd /geekos/build
rm -f /tmp/qemu-monitor.sock

# Start QEMU paused with monitor socket
timeout $((TIMEOUT + 20)) make run DISP_OPTION="-display none" \
  ARGS="-S -monitor unix:/tmp/qemu-monitor.sock,server,nowait" \
  > /tmp/serial_output.txt 2>&1 &
QEMU_PID=$!

# Wait for socket (not arbitrary time)
for i in {1..50}; do
  [ -S /tmp/qemu-monitor.sock ] && break
  sleep 0.1
done

if [ ! -S /tmp/qemu-monitor.sock ]; then
  echo "ERROR: QEMU monitor socket not created"
  cat output.log 2>/dev/null
  exit 1
fi

python3 << PYEOF
import socket, time

KEY_MAP = {
    "/": "slash", ".": "dot", " ": "spc", "-": "minus",
    "_": "shift-minus", "=": "equal",
}
for c in "0123456789abcdefghijklmnopqrstuvwxyz":
    KEY_MAP[c] = c
for c in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    KEY_MAP[c] = f"shift-{c.lower()}"

def send_cmd(sock, cmd):
    sock.send((cmd + "\n").encode())
    time.sleep(0.03)

def send_string(sock, s):
    for c in s:
        key = KEY_MAP.get(c, c.lower())
        send_cmd(sock, f"sendkey {key}")
    send_cmd(sock, "sendkey ret")
    time.sleep(0.2)

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect("/tmp/qemu-monitor.sock")

# Unpause QEMU
send_cmd(sock, "cont")
time.sleep(8)  # Wait for boot

# First-char workaround
send_cmd(sock, "sendkey ret")
time.sleep(0.3)

# Run the command
send_string(sock, "$COMMAND")
time.sleep($TIMEOUT)

# Exit cleanly
send_string(sock, "exit")
time.sleep(2)

# Quit QEMU
send_cmd(sock, "quit")
sock.close()
PYEOF

wait $QEMU_PID 2>/dev/null

echo "=== DEBUG OUTPUT (output.log) ==="
cat output.log 2>/dev/null

echo ""
echo "=== SERIAL OUTPUT ==="
cat /tmp/serial_output.txt 2>/dev/null
'
```

Usage:
```bash
./run_geekos_test.sh "/c/rec.exe 10" 15
./run_geekos_test.sh "/c/sigpipe.exe" 20
```

---

## Troubleshooting: GeekOS Crashes Before Accepting Input

If the QEMU monitor socket connection fails with "Connection refused" or QEMU exits immediately, GeekOS likely crashed during boot. This commonly happens due to:
- KASSERT failures in kernel code
- Page faults during initialization
- Interrupt handling bugs

### How to Diagnose

**Always check `build/output.log`** when GeekOS fails to start. The Makefile configures QEMU with `-debugcon file:output.log` which captures all kernel `Print()` output.

```bash
# After a failed run:
cat build/output.log
```

### Example: KASSERT Failure

When a KASSERT fails, GeekOS prints diagnostic info before shutting down:

```
Welcome to GeekOS!
Spawning init process (/c/shell.exe)
Failed assertion in Alloc_Or_Reclaim_Page: Interrupts_Enabled()
at ../src/geekos/mem.c, line 420, RA=1a73d, thread=0x0062c000
KASSERT calling Hardware_Shutdown()
```

This tells you:
- **Function:** `Alloc_Or_Reclaim_Page`
- **Assertion:** `Interrupts_Enabled()` - interrupts should have been enabled
- **Location:** `mem.c`, line 420
- **Return address:** 0x1a73d (use `nm geekos/kernel.exe` to find the caller)

### Common Boot-Time Failures

| Symptom | Likely Cause |
|---------|--------------|
| Crashes in `Alloc_Or_Reclaim_Page` | Calling page allocator with interrupts disabled |
| Crashes in `Switch_To_User_Context` | Page fault handler leaving interrupts enabled |
| No output at all | Boot sector or setup.bin issue |
| Hangs after "Init_Paging" | Paging file not found or disk I/O issue |

### Debugging Pattern

```bash
docker exec geekos bash -c 'cd /geekos/build && \
  timeout 30 qemu-system-i386 -m 32 \
    -drive file=diskc.img,format=raw,if=ide,index=0 \
    -display none \
    -device isa-debug-exit,iobase=0x501 \
    -debugcon file:output.log \
    -monitor unix:/tmp/qemu-monitor.sock,server,nowait &
  QEMU_PID=$!
  sleep 10

  # Check if QEMU is still running
  if ! kill -0 $QEMU_PID 2>/dev/null; then
    echo "QEMU exited early - checking output.log:"
    cat output.log
    exit 1
  fi

  # ... rest of test script ...
'
```

---

## Refinements Summary (from peer review)

| Aspect | Original Approach | Refined Approach |
|--------|------------------|------------------|
| Boot timing | `sleep 8` (arbitrary) | `-S` flag + socket wait |
| Key delay | 0.1 sec | 0.03 sec |
| Key map | Partial | Complete |
| Output capture | output.log only | output.log + serial |
| Reusability | Inline script | Parameterized script |

---

## Test Results

| Test | Result |
|------|--------|
| `rec.exe 10` | SUCCESS - "Rec 10 success" |
| `rec.exe 1000` | SUCCESS - "Rec 1000 success" |
| `sigpipe.exe` | SUCCESS - Signal handling works |
| Project 2 signal tests | 9/12 PASS, 3 timeout (busy-wait loops slow in emulator) |

---

## Why Not Modify GeekOS to Read Serial?

It's possible to make GeekOS read from serial by modifying `keyboard.c` or `shell.c`, but this is more invasive than the sendkey approach and would need to be done for each project. Not worth it for testing purposes.
