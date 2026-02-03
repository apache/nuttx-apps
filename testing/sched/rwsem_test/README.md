# Reader-Writer Semaphore Test Suite

## Overview

This directory contains a comprehensive test suite for NuttX reader-writer semaphore (rwsem) optimization.

**Commit ID**: `Id3b49dda0309b098f04e8ab499c28c94fe1f77ce`

## Directory Structure

```
rwsem_test/
├── CMakeLists.txt              # CMake build configuration
├── Kconfig                     # Configuration options
├── Make.defs                   # Make build system integration
├── Makefile                    # Make build rules
├── README.md                   # This file
├── rwsem_comprehensive_test.c  # Test source code
├── RWSEM_TEST_COVERAGE.md      # Test coverage details
├── RWSEM_TEST_SUMMARY.md       # Test suite summary
└── RWSEM_TEST_USAGE.md         # Usage instructions
```

## Quick Start

### 1. Configuration

```bash
make menuconfig
```

Navigate to:
```
Application Configuration
  └─> Testing
      └─> Sched
          └─> [*] Reader-Writer Semaphore Comprehensive Test
```

### 2. Build

```bash
make clean
make
```

### 3. Run

```bash
nsh> rwsem_comprehensive_test
```

## Test Contents

This test suite contains 10 comprehensive tests:

### Core Correctness Tests (Community Required)
1. Multiple readers with no writers
2. Multiple writers (exclusive access)
3. Mixed reader-writer access patterns
4. Waiter wake-up correctness
5. Lock holder tracking
6. Context switch reduction verification

### Performance Tests
7. Recursive write lock performance
8. Converted lock performance
10. High contention multi-threaded performance

### Basic Functionality Tests
9. Basic operations

## Optimization Description

This optimization reduces unnecessary context switches by only calling `up_wait()` when the lock is actually available:

- **up_write() optimization**: Only call `up_wait()` when `writer` reaches 0
- **up_read() optimization**: Only call `up_read()` when `reader` reaches 0

## Documentation

- **RWSEM_TEST_USAGE.md** - Detailed usage instructions and troubleshooting
- **RWSEM_TEST_COVERAGE.md** - Complete test coverage matrix
- **RWSEM_TEST_SUMMARY.md** - Test suite summary and technical analysis

## Configuration Options

Configurable in `make menuconfig`:

- `CONFIG_TESTING_RWSEM_TEST` - Enable/disable rwsem test
- `CONFIG_TESTING_RWSEM_TEST_PRIORITY` - Test task priority (default 100)
- `CONFIG_TESTING_RWSEM_TEST_STACKSIZE` - Test stack size (default 8192)

## Expected Results

All tests should pass with output similar to:

```
========================================
RWSem Comprehensive Test Suite
========================================
...
========================================
ALL TESTS PASSED
========================================
```

## Performance Metrics

Tests report the following metrics:

- Execution time
- Average time per operation
- Throughput (ops/ms)

## Production Validation

This optimization has been validated in production on **Vela OS** (Xiaomi's NuttX-based embedded OS) for months, running on:

- Wearable devices
- IoT devices
- Automotive systems

## Related Links

- GitHub PR: https://github.com/apache/nuttx/pull/18210
- Commit: Id3b49dda0309b098f04e8ab499c28c94fe1f77ce

## Contact

For questions, please ask in GitHub PR #18210.
