# shared-memory-ipc-poc

Proof-of-concept for benchmarking inter-process communication using shared memory vs. pipes.
## Prerequisites

- GNU make
- g++ (C++11 or later)
- POSIX-compatible system for shared memory (e.g., Linux)
## Usage

**Running:**

To build and run both benchmarks:
```bash
make run
```

This will compile the executables into `./bin`:
- `bin/shared_benchmark`
- `bin/pipe_benchmark`

**Cleaning:**

To remove compiled binaries:
```bash
make clean
```
