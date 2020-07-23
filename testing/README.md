# Testing

The `apps/testing` directory is used to build NuttX-specific tests and to
include external testing frameworks.

There is overlap between what you will find in `apps/examples` and
`apps/testing` in the sense that there are also tests in `apps/examples` as
well. Those tests, however, can also be used to illustrate usage of a NuttX
feature. Most of the tests in `apps/testing`, on the other hand, are pure tests
with little value as usage examples.

## `cxxtest`

This is a test of the C++ standard library. At present a port of the uClibc++
C++ library is available. Due to licensing issues, the uClibc++ C++ library is
not included in the NuttX source tree by default, but must be installed (see the
`README.txt` file in the uClibc++ download package for installation).

The uClibc++ test includes simple test of:

- iostreams,
- STL,
- RTTI, and
- Exceptions

### Example Configuration Options

- `CONFIG_TESTING_CXXTEST=y` – Eanbles the example

### Other Required Configuration Settings

Other NuttX setting that are required include:

- `CONFIG_HAVE_CXX=y`
- `CONFIG_HAVE_CXXINITIALIZE=y`
- `CONFIG_UCLIBCXX=y` or `CONFIG_LIBCXX=y`

Additional uClibc++/libcxx settings may be required in your build environment.

## `fstest`

This is a generic file system test that derives from `testing/nxffs`. It was
created to test the tmpfs file system, but should work with any file system
provided that all initialization has already been performed prior to starting
the test.

This test a a general test for any file system, but includes some specific hooks
for the SPIFFS file system.

- `CONFIG_TESTING_FSTEST` – Enable the file system example.
- `CONFIG_TESTING_FSTEST_MAXNAME` – Determines the maximum size of names used in
  the filesystem.
- `CONFIG_TESTING_FSTEST_MAXFILE` – Determines the maximum size of a file.
- `CONFIG_TESTING_FSTEST_MAXIO` – Max I/O, default `347`.
- `CONFIG_TESTING_FSTEST_MAXOPEN` – Max open files.
- `CONFIG_TESTING_FSTEST_MOUNTPT` – Path where the file system is mounted.
- `CONFIG_TESTING_FSTEST_NLOOPS` – Number of test loops. default `100`.
- `CONFIG_TESTING_FSTEST_VERBOSE` – Verbose output.

## `mm`

This is a simple test of the memory manager.

## `nxffs`

This is a test of the NuttX NXFFS FLASH file system. This is an NXFFS stress
test and beats on the file system very hard. It should only be used in a
simulation environment! Putting this NXFFS test on real hardware will most
likely destroy your FLASH. You have been warned.

## `ostest`

This is the NuttX _qualification_ suite. It attempts to exercise a broad set of
OS functionality. Its coverage is not very extensive as of this writing, but it
is used to qualify each NuttX release.

The behavior of the `ostest` can be modified with the following settings in the
`boards/<arch>/<chip>/<board>/configs/<config>/defconfig` file:

- `CONFIG_NSH_BUILTIN_APPS` – Build the OS test example as an NSH built-in
    application.
- `CONFIG_TESTING_OSTEST_LOOPS` – Used to control the number of executions of
    the test. If undefined, the test executes one time. If defined to be zero,
    the test runs forever.

- `CONFIG_TESTING_OSTEST_STACKSIZE` – Used to create the ostest task. Default is
    `8192`.
- `CONFIG_TESTING_OSTEST_NBARRIER_THREADS` – Specifies the number of threads to
    create in the barrier test. The default is 8 but a smaller number may be
    needed on systems without sufficient memory to start so many threads.

- `CONFIG_TESTING_OSTEST_RR_RANGE` – During round-robin scheduling test two
    threads are created. Each of the threads searches for prime numbers in the
    configurable range, doing that configurable number of times. This value
    specifies the end of search range and together with number of runs allows to
    configure the length of this test – it should last at least a few tens of
    seconds. Allowed values `[1; 32767]`, default `10000`.

- `CONFIG_TESTING_OSTEST_RR_RUNS` – During round-robin scheduling test two
    threads are created. Each of the threads searches for prime numbers in the
    configurable range, doing that configurable number of times.

## `smart` SMART File System

This is a test of the SMART file system that derives from `testing/nxffs`.

- `CONFIG_TESTING_SMART` – Enable the SMART file system example.

- `CONFIG_TESTING_SMART_ARCHINIT` – The default is to use the RAM MTD device at
  `drivers/mtd/rammtd.c`. But an architecture-specific MTD driver can be used
  instead by defining `CONFIG_TESTING_SMART_ARCHINIT`. In this case, the
  initialization logic will call `smart_archinitialize()` to obtain the MTD
  driver instance.

- `CONFIG_TESTING_SMART_NEBLOCKS` – When `CONFIG_TESTING_SMART_ARCHINIT` is not
  defined, this test will use the RAM MTD device at `drivers/mtd/rammtd.c` to
  simulate FLASH. In this case, this value must be provided to give the number
  of erase blocks in MTD RAM device. The size of the allocated RAM drive will
  be: `CONFIG_RAMMTD_ERASESIZE * CONFIG_TESTING_SMART_NEBLOCKS`.

- `CONFIG_TESTING_SMART_MAXNAME` – Determines the maximum size of names used in
  the filesystem.

- `CONFIG_TESTING_SMART_MAXFILE` – Determines the maximum size of a file.
- `CONFIG_TESTING_SMART_MAXIO` – Max I/O, default `347`.
- `CONFIG_TESTING_SMART_MAXOPEN` – Max open files.
- `CONFIG_TESTING_SMART_MOUNTPT` – SMART mountpoint.
- `CONFIG_TESTING_SMART_NLOOPS` – Number of test loops. default `100`.
- `CONFIG_TESTING_SMART_VERBOSE` – Verbose output.

## `smart_test` SMART File System

Performs a file-based test on a SMART (or any) filesystem. Validates seek,
append and seek-with-write operations.

* `CONFIG_TESTING_SMART_TEST=y`

```
Author: Ken Pettit
  Date: April 24, 2013
```

Performs a file-based test on a SMART (or any) filesystem. Validates seek,
append and seek-with-write operations.

```
Usage:

  flash_test mtdblock_device

Additional options:

  --force                     to replace existing installation
```

## `smp`

This is a simple test for SMP functionality. It is basically just the pthread
barrier test with some custom instrumentation.

## `unity`

Unity is a unit testing framework for C developed by ThrowTheSwitch.org:

http://www.throwtheswitch.org/unity
