# GeekOS Development Environment Setup

**CMSC412: Operating Systems**

This guide covers setting up the development environment for GeekOS on various platforms.

## Quick Start (Docker - Recommended)

Docker provides a consistent, pre-configured environment across all platforms.

### Prerequisites

1. **Install Docker** from https://www.docker.com/
   - Download the version appropriate for your operating system
   - For Windows users using WSL: Download the Windows version and run it on the host

### Platform-Specific Notes

#### Apple Silicon (M1/M2/M3 Macs)

> **Note**: The following Rosetta advice is **under review**. Some users have reported that GeekOS works fine with Rosetta enabled. If you encounter issues, try disabling Rosetta as described below, but it may not be necessary.

For **Docker 4.26 or higher**:
- Go to Settings → General
- Optionally uncheck "Use Rosetta for x86/amd64 emulation on Apple Silicon"

For **Docker versions below 4.26**:
- Go to Settings → General
- Change file sharing to **gRPC FUSE**
- Optionally uncheck "Use Virtualization framework"

**Background on Rosetta**: There are two layers of emulation involved:
1. Docker runs a Linux container (either ARM64 native or x86_64 via Rosetta)
2. Inside the container, QEMU emulates x86 hardware for GeekOS

With Rosetta disabled, Docker runs an ARM64 Linux container natively, and QEMU (compiled for ARM64) performs full software emulation of x86. With Rosetta enabled, Docker runs an x86_64 container translated through Rosetta, with QEMU then also emulating x86. The original advice suggested this could create conflicts, but in practice both configurations appear to work.

#### Windows (WSL)

- WSL-1 is **not compatible** with Docker; ensure you're running WSL-2
- In Docker Desktop: Settings → Resources → WSL Integration must be **enabled**
- **Important**: Place the GeekOS folder inside the Ubuntu/WSL filesystem (not on the Windows filesystem), otherwise builds will be extremely slow

### Running GeekOS in Docker

```bash
# Navigate to your geekos directory
cd /path/to/geekos

# Start the Docker container (builds image if needed)
./geekos_docker.sh

# You are now inside the Docker container
# Build and run GeekOS:
cd build
make
make run
```

### Docker Container Management

```bash
# Remove the container (if you need to reset)
docker container rm geekos

# Remove the image (to force rebuild)
docker image rm geekos
```

---

## Alternative: Native Linux (x86_64)

If you prefer running natively on Linux:

### Required Packages (Ubuntu/Debian)

```bash
sudo apt-get install build-essential nasm libc6-dev-i386 qemu-system
```

Then build normally:

```bash
cd build
make
make run
```

---

## Alternative: Native macOS

### Required Tools (via MacPorts)

```bash
sudo port install i386-elf-gcc i386-elf-binutils nasm qemu
```

---

## Build Commands Reference

All commands must be run from the `build/` directory:

| Command | Description |
|---------|-------------|
| `make` | Build kernel and disk images |
| `make clean` | Remove build artifacts |
| `make run` | Run GeekOS in QEMU |
| `make dbgrun` | Run with GDB debugging (starts paused) |
| `make dbg` | Start GDB for debugging (run in separate terminal) |
| `make depend` | Generate header dependencies |

---

## IDE Setup (Optional)

VSCode works well for GeekOS development. Useful extensions:
- **C/C++** (Microsoft) - IntelliSense, debugging
- **x86 and x86_64 Assembly** - Syntax highlighting for .asm files

Note: The course staff may not provide IDE-specific support.

---

## Troubleshooting

### "Permission denied" running geekos_docker.sh

```bash
chmod +x geekos_docker.sh
```

### Docker command not found in WSL

Ensure Docker Desktop is running on Windows and WSL integration is enabled in Docker settings.

### Build is extremely slow on Windows/WSL

Move your GeekOS folder into the Linux filesystem:
```bash
# From within WSL
mv /mnt/c/Users/yourname/geekos ~/geekos
```

### QEMU window doesn't appear

- On macOS: Ensure XQuartz or another X11 server is running
- On WSL: Install an X server like VcXsrv or use WSLg (Windows 11)
