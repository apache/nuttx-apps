/****************************************************************************
 * apps/testing/fs/refstest/fs_refs_test.c
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

#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <setjmp.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/atomic.h>
#include <nuttx/fdcheck.h>
#include <nuttx/fs/fs.h>

#include <cmocka.h>

#ifdef CONFIG_FS_RAMMAP
#  include <sys/mman.h>
#endif

#include "refstest_common.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TESTDIR CONFIG_TESTING_FS_REFSTEST_MOUNTDIR "/refstest-XXXXXX"
#define NTHREADS 4
#define NITERATIONS 100
#define MAPSIZE 64

#define ASSERT_REFS(fd, expected_f, expected_i) \
  do \
    { \
      int fr; \
      int ir; \
      assert_int_equal(get_refs(fd, &fr, &ir), 0); \
      assert_int_equal(fr, expected_f); \
      assert_int_equal(ir, expected_i); \
    } \
  while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct worker_s
{
  pthread_t thread;
  int fd;
  int completed;
  bool started;
};

struct refs_state_s
{
  char dir[sizeof(TESTDIR)];
  char path[sizeof(TESTDIR) + 6];
  int fd[3];
  int inode_refs;
  pid_t child;
  sem_t ready;
  sem_t release;
  struct worker_s workers[NTHREADS];
#ifdef CONFIG_FS_RAMMAP
  char device[64];
  FAR void *mapped;
  bool registered;
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_refs
 ****************************************************************************/

static int get_refs(int fd, FAR int *f_refs, FAR int *i_crefs)
{
  FAR struct file *filep;
  int ret;

  ret = file_get(fd, &filep);
  if (ret < 0)
    {
      return ret;
    }

  /* Do not count the temporary reference acquired by file_get(). */

  *f_refs = atomic_read(&filep->f_refs) - 1;
  *i_crefs = atomic_read(&filep->f_inode->i_crefs);
  file_put(filep);
  return 0;
}

/****************************************************************************
 * Name: raw_fd
 ****************************************************************************/

static int raw_fd(int fd)
{
#ifdef CONFIG_FDCHECK
  return fdcheck_restore(fd);
#else
  return fd;
#endif
}

/****************************************************************************
 * Name: close_slot
 ****************************************************************************/

static int close_slot(FAR struct refs_state_s *s, int slot)
{
  int ret;

  ret = close(s->fd[slot]);
  s->fd[slot] = -1;
  return ret;
}

/****************************************************************************
 * Name: wait_sem
 ****************************************************************************/

static int wait_sem(FAR sem_t *sem)
{
  struct timespec deadline;
  int ret;

  if (clock_gettime(CLOCK_REALTIME, &deadline) < 0)
    {
      return -1;
    }

  deadline.tv_sec += 10;
  do
    {
      ret = sem_timedwait(sem, &deadline);
    }
  while (ret < 0 && errno == EINTR);

  return ret;
}

/****************************************************************************
 * Name: reap_child
 ****************************************************************************/

static int reap_child(FAR struct refs_state_s *s, FAR int *status)
{
  pid_t pid;

  do
    {
      pid = waitpid(s->child, status, 0);
    }
  while (pid < 0 && errno == EINTR);

  if (pid != s->child)
    {
      return -1;
    }

  s->child = -1;
  return 0;
}

/****************************************************************************
 * Name: spawn_child_entry
 ****************************************************************************/

static int spawn_child_entry(int argc, FAR char *argv[])
{
  FAR struct refs_state_s *s;

  if (argc != 2)
    {
      return EXIT_FAILURE;
    }

  /* BUILD_FLAT permits the parent and child to share this fixture. */

  s = (FAR struct refs_state_s *)(uintptr_t)strtoumax(argv[1], NULL, 16);
  if (sem_post(&s->ready) < 0 || wait_sem(&s->release) < 0)
    {
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: thread_dup_close
 ****************************************************************************/

static FAR void *thread_dup_close(FAR void *arg)
{
  FAR struct worker_s *worker = arg;
  int fd;
  int i;

  for (i = 0; i < NITERATIONS; i++)
    {
      fd = dup(worker->fd);
      if (fd < 0)
        {
          break;
        }

      sched_yield();
      if (close(fd) < 0)
        {
          break;
        }

      worker->completed++;
    }

  return NULL;
}

#ifdef CONFIG_FS_RAMMAP
/****************************************************************************
 * Name: refs_read
 ****************************************************************************/

static ssize_t refs_read(FAR struct file *filep, FAR char *buffer,
                         size_t buflen)
{
  size_t count;

  if (filep->f_pos < 0)
    {
      return -EINVAL;
    }

  if (filep->f_pos >= MAPSIZE)
    {
      return 0;
    }

  count = MAPSIZE - filep->f_pos;
  if (count > buflen)
    {
      count = buflen;
    }

  memset(buffer, 'A', count);
  filep->f_pos += count;
  return count;
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* No mmap or XIP ioctl: the test must exercise the RAMMAP fallback. */

static const struct file_operations g_refs_operations =
{
  .read = refs_read
};
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: refstest_teardown
 ****************************************************************************/

int refstest_teardown(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  int status;
  int result = 0;
  int i;

  if (s == NULL)
    {
      return 0;
    }

  /* cmocka assertions bypass the rest of a test, so all resources belong
   * to the fixture, including workers and a child waiting for its parent.
   */

  if (s->child > 0)
    {
      sem_post(&s->release);
      if (reap_child(s, &status) < 0)
        {
          return -1;
        }
    }

  for (i = 0; i < NTHREADS; i++)
    {
      if (s->workers[i].started &&
          pthread_join(s->workers[i].thread, NULL) != 0)
        {
          return -1;
        }
    }

#ifdef CONFIG_FS_RAMMAP
  if (s->mapped != MAP_FAILED &&
      munmap(s->mapped, MAPSIZE) < 0)
    {
      result = -1;
    }
#endif

  for (i = 0; i < 3; i++)
    {
      if (s->fd[i] >= 0 && close_slot(s, i) < 0)
        {
          result = -1;
        }
    }

#ifdef CONFIG_FS_RAMMAP
  if (s->registered && unregister_driver(s->device) < 0)
    {
      result = -1;
    }
#endif

  if (unlink(s->path) < 0 && errno != ENOENT)
    {
      result = -1;
    }

  if (rmdir(s->dir) < 0)
    {
      result = -1;
    }

  if (sem_destroy(&s->release) < 0)
    {
      result = -1;
    }

  if (sem_destroy(&s->ready) < 0)
    {
      result = -1;
    }

  free(s);
  *state = NULL;
  return result;
}

/****************************************************************************
 * Name: refstest_setup
 ****************************************************************************/

int refstest_setup(FAR void **state)
{
  FAR struct refs_state_s *s;
  int f_refs;
  int i;

  *state = NULL;
  s = calloc(1, sizeof(*s));
  if (s == NULL)
    {
      return -1;
    }

  for (i = 0; i < 3; i++)
    {
      s->fd[i] = -1;
    }

  s->child = -1;
#ifdef CONFIG_FS_RAMMAP
  s->mapped = MAP_FAILED;
#endif
  if (sem_init(&s->ready, 0, 0) < 0)
    {
      free(s);
      return -1;
    }

  if (sem_init(&s->release, 0, 0) < 0)
    {
      sem_destroy(&s->ready);
      free(s);
      return -1;
    }

  strlcpy(s->dir, TESTDIR, sizeof(s->dir));
  if (mkdtemp(s->dir) == NULL)
    {
      sem_destroy(&s->release);
      sem_destroy(&s->ready);
      free(s);
      return -1;
    }

  snprintf(s->path, sizeof(s->path), "%s/file", s->dir);
  *state = s;
  s->fd[0] = open(s->path, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (s->fd[0] < 0 ||
      get_refs(s->fd[0], &f_refs, &s->inode_refs) < 0 ||
      f_refs != 1)
    {
      refstest_teardown(state);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: test_nuttx_fs_refs01
 * Description: Separate opens own file objects; dup shares a file object.
 ****************************************************************************/

void test_nuttx_fs_refs01(FAR void **state)
{
  FAR struct refs_state_s *s = *state;

  s->fd[1] = dup(s->fd[0]);
  assert_true(s->fd[1] >= 0);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs);

  s->fd[2] = open(s->path, O_RDWR);
  assert_true(s->fd[2] >= 0);
  ASSERT_REFS(s->fd[2], 1, s->inode_refs + 1);

  assert_int_equal(close_slot(s, 1), 0);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs + 1);
  assert_int_equal(close_slot(s, 2), 0);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
}

/****************************************************************************
 * Name: test_nuttx_fs_refs02
 * Description: dup2 onto itself, an open descriptor and an empty slot.
 ****************************************************************************/

void test_nuttx_fs_refs02(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  int target;
  int ret;

  assert_int_equal(dup2(s->fd[0], s->fd[0]), s->fd[0]);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);

  s->fd[1] = open(s->path, O_RDONLY);
  assert_true(s->fd[1] >= 0);
  ASSERT_REFS(s->fd[1], 1, s->inode_refs + 1);

  target = raw_fd(s->fd[1]);
  ret = dup2(s->fd[0], s->fd[1]);
  assert_true(ret >= 0);
  s->fd[1] = ret;
  assert_int_equal(raw_fd(ret), target);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs);

  /* Save the raw slot before close: the old FDCHECK tag becomes stale. */

  assert_int_equal(close_slot(s, 1), 0);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
  s->fd[1] = dup2(s->fd[0], target);
  assert_true(s->fd[1] >= 0);
  assert_int_equal(raw_fd(s->fd[1]), target);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs);
  assert_int_equal(close_slot(s, 1), 0);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
}

/****************************************************************************
 * Name: test_nuttx_fs_refs03
 * Description: Open files remain readable after unlink.
 ****************************************************************************/

void test_nuttx_fs_refs03(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  const char data[] = "data before unlink";
  char buffer[sizeof(data)];
  struct stat st;
  int i;

  assert_int_equal(write(s->fd[0], data, sizeof(data)), sizeof(data));
  s->fd[1] = open(s->path, O_RDONLY);
  assert_true(s->fd[1] >= 0);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs + 1);
  assert_int_equal(unlink(s->path), 0);
  assert_int_equal(stat(s->path, &st), -1);
  assert_int_equal(errno, ENOENT);

  for (i = 0; i < 2; i++)
    {
      assert_int_equal(lseek(s->fd[i], 0, SEEK_SET), 0);
      assert_int_equal(read(s->fd[i], buffer, sizeof(buffer)), sizeof(data));
      assert_memory_equal(buffer, data, sizeof(data));
      assert_int_equal(close_slot(s, i), 0);
    }

  s->fd[0] = open(s->path, O_RDONLY);
  assert_int_equal(s->fd[0], -1);
  assert_int_equal(errno, ENOENT);
}

/****************************************************************************
 * Name: test_nuttx_fs_refs04
 * Description: epoll ADD holds a file reference; DEL and close release it.
 ****************************************************************************/

void test_nuttx_fs_refs04(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  struct epoll_event ev;

  s->fd[1] = epoll_create1(0);
  assert_true(s->fd[1] >= 0);
  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;
  ev.data.fd = s->fd[0];

  assert_int_equal(epoll_ctl(s->fd[1], EPOLL_CTL_ADD, s->fd[0], &ev), 0);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs);
  assert_int_equal(epoll_ctl(s->fd[1], EPOLL_CTL_DEL, s->fd[0], NULL), 0);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
  assert_int_equal(epoll_ctl(s->fd[1], EPOLL_CTL_ADD, s->fd[0], &ev), 0);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs);
  assert_int_equal(close_slot(s, 1), 0);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
}

/****************************************************************************
 * Name: test_nuttx_fs_refs05
 * Description: dup3 replaces a descriptor and sets only its CLOEXEC flag.
 ****************************************************************************/

void test_nuttx_fs_refs05(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  int target;
  int ret;

  s->fd[1] = open(s->path, O_RDONLY);
  assert_true(s->fd[1] >= 0);
  target = raw_fd(s->fd[1]);
  ret = dup3(s->fd[0], s->fd[1], O_CLOEXEC);
  assert_true(ret >= 0);
  s->fd[1] = ret;
  assert_int_equal(raw_fd(ret), target);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs);
  assert_int_equal(fcntl(s->fd[1], F_GETFD), FD_CLOEXEC);
  assert_int_equal(fcntl(s->fd[0], F_GETFD), 0);
  assert_int_equal(dup3(s->fd[0], s->fd[0], O_CLOEXEC), -1);
  assert_int_equal(errno, EINVAL);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs);
}

/****************************************************************************
 * Name: test_nuttx_fs_refs06
 * Description: Spawn inherits normal descriptors but skips CLOEXEC ones.
 ****************************************************************************/

void test_nuttx_fs_refs06(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  char address[2 * sizeof(uintptr_t) + 1];
  FAR char *argv[2];
  int status;

  s->fd[1] = open(s->path, O_RDWR | O_CLOEXEC);
  assert_true(s->fd[1] >= 0);
  snprintf(address, sizeof(address), "%" PRIxPTR, (uintptr_t)s);
  argv[0] = address;
  argv[1] = NULL;
  s->child = task_spawn("refs_child", spawn_child_entry,
                        NULL, NULL, argv, NULL);
  assert_true(s->child > 0);
  assert_int_equal(wait_sem(&s->ready), 0);
  ASSERT_REFS(s->fd[0], 2, s->inode_refs + 1);
  ASSERT_REFS(s->fd[1], 1, s->inode_refs + 1);
  assert_int_equal(sem_post(&s->release), 0);
  assert_int_equal(reap_child(s, &status), 0);
  assert_true(WIFEXITED(status));
  assert_int_equal(WEXITSTATUS(status), EXIT_SUCCESS);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs + 1);
  ASSERT_REFS(s->fd[1], 1, s->inode_refs + 1);
}

#ifdef CONFIG_FS_RAMMAP
/****************************************************************************
 * Name: test_nuttx_fs_refs07
 * Description: RAMMAP owns a file reference until munmap.
 ****************************************************************************/

void test_nuttx_fs_refs07(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  int inode_refs;
  int file_refs;
  int i;

  snprintf(s->device, sizeof(s->device), "/dev/refs-%d-%s",
           (int)getpid(), strrchr(s->dir, '/') + 1);
  assert_int_equal(register_driver(s->device, &g_refs_operations,
                                  0600, NULL), 0);
  s->registered = true;
  s->fd[1] = open(s->device, O_RDONLY);
  assert_true(s->fd[1] >= 0);
  assert_int_equal(get_refs(s->fd[1], &file_refs, &inode_refs), 0);
  assert_int_equal(file_refs, 1);

  s->mapped = mmap(NULL, MAPSIZE, PROT_READ, MAP_PRIVATE, s->fd[1], 0);
  assert_ptr_not_equal(s->mapped, MAP_FAILED);
  for (i = 0; i < MAPSIZE; i++)
    {
      assert_int_equal(((FAR char *)s->mapped)[i], 'A');
    }

  ASSERT_REFS(s->fd[1], 2, inode_refs);
  assert_int_equal(munmap(s->mapped, MAPSIZE), 0);
  s->mapped = MAP_FAILED;
  ASSERT_REFS(s->fd[1], 1, inode_refs);
}
#endif

/****************************************************************************
 * Name: test_nuttx_fs_refs08
 * Description: Invalid descriptors must not disturb live references.
 ****************************************************************************/

void test_nuttx_fs_refs08(FAR void **state)
{
  FAR struct refs_state_s *s = *state;

  assert_int_equal(dup(-1), -1);
  assert_int_equal(errno, EBADF);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
  assert_int_equal(dup2(-1, s->fd[0]), -1);
  assert_int_equal(errno, EBADF);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
  assert_int_equal(close(-1), -1);
  assert_int_equal(errno, EBADF);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
  assert_int_equal(dup2(s->fd[0], -1), -1);
  assert_int_equal(errno, EBADF);
  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
}

/****************************************************************************
 * Name: test_nuttx_fs_refs09
 * Description: All concurrent dup/close operations must complete.
 ****************************************************************************/

void test_nuttx_fs_refs09(FAR void **state)
{
  FAR struct refs_state_s *s = *state;
  int i;

  for (i = 0; i < NTHREADS; i++)
    {
      s->workers[i].fd = s->fd[0];
      assert_int_equal(pthread_create(&s->workers[i].thread, NULL,
                                     thread_dup_close, &s->workers[i]), 0);
      s->workers[i].started = true;
    }

  for (i = 0; i < NTHREADS; i++)
    {
      assert_int_equal(pthread_join(s->workers[i].thread, NULL), 0);
      s->workers[i].started = false;
      assert_int_equal(s->workers[i].completed, NITERATIONS);
    }

  ASSERT_REFS(s->fd[0], 1, s->inode_refs);
}
