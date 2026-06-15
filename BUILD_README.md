# Aquarium GTK Application - Build and Run Guide

## Problem Fixed

The application was failing with this error:
```
./aquarium: symbol lookup error: /snap/core20/current/lib/x86_64-linux-gnu/libpthread.so.0: 
undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE
```

### Root Cause
When running from VS Code (which is installed as a snap), the environment becomes contaminated with snap-specific library paths. This causes GTK3 and pthread libraries to load from the snap environment, which creates version incompatibilities with the system's glibc.

### Solution Applied

**Two fixes have been implemented:**

1. **Recompiled the binary** with a clean compilation using the system's gcc and libraries (no snap contamination)
2. **Created a wrapper script** (`run_aquarium.sh`) that clears problematic snap-related environment variables before running the binary

## Building the Application

### Prerequisites
Ensure you have the required development libraries installed:
```bash
sudo apt install libgtk-3-dev libjson-c-dev
```

### Build Instructions

#### Option 1: Using the Makefile (Recommended)
```bash
make clean
make
```

To rebuild from scratch:
```bash
make rebuild
```

#### Option 2: Manual Compilation
```bash
gcc -o aquarium main.c aquarium.c structures.c -Wall -pthread \
    $(pkg-config --cflags --libs gtk+-3.0 json-c) -lm
```

## Running the Application

### Method 1: Using the Wrapper Script (Recommended)
```bash
./run_aquarium.sh
```

This automatically clears snap environment variables that interfere with library loading.

### Method 2: Direct Execution
If you're running from a non-snap terminal environment, you can run directly:
```bash
./aquarium
```

### Method 3: Manual Environment Cleanup
If you need to run from VS Code's terminal, you can manually clean the environment:
```bash
unset GTK_PATH GTK_EXE_PREFIX GTK_IM_MODULE_FILE LOCPATH GIO_MODULE_DIR \
      GSETTINGS_SCHEMA_DIR XDG_DATA_HOME SNAP
./aquarium
```

## Troubleshooting

### Still Getting Symbol Lookup Error?
1. Make sure you're using the wrapper script: `./run_aquarium.sh`
2. Or rebuild the binary: `make clean && make`
3. Check that GTK3 development libraries are installed properly

### Display Issues
If you get "cannot open display", ensure:
- You have an X server running
- The `DISPLAY` variable is set correctly (usually `:0` or `:1`)

### Missing Libraries
If you get "cannot find library" errors during compilation, install the missing dev packages:
```bash
sudo apt install libgtk-3-dev libjson-c-dev libcairo2-dev libpango1.0-dev
```

## Files Modified

- `aquarium.c`, `structures.c`, `main.c` - Recompiled with clean environment
- `run_aquarium.sh` - New wrapper script for safe execution
- `Makefile` - New build configuration file

## Notes

- The wrapper script `run_aquarium.sh` is the recommended way to run the application
- The Makefile ensures consistent, clean builds without snap interference
- The recompiled binary includes proper system library linking
