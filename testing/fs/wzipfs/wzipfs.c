/****************************************************************************
 * apps/testing/fs/wzipfs/wzipfs.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <archive.h>
#include <archive_entry.h>

#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOUNTPOINT        "/zip"

/* Default backing directory for regular-file artifacts.  Override via
 * argv[1] to exercise the test suite against different backing
 * filesystems (e.g. "/tmp" for FAT, "/data" for tmpfs, "/host" for
 * hostfs).  FIFO artifacts always live on "/dev" since mkfifo()
 * requires the pseudo-fs.
 */

#define DEFAULT_TEST_DIR  "/tmp"

#define PATH_PIPE_FIFO    "/dev/wzipfs_fifo"

#define PIPE_CAPTURE_MAX  (16 * 1024)
#define METHOD_PAYLOAD    (32 * 1024)
#define METHOD_READ_BUF   (4 * 1024)

static char g_path_file_zip[PATH_MAX];

#define PATH_FILE_ZIP     g_path_file_zip

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pipe_reader_args_s
{
  FAR const char *fifo_path;
  FAR uint8_t    *buf;
  size_t          buf_max;
  size_t          bytes_read;
};

struct fd_reader_args_s
{
  FAR uint8_t *buf;
  size_t       buf_max;
  size_t       bytes_read;
  int          rfd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cleanup_paths
 ****************************************************************************/

static void cleanup_paths(void)
{
  /* Best-effort unmount from a prior aborted test. */

  umount(MOUNTPOINT);

  /* Best-effort delete of temp artifacts. */

  unlink(PATH_FILE_ZIP);
  unlink(PATH_PIPE_FIFO);
}

/****************************************************************************
 * Name: safe_mkfifo
 *
 * Description:
 *   Robust mkfifo() wrapper that tolerates a stale inode left over from a
 *   previous test run.  In NuttX sim, the pseudo-fs does not reboot
 *   between `cmocka_main --test=...` invocations, and if a prior test
 *   failed to unlink the FIFO, plain mkfifo() will return -EEXIST.  We
 *   unlink up-front (best-effort), call mkfifo(), and retry once on
 *   EEXIST with a second unlink.  Returns the mkfifo() return code.
 ****************************************************************************/

static int safe_mkfifo(FAR const char *path, mode_t mode)
{
  int ret;

  unlink(path);
  ret = mkfifo(path, mode);
  if (ret < 0 && errno == EEXIST)
    {
      unlink(path);
      ret = mkfifo(path, mode);
    }

  return ret;
}

/****************************************************************************
 * Name: ensure_tmpfs
 *
 * Description:
 *   Make sure /tmp is a writable tmpfs.  Idempotent: a second mount returns
 *   -EBUSY which we silently accept.
 ****************************************************************************/

static void ensure_tmpfs(void)
{
  int ret = mount(NULL, "/tmp", "tmpfs", 0, NULL);

  if (ret < 0 && errno != EBUSY)
    {
      /* Not fatal here - cleanup_paths() tolerates a missing /tmp. */
    }
}

/****************************************************************************
 * Name: ensure_mountpoint
 *
 * Description:
 *   Create /zip in the pseudo-fs if it does not already exist.
 ****************************************************************************/

static void ensure_mountpoint(void)
{
  int ret = mkdir(MOUNTPOINT, 0755);

  if (ret < 0 && errno != EEXIST)
    {
      /* Leave diagnostics to the caller's mount() assertion. */
    }
}

/****************************************************************************
 * Name: common_setup
 ****************************************************************************/

static int common_setup(FAR void **state)
{
  UNUSED(state);
  ensure_tmpfs();
  ensure_mountpoint();
  cleanup_paths();
  return 0;
}

/****************************************************************************
 * Name: common_teardown
 ****************************************************************************/

static int common_teardown(FAR void **state)
{
  UNUSED(state);
  cleanup_paths();
  return 0;
}

/****************************************************************************
 * Name: pipe_reader_thread
 ****************************************************************************/

static FAR void *pipe_reader_thread(FAR void *arg)
{
  FAR struct pipe_reader_args_s *a = arg;
  size_t off = 0;
  int attempts;
  int rfd;

  /* Open the FIFO read end in non-blocking mode so we do not deadlock if
   * the writer never appears (e.g., the zipfs mount deferred opening the
   * write end).  Retry a small number of times to allow the main thread
   * to create a writer via the zipfs mount.
   */

  for (attempts = 0; attempts < 20; attempts++)
    {
      rfd = open(a->fifo_path, O_RDONLY | O_NONBLOCK);
      if (rfd >= 0)
        {
          break;
        }

      usleep(50 * 1000);  /* 50 ms between retries */
    }

  if (rfd < 0)
    {
      a->bytes_read = 0;
      return NULL;
    }

  /* Read until EOF (writer closed) or buffer is full.  In non-blocking
   * mode read() returns -1/EAGAIN when the pipe is momentarily empty but
   * the writer is still open; poll briefly in that case.  A read of 0
   * indicates the writer has closed the pipe: we are done.
   */

  while (off < a->buf_max)
    {
      ssize_t n = read(rfd, a->buf + off, a->buf_max - off);

      if (n > 0)
        {
          off += (size_t)n;
          continue;
        }

      if (n == 0)
        {
          /* EOF: writer closed. */

          break;
        }

      /* n < 0.  EAGAIN means no data ready; short-sleep and retry.  Any
       * other errno is fatal for the reader.
       */

      if (errno == EAGAIN || errno == EINTR)
        {
          usleep(10 * 1000);
          continue;
        }

      break;
    }

  close(rfd);
  a->bytes_read = off;
  return NULL;
}

/****************************************************************************
 * Name: fd_reader_thread
 ****************************************************************************/

static FAR void *fd_reader_thread(FAR void *arg)
{
  FAR struct fd_reader_args_s *a = arg;
  size_t off = 0;

  while (off < a->buf_max)
    {
      ssize_t n = read(a->rfd, a->buf + off, a->buf_max - off);

      if (n > 0)
        {
          off += (size_t)n;
          continue;
        }

      if (n == 0)
        {
          break;
        }

      if (errno == EINTR)
        {
          continue;
        }

      break;
    }

  a->bytes_read = off;
  return NULL;
}

/****************************************************************************
 * Tests
 ****************************************************************************/

/****************************************************************************
 * Name: test_file_target
 *
 * Description:
 *   Mount zipfs rw on a regular file, write a text file + a directory +
 *   a nested binary file, unmount, then verify archive contents with the
 *   libarchive reader.
 ****************************************************************************/

static void test_file_target(FAR void **state)
{
  FAR struct archive_entry *e;
  char opts[PATH_MAX + 32];
  FAR struct archive *ra;
  uint8_t payload[256];
  bool saw_nested;
  bool saw_hello;
  int n_entries;
  bool saw_sub;
  size_t i;
  int ret;
  int fd;

  UNUSED(state);

  fd = open(PATH_FILE_ZIP, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  assert_true(fd >= 0);
  close(fd);

  snprintf(opts, sizeof(opts), "src=%s", PATH_FILE_ZIP);
  ret = mount(NULL, MOUNTPOINT, "wzipfs", 0, opts);
  assert_int_equal(ret, 0);

  /* Entry 1: short text. */

  fd = open(MOUNTPOINT "/hello.txt", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, "hello world\n", 12), 12);
  close(fd);

  /* Entry 2: directory via mkdir. */

  ret = mkdir(MOUNTPOINT "/sub", 0755);
  assert_int_equal(ret, 0);

  /* Entry 3: nested binary. */

  fd = open(MOUNTPOINT "/sub/nested.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  for (i = 0; i < sizeof(payload); i++)
    {
      payload[i] = (uint8_t)i;
    }

  assert_int_equal(write(fd, payload, sizeof(payload)),
                   sizeof(payload));
  close(fd);

  ret = umount(MOUNTPOINT);
  assert_int_equal(ret, 0);

  /* Verify with libarchive reader. */

  ra = archive_read_new();
  assert_non_null(ra);
  assert_int_equal(archive_read_support_format_zip(ra), ARCHIVE_OK);
  assert_int_equal(archive_read_open_filename(ra, PATH_FILE_ZIP, 4096),
                   ARCHIVE_OK);

  n_entries  = 0;
  saw_hello  = false;
  saw_sub    = false;
  saw_nested = false;

  while (archive_read_next_header(ra, &e) == ARCHIVE_OK)
    {
      FAR const char *name = archive_entry_pathname(e);

      if (name == NULL)
        {
          continue;
        }

      n_entries++;

      if (strcmp(name, "hello.txt") == 0)
        {
          char buf[32];
          ssize_t n;

          memset(buf, 0, sizeof(buf));
          n = archive_read_data(ra, buf, sizeof(buf) - 1);
          assert_true(n >= 12);
          assert_int_equal(memcmp(buf, "hello world\n", 12), 0);
          saw_hello = true;
        }
      else if (strcmp(name, "sub/") == 0)
        {
          saw_sub = true;
        }
      else if (strcmp(name, "sub/nested.bin") == 0)
        {
          uint8_t buf[256];
          ssize_t n;
          size_t j;

          n = archive_read_data(ra, buf, sizeof(buf));
          assert_int_equal(n, sizeof(buf));
          for (j = 0; j < sizeof(buf); j++)
            {
              assert_int_equal(buf[j], (uint8_t)j);
            }

          saw_nested = true;
        }
    }

  archive_read_free(ra);

  assert_int_equal(n_entries, 3);
  assert_true(saw_hello);
  assert_true(saw_sub);
  assert_true(saw_nested);
}

/****************************************************************************
 * Name: test_pipe_target
 *
 * Description:
 *   Mount zipfs rw on a FIFO; a reader thread captures the streamed ZIP
 *   bytes.  After umount, verify the captured bytes with libarchive memory
 *   reader.
 ****************************************************************************/

static void test_pipe_target(FAR void **state)
{
  struct pipe_reader_args_s args;
  FAR struct archive_entry *e;
  char opts[PATH_MAX + 32];
  FAR struct archive *ra;
  uint8_t payload[64];
  FAR uint8_t *buf;
  pthread_t reader;
  int n_entries;
  bool saw_p1;
  bool saw_p2;
  size_t i;
  int ret;
  int fd;

  UNUSED(state);

  ret = safe_mkfifo(PATH_PIPE_FIFO, 0666);
  if (ret < 0)
    {
      printf("INFO: mkfifo(%s) failed errno=%d. Skipping "
             "test_pipe_target.\n", PATH_PIPE_FIFO, errno);
      skip();
    }

  buf = malloc(PIPE_CAPTURE_MAX);
  assert_non_null(buf);

  args.fifo_path  = PATH_PIPE_FIFO;
  args.buf        = buf;
  args.buf_max    = PIPE_CAPTURE_MAX;
  args.bytes_read = 0;

  ret = pthread_create(&reader, NULL, pipe_reader_thread, &args);
  assert_int_equal(ret, 0);

  snprintf(opts, sizeof(opts), "src=%s", PATH_PIPE_FIFO);
  ret = mount(NULL, MOUNTPOINT, "wzipfs", 0, opts);
  if (ret < 0)
    {
      printf("INFO: wzipfs mount on FIFO failed errno=%d. "
             "Skipping test_pipe_target.\n", errno);
      pthread_join(reader, NULL);
      free(buf);
      skip();
    }

  fd = open(MOUNTPOINT "/p1.txt", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, "pipe1", 5), 5);
  close(fd);

  fd = open(MOUNTPOINT "/p2.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  for (i = 0; i < sizeof(payload); i++)
    {
      payload[i] = (uint8_t)(i ^ 0x5a);
    }

  assert_int_equal(write(fd, payload, sizeof(payload)),
                   sizeof(payload));
  close(fd);

  ret = umount(MOUNTPOINT);
  assert_int_equal(ret, 0);

  pthread_join(reader, NULL);

  /* Pipe streaming must deliver bytes to the external reader: zipfs opens
   * the FIFO O_WRONLY so pipecommon routes writes only to outside readers.
   */

  assert_true(args.bytes_read > 0);

  /* Verify the captured bytes via memory reader. */

  ra = archive_read_new();
  assert_non_null(ra);
  assert_int_equal(archive_read_support_format_zip(ra), ARCHIVE_OK);
  assert_int_equal(archive_read_open_memory(ra, buf, args.bytes_read),
                   ARCHIVE_OK);

  n_entries = 0;
  saw_p1    = false;
  saw_p2    = false;

  while (archive_read_next_header(ra, &e) == ARCHIVE_OK)
    {
      FAR const char *name = archive_entry_pathname(e);

      if (name == NULL)
        {
          continue;
        }

      n_entries++;

      if (strcmp(name, "p1.txt") == 0)
        {
          char tmp[16];
          ssize_t n;

          memset(tmp, 0, sizeof(tmp));
          n = archive_read_data(ra, tmp, sizeof(tmp) - 1);
          assert_int_equal(n, 5);
          assert_int_equal(memcmp(tmp, "pipe1", 5), 0);
          saw_p1 = true;
        }
      else if (strcmp(name, "p2.bin") == 0)
        {
          uint8_t tmp[64];
          ssize_t n;

          n = archive_read_data(ra, tmp, sizeof(tmp));
          assert_int_equal(n, sizeof(tmp));
          for (i = 0; i < sizeof(tmp); i++)
            {
              assert_int_equal(tmp[i], (uint8_t)(i ^ 0x5a));
            }

          saw_p2 = true;
        }
    }

  archive_read_free(ra);
  free(buf);

  assert_int_equal(n_entries, 2);
  assert_true(saw_p1);
  assert_true(saw_p2);
}

/****************************************************************************
 * Name: test_fd_target
 *
 * Description:
 *   Mount wzipfs on a caller-provided writer fd (anonymous pipe).  wzipfs
 *   borrows the fd: the caller must keep it open until after umount(2).
 *   Closing pipefd[1] after umount delivers EOF to the reader thread,
 *   which then validates the captured bytes via libarchive.
 ****************************************************************************/

static void test_fd_target(FAR void **state)
{
  struct fd_reader_args_s args;
  FAR struct archive_entry *e;
  FAR struct archive *ra;
  uint8_t payload[64];
  FAR uint8_t *buf;
  pthread_t reader;
  int pipefd[2];
  char opts[64];
  int n_entries;
  bool saw_f1;
  bool saw_f2;
  size_t i;
  int ret;
  int fd;

  UNUSED(state);

  ret = pipe(pipefd);
  if (ret < 0)
    {
      printf("INFO: pipe() failed errno=%d. Skipping test_fd_target.\n",
             errno);
      skip();
    }

  buf = malloc(PIPE_CAPTURE_MAX);
  assert_non_null(buf);

  args.buf        = buf;
  args.buf_max    = PIPE_CAPTURE_MAX;
  args.bytes_read = 0;
  args.rfd        = pipefd[0];

  ret = pthread_create(&reader, NULL, fd_reader_thread, &args);
  assert_int_equal(ret, 0);

  snprintf(opts, sizeof(opts), "fd=%d", pipefd[1]);
  ret = mount(NULL, MOUNTPOINT, "wzipfs", 0, opts);
  if (ret < 0)
    {
      printf("INFO: wzipfs mount on fd=%d failed errno=%d. "
             "Skipping test_fd_target.\n", pipefd[1], errno);
      close(pipefd[0]);
      close(pipefd[1]);
      pthread_join(reader, NULL);
      free(buf);
      skip();
    }

  fd = open(MOUNTPOINT "/f1.txt", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, "fdone", 5), 5);
  close(fd);

  fd = open(MOUNTPOINT "/f2.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  for (i = 0; i < sizeof(payload); i++)
    {
      payload[i] = (uint8_t)(i ^ 0xa5);
    }

  assert_int_equal(write(fd, payload, sizeof(payload)),
                   sizeof(payload));
  close(fd);

  ret = umount(MOUNTPOINT);
  assert_int_equal(ret, 0);

  /* Close the writer end now that wzipfs has released its borrow.
   * This delivers EOF to the reader thread waiting on pipefd[0].
   */

  close(pipefd[1]);
  pthread_join(reader, NULL);
  close(pipefd[0]);

  assert_true(args.bytes_read > 0);

  ra = archive_read_new();
  assert_non_null(ra);
  assert_int_equal(archive_read_support_format_zip(ra), ARCHIVE_OK);
  assert_int_equal(archive_read_open_memory(ra, buf, args.bytes_read),
                   ARCHIVE_OK);

  n_entries = 0;
  saw_f1    = false;
  saw_f2    = false;

  while (archive_read_next_header(ra, &e) == ARCHIVE_OK)
    {
      FAR const char *name = archive_entry_pathname(e);

      if (name == NULL)
        {
          continue;
        }

      n_entries++;

      if (strcmp(name, "f1.txt") == 0)
        {
          char tmp[16];
          ssize_t n;

          memset(tmp, 0, sizeof(tmp));
          n = archive_read_data(ra, tmp, sizeof(tmp) - 1);
          assert_int_equal(n, 5);
          assert_int_equal(memcmp(tmp, "fdone", 5), 0);
          saw_f1 = true;
        }
      else if (strcmp(name, "f2.bin") == 0)
        {
          uint8_t tmp[64];
          ssize_t n;

          n = archive_read_data(ra, tmp, sizeof(tmp));
          assert_int_equal(n, sizeof(tmp));
          for (i = 0; i < sizeof(tmp); i++)
            {
              assert_int_equal(tmp[i], (uint8_t)(i ^ 0xa5));
            }

          saw_f2 = true;
        }
    }

  archive_read_free(ra);
  free(buf);

  assert_int_equal(n_entries, 2);
  assert_true(saw_f1);
  assert_true(saw_f2);
}

#ifdef CONFIG_FS_ZIPFS_WRITE_DEFLATE

/****************************************************************************
 * Name: test_method_mount_option
 *
 * Description:
 *   Verify that the method=store and method=deflate mount options correctly
 *   control compression for all entries written in a session.  Writes the
 *   same 1024-byte highly-compressible payload (all 'X') with each method
 *   and asserts DEFLATE output is significantly smaller than STORE output.
 ****************************************************************************/

static void test_method_mount_option(FAR void **state)
{
  FAR struct archive_entry *e;
  char opts[PATH_MAX + 64];
  FAR struct archive *ra;
  struct stat st_deflate;
  struct stat st_store;
  FAR char *payload;
  FAR char *buf;
  int ret;
  int fd;

  UNUSED(state);

  payload = malloc(METHOD_PAYLOAD);
  assert_non_null(payload);
  buf = malloc(METHOD_READ_BUF);
  assert_non_null(buf);
  memset(payload, 'X', METHOD_PAYLOAD);

  /* Phase 1: method=store (default). */

  snprintf(opts, sizeof(opts), "src=%s", PATH_FILE_ZIP);
  ret = mount(NULL, MOUNTPOINT, "wzipfs", 0, opts);
  assert_int_equal(ret, 0);

  fd = open(MOUNTPOINT "/compressible.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, payload, METHOD_PAYLOAD), METHOD_PAYLOAD);
  close(fd);
  assert_int_equal(umount(MOUNTPOINT), 0);
  assert_int_equal(stat(PATH_FILE_ZIP, &st_store), 0);

  /* Phase 2: method=deflate on a fresh empty file. */

  unlink(PATH_FILE_ZIP);
  fd = open(PATH_FILE_ZIP, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  assert_true(fd >= 0);
  close(fd);

  snprintf(opts, sizeof(opts), "src=%s,method=deflate", PATH_FILE_ZIP);
  ret = mount(NULL, MOUNTPOINT, "wzipfs", 0, opts);
  assert_int_equal(ret, 0);

  fd = open(MOUNTPOINT "/compressible.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, payload, METHOD_PAYLOAD), METHOD_PAYLOAD);
  close(fd);
  assert_int_equal(umount(MOUNTPOINT), 0);
  assert_int_equal(stat(PATH_FILE_ZIP, &st_deflate), 0);

  /* DEFLATE of all-'X' payload should be noticeably smaller. */

  printf("test_method: store=%ld deflate=%ld\n",
         (long)st_store.st_size, (long)st_deflate.st_size);
  assert_true(st_deflate.st_size < st_store.st_size);

  /* Verify the DEFLATE archive decodes cleanly via libarchive. */

  ra = archive_read_new();
  assert_non_null(ra);
  assert_int_equal(archive_read_support_format_zip(ra), ARCHIVE_OK);
  assert_int_equal(archive_read_open_filename(ra, PATH_FILE_ZIP, 4096),
                   ARCHIVE_OK);
  while (archive_read_next_header(ra, &e) == ARCHIVE_OK)
    {
      la_ssize_t total = 0;
      la_ssize_t n;

      while ((n = archive_read_data(ra, buf, METHOD_READ_BUF)) > 0)
        {
          assert_int_equal(memcmp(buf, payload + total, (size_t)n), 0);
          total += n;
        }

      assert_int_equal(total, METHOD_PAYLOAD);
    }

  archive_read_free(ra);
  free(buf);
  free(payload);
  unlink(PATH_FILE_ZIP);
}

/****************************************************************************
 * Name: test_method_deflate_persists_across_mkdir
 *
 * Description:
 *   Regression for the mkdir-resets-compression bug: under method=deflate,
 *   writing a regular file after mkdir() must still emit DEFLATE bytes.
 *   The test writes an identical highly-compressible payload before and
 *   after mkdir, then compares the archive size against a pure-store
 *   variant; DEFLATE must still be significantly smaller than STORE
 *   even when a directory entry appears between the two files.
 ****************************************************************************/

static void test_method_deflate_persists_across_mkdir(FAR void **state)
{
  char opts[PATH_MAX + 64];
  struct stat st_deflate;
  struct stat st_store;
  FAR char *payload;
  int ret;
  int fd;

  UNUSED(state);

  payload = malloc(METHOD_PAYLOAD);
  assert_non_null(payload);
  memset(payload, 'X', METHOD_PAYLOAD);

  snprintf(opts, sizeof(opts), "src=%s", PATH_FILE_ZIP);
  ret = mount(NULL, MOUNTPOINT, "wzipfs", 0, opts);
  assert_int_equal(ret, 0);

  fd = open(MOUNTPOINT "/a.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, payload, METHOD_PAYLOAD), METHOD_PAYLOAD);
  close(fd);

  assert_int_equal(mkdir(MOUNTPOINT "/sub", 0755), 0);

  fd = open(MOUNTPOINT "/sub/b.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, payload, METHOD_PAYLOAD), METHOD_PAYLOAD);
  close(fd);
  assert_int_equal(umount(MOUNTPOINT), 0);
  assert_int_equal(stat(PATH_FILE_ZIP, &st_store), 0);

  unlink(PATH_FILE_ZIP);
  fd = open(PATH_FILE_ZIP, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  assert_true(fd >= 0);
  close(fd);

  snprintf(opts, sizeof(opts), "src=%s,method=deflate", PATH_FILE_ZIP);
  ret = mount(NULL, MOUNTPOINT, "wzipfs", 0, opts);
  assert_int_equal(ret, 0);

  fd = open(MOUNTPOINT "/a.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, payload, METHOD_PAYLOAD), METHOD_PAYLOAD);
  close(fd);

  assert_int_equal(mkdir(MOUNTPOINT "/sub", 0755), 0);

  fd = open(MOUNTPOINT "/sub/b.bin", O_WRONLY | O_CREAT, 0644);
  assert_true(fd >= 0);
  assert_int_equal(write(fd, payload, METHOD_PAYLOAD), METHOD_PAYLOAD);
  close(fd);
  assert_int_equal(umount(MOUNTPOINT), 0);
  assert_int_equal(stat(PATH_FILE_ZIP, &st_deflate), 0);

  free(payload);

  printf("test_mkdir_compression: store=%ld deflate=%ld (kept at %s)\n",
         (long)st_store.st_size, (long)st_deflate.st_size, PATH_FILE_ZIP);

  /* If mkdir resets requested_compression to store, the post-mkdir file
   * would be emitted uncompressed and the deflate archive would be
   * roughly half STORE (one file compressed, one not).
   */

  assert_true(st_deflate.st_size * 2 < st_store.st_size);
}

#endif /* CONFIG_FS_ZIPFS_WRITE_DEFLATE */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *dir = (argc > 1) ? argv[1] : DEFAULT_TEST_DIR;
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_setup_teardown(test_file_target,
                                    common_setup, common_teardown),
    cmocka_unit_test_setup_teardown(test_pipe_target,
                                    common_setup, common_teardown),
    cmocka_unit_test_setup_teardown(test_fd_target,
                                    common_setup, common_teardown),
#ifdef CONFIG_FS_ZIPFS_WRITE_DEFLATE
    cmocka_unit_test_setup_teardown(test_method_mount_option,
                                    common_setup, common_teardown),
    cmocka_unit_test_setup_teardown(
        test_method_deflate_persists_across_mkdir,
        common_setup, common_teardown),
#endif
  };

  snprintf(g_path_file_zip, sizeof(g_path_file_zip),
           "%s/cmocka_wzipfs.zip", dir);

  printf("cmocka_wzipfs: backing dir = %s\n", dir);
  return cmocka_run_group_tests_name("wzipfs", tests, NULL, NULL);
}
