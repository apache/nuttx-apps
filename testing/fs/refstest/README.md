# File and inode reference counting tests

Enable `CONFIG_TESTING_CMOCKA` and `CONFIG_TESTING_FS_REFSTEST` in a flat
build with pthreads, full descriptor inheritance, waitpid and child status
retention. Set `CONFIG_TESTING_FS_REFSTEST_MOUNTDIR` to an existing writable
directory. The filesystem must support unlinking open files; tmpfs and
writable pseudofiles are suitable. Allow at least eight unused descriptors.

For example, with tmpfs and mount commands enabled:

```text
nsh> mkdir /data
nsh> mount -t tmpfs /data
nsh> cmocka_fs_refstest
```

The command returns zero only when all enabled tests and their fixtures pass.
Each test owns a temporary directory and removes it after the test, including
after assertion failures. Existing files in the parent directory are retained.

The suite checks:

- Independent opens versus a shared file object after dup.
- dup2 onto itself, an occupied descriptor, and a released descriptor slot.
- Continued access through open descriptors after unlink.
- References held by epoll ADD and released by DEL or closing epoll.
- dup3 replacement, CLOEXEC flags, and rejection of identical descriptors.
- task_spawn inheritance and CLOEXEC exclusion, with semaphore synchronization.
- RAMMAP references acquired by mmap and released by munmap.
- Invalid descriptor errors without changing live reference counts.
- Four pthreads, each completing 100 dup/close pairs.

The RAMMAP case is registered only with `CONFIG_FS_RAMMAP=y`. It registers
a temporary character device with neither mmap nor XIP support, so filesystem
specific direct mappings cannot bypass the path under test. Mapping failure
fails the test. This case requires a writable `/dev` pseudofs directory.

The tests inspect NuttX internal file and inode counters and require
`CONFIG_BUILD_FLAT`. They exercise concurrency but do not prove race freedom
on every SMP target. Descriptor exhaustion is reported as a test failure.
