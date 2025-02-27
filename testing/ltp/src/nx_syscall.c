/****************************************************************************
 * apps/testing/ltp/src/nx_syscall.c
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

#include <aio.h>
#include <assert.h>
#include <mqueue.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include <sys/boardctl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/shm.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#include <nuttx/arch.h>
#include <nuttx/binfmt/binfmt.h>
#include <nuttx/config.h>
#include <nuttx/module.h>

#include "lapi/syscalls.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int syscall(int syscall_nr, ...)
{
  int ret = 0;

  /* extract out the detailed params list from __va_args list,
   * and then convert to the detailed function calling
   */

  va_list ap;
  va_start(ap, syscall_nr);
  switch (syscall_nr)
  {
    case __NR__assert:
    {
      const char *filename = va_arg(ap, const char *);
      int linenum = va_arg(ap, int);
      const char *msg = va_arg(ap, const char *);
      void *reg = va_arg(ap, void *);
      int irq = va_arg(ap, int);
      _assert(filename, linenum, msg, reg, irq);
      break;
    }

    case __NR_exit:
    {
      int status = va_arg(ap, int);
      _exit(status);
      break;
    }

    case __NR_accept4:
    {
      int sockfd = va_arg(ap, int);
      struct sockaddr *addr = va_arg(ap, struct sockaddr *);
      socklen_t *addrlen = va_arg(ap, socklen_t *);
      int flags = va_arg(ap, int);
      ret = accept4(sockfd, addr, addrlen, flags);
      break;
    }

#if defined(CONFIG_CLOCK_TIMEKEEPING) || defined(CONFIG_CLOCK_ADJTIME)
    case __NR_adjtimex:
    {
      const struct timeval *delta = va_arg(ap, const struct timeval *);
      const struct timeval *olddelta = va_arg(ap, const struct timeval *);
      ret = adjtime(delta, olddelta);
      break;
    }
#endif

#ifdef CONFIG_FS_AIO
    case __NR_aio_cancel:
    {
      int fildes = va_arg(ap, int);
      struct aiocb *aiocbp = va_arg(ap, struct aiocb *);
      ret = aio_cancel(fildes, aiocbp);
      break;
    }
#endif

#ifdef CONFIG_FS_AIO
    case __NR_aio_fsync:
    {
      int op = va_arg(ap, int);
      struct aiocb *aiocbp = va_arg(ap, struct aiocb *);
      ret = aio_fsync(op, aiocbp);
      break;
    }
#endif

#ifdef CONFIG_FS_AIO
    case __NR_aio_read:
    {
      struct aiocb *aiocbp = va_arg(ap, struct aiocb *);
      ret = aio_read(aiocbp);
      break;
    }
#endif

#ifdef CONFIG_FS_AIO
    case __NR_aio_write:
    {
      struct aiocb *aiocbp = va_arg(ap, struct aiocb *);
      ret = aio_write(aiocbp);
      break;
    }
#endif

#ifdef CONFIG_CRYPTO_RANDOM_POOL
    case __NR_arc4random_buf:
    {
      void *buf = va_arg(ap, void *);
      size_t nbytes = va_arg(ap, size_t);
      arc4random_buf(buf, nbytes);
      break;
    }
#endif

#ifdef CONFIG_NET
    case __NR_bind:
    {
      int sockfd = va_arg(ap, int);
      const struct sockaddr *addr = va_arg(ap, const struct sockaddr *);
      socklen_t addrlen = va_arg(ap, socklen_t);
      ret = bind(sockfd, addr, addrlen);
      break;
    }
#endif

#ifdef CONFIG_BOARDCTL
    case __NR_boardctl:
    {
      unsigned int cmd = va_arg(ap, unsigned int);
      uintptr_t arg = va_arg(ap, uintptr_t);
      ret = boardctl(cmd, arg);
      break;
    }
#endif

    case __NR_chmod:
    {
      const char *path = va_arg(ap, const char *);
      mode_t mode = va_arg(ap, mode_t);
      ret = chmod(path, mode);
      break;
    }

    case __NR_chown:
    {
      const char *path = va_arg(ap, const char *);
      uid_t owner = va_arg(ap, uid_t);
      gid_t group = va_arg(ap, gid_t);
      ret = chown(path, owner, group);
      break;
    }

#ifndef CONFIG_DISABLE_ENVIRON
    case __NR_clearenv:
    {
      ret = clearenv();
      break;
    }
#endif

    case __NR_clock:
    {
      ret = clock();
      break;
    }

    case __NR_clock_gettime:
    {
      int clockid = va_arg(ap, int);
      struct timespec *ts = va_arg(ap, struct timespec *);
      ret = clock_gettime(clockid, ts);
      break;
    }

    case __NR_clock_nanosleep:
    {
      clockid_t clockid = va_arg(ap, clockid_t);
      int flags = va_arg(ap, int);
      const struct timespec *req = va_arg(ap, const struct timespec *);
      struct timespec *rem = va_arg(ap, struct timespec *);
      ret = clock_nanosleep(clockid, flags, req, rem);
      break;
    }

    case __NR_clock_settime:
    {
      clockid_t clockid = va_arg(ap, clockid_t);
      struct timespec *ts = va_arg(ap, struct timespec *);
      ret = clock_settime(clockid, ts);
      break;
    }

    case __NR_close:
    {
      int fd = va_arg(ap, int);
      ret = close(fd);
      break;
    }

#ifdef CONFIG_NET
    case __NR_connect:
    {
      int sockfd = va_arg(ap, int);
      const struct sockaddr *addr = va_arg(ap, const struct sockaddr *);
      socklen_t addrlen = va_arg(ap, socklen_t);
      ret = connect(sockfd, addr, addrlen);
      break;
    }
#endif

    case __NR_dup:
    {
      int fd = va_arg(ap, int);
      ret = dup(fd);
      break;
    }

    case __NR_dup2:
    {
      int fd1 = va_arg(ap, int);
      int fd2 = va_arg(ap, int);
      ret = dup2(fd1, fd2);
      break;
    }

    case __NR_epoll_create1:
    {
      int flags = va_arg(ap, int);
      ret = epoll_create1(flags);
      break;
    }

    case __NR_epoll_ctl:
    {
      int epfd = va_arg(ap, int);
      int op = va_arg(ap, int);
      int fd = va_arg(ap, int);
      struct epoll_event *event = va_arg(ap, struct epoll_event *);
      ret = epoll_ctl(epfd, op, fd, event);
      break;
    }

    case __NR_epoll_wait:
    {
      int epfd = va_arg(ap, int);
      struct epoll_event *events = va_arg(ap, struct epoll_event *);
      int maxevents = va_arg(ap, int);
      int timeout = va_arg(ap, int);
      ret = epoll_wait(epfd, events, maxevents, timeout);
      break;
    }

#if defined(CONFIG_EVENT_FD)
    case __NR_eventfd:
    {
      unsigned int count = va_arg(ap, unsigned int);
      int flags = va_arg(ap, int);
      ret = eventfd(count, flags);
      break;
    }
#endif

#if !defined(CONFIG_BINFMT_DISABLE) && !defined(CONFIG_BUILD_KERNEL)
    case __NR_exec:
    {
      const char *filename = va_arg(ap, const char *);
      char *const *argv = va_arg(ap, char *const *);
      char *const *envp = va_arg(ap, char *const *);
      const struct symtab_s *exports = va_arg(ap, const struct symtab_s *);
      int nexports = va_arg(ap, int);
      ret = exec(filename, argv, envp, exports, nexports);
      break;
    }
#endif

#if !defined(CONFIG_BINFMT_DISABLE) && defined(CONFIG_LIBC_EXECFUNCS)
    case __NR_execve:
    {
      const char *path = va_arg(ap, const char *);
      char *const *argv = va_arg(ap, char *const *);
      char *const *envp = va_arg(ap, char *const *);
      ret = execve(path, argv, envp);
      break;
    }
#endif

    case __NR_fchmod:
    {
      int fd = va_arg(ap, int);
      mode_t mode = va_arg(ap, mode_t);
      ret = fchmod(fd, mode);
      break;
    }

    case __NR_fchown:
    {
      int fd = va_arg(ap, int);
      uid_t owner = va_arg(ap, uid_t);
      gid_t group = va_arg(ap, gid_t);
      ret = fchown(fd, owner, group);
      break;
    }

    case __NR_fdatasync:
    {
      int fd = va_arg(ap, int);
      ret = fdatasync(fd);
      break;
    }

    case __NR_fstatat:
    case __NR_fstatat64:
    {
      int dirfd = va_arg(ap, int);
      const char *pathname = va_arg(ap, const char *);
      struct stat *buf = va_arg(ap, struct stat *);
      int flags = va_arg(ap, int);
      ret = fstatat(dirfd, pathname, buf, flags);
      break;
    }

    case __NR_fcntl:
    {
      int fd = va_arg(ap, int);
      int cmd = va_arg(ap, int);
      long arg = va_arg(ap, long);
      ret = fcntl(fd, cmd, arg);
      break;
    }

    case __NR_fstat:
    {
      int fd = va_arg(ap, int);
      struct stat *buf = va_arg(ap, struct stat *);
      ret = fstat(fd, buf);
      break;
    }

    case __NR_fstatfs:
    {
      int fd = va_arg(ap, int);
      struct statfs *buf = va_arg(ap, struct statfs *);
      ret = fstatfs(fd, buf);
      break;
    }

    case __NR_fsync:
    {
      int fd = va_arg(ap, int);
      ret = fsync(fd);
      break;
    }

    case __NR_ftruncate:
    {
      int fd = va_arg(ap, int);
      off_t length = va_arg(ap, off_t);
      ret = ftruncate(fd, length);
      break;
    }

    case __NR_futimens:
    {
      int fd = va_arg(ap, int);
      const struct timespec *times = va_arg(ap, const struct timespec *);
      ret = futimens(fd, times);
      break;
    }

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_getegid:
    {
      ret = getegid();
      break;
    }
#endif

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_geteuid:
    {
      ret = geteuid();
      break;
    }
#endif

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_getgid:
    {
      ret = getgid();
      break;
    }
#endif

    case __NR_gethostname:
    {
      char *name = va_arg(ap, char *);
      size_t name_len = va_arg(ap, size_t);
      ret = gethostname(name, name_len);
      break;
    }

#if !defined(CONFIG_DISABLE_POSIX_TIMERS)
    case __NR_getitimer:
    {
      int which = va_arg(ap, int);
      struct itimerval *value = va_arg(ap, struct itimerval *);
      ret = getitimer(which, value);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_getpeername:
    {
      int sockfd = va_arg(ap, int);
      struct sockaddr *addr = va_arg(ap, struct sockaddr *);
      socklen_t *addrlen = va_arg(ap, socklen_t *);
      ret = getpeername(sockfd, addr, addrlen);
      break;
    }
#endif

    case __NR_getpid:
    {
      ret = getpid();
      break;
    }

#if defined(CONFIG_SCHED_HAVE_PARENT)
    case __NR_getppid:
    {
      ret = getppid();
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_getsockname:
    {
      int sockfd = va_arg(ap, int);
      struct sockaddr *addr = va_arg(ap, struct sockaddr *);
      socklen_t *addrlen = va_arg(ap, socklen_t *);
      ret = getsockname(sockfd, addr, addrlen);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_getsockopt:
    {
      int sockfd = va_arg(ap, int);
      int level = va_arg(ap, int);
      int option = va_arg(ap, int);
      void *value = va_arg(ap, void *);
      socklen_t *value_len = va_arg(ap, socklen_t *);
      ret = getsockopt(sockfd, level, option, value, value_len);
      break;
    }
#endif

    case __NR_gettid:
    {
      ret = gettid();
      break;
    }

    case __NR_gettimeofday:
    {
      struct timeval *tv = va_arg(ap, struct timeval *);
      struct timezone *tz = va_arg(ap, struct timezone *);
      ret = gettimeofday(tv, tz);
      break;
    }

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_getuid:
    {
      ret = getuid();
      break;
    }
#endif

#if defined(CONFIG_FS_NOTIFY)
    case __NR_inotify_add_watch:
    {
      int fd = va_arg(ap, int);
      const char *pathname = va_arg(ap, const char *);
      uint32_t mask = va_arg(ap, uint32_t);
      ret = inotify_add_watch(fd, pathname, mask);
      break;
    }
#endif

#if defined(CONFIG_FS_NOTIFY)
    case __NR_inotify_init:
    {
      ret = inotify_init();
      break;
    }
#endif

#if defined(CONFIG_FS_NOTIFY)
    case __NR_inotify_init1:
    {
      int flags = va_arg(ap, int);
      ret = inotify_init1(flags);
      break;
    }
#endif

#if defined(CONFIG_FS_NOTIFY)
    case __NR_inotify_rm_watch:
    {
      int fd = va_arg(ap, int);
      int wd = va_arg(ap, int);
      ret = inotify_rm_watch(fd, wd);
      break;
    }
#endif

#if defined(CONFIG_MODULE)
    case __NR_insmod:
    {
      const char *file_name = va_arg(ap, const char *);
      const char *mod_name = va_arg(ap, const char *);
      insmod(file_name, mod_name);
      break;
    }
#endif

    case __NR_ioctl:
    {
      int fd = va_arg(ap, int);
      int req = va_arg(ap, int);
      long arg = va_arg(ap, long);
      ret = ioctl(fd, req, arg);
      break;
    }

    case __NR_kill:
    {
      pid_t pid = va_arg(ap, pid_t);
      int sig = va_arg(ap, int);
      ret = kill(pid, sig);
      break;
    }

    case __NR_lchmod:
    {
      const char *pathname = va_arg(ap, const char *);
      mode_t mode = va_arg(ap, mode_t);
      ret = lchmod(pathname, mode);
      break;
    }

    case __NR_lchown:
    {
      const char *path = va_arg(ap, const char *);
      uid_t owner = va_arg(ap, uid_t);
      gid_t group = va_arg(ap, gid_t);
      ret = lchown(path, owner, group);
      break;
    }

#if defined(CONFIG_PSEUDOFS_SOFTLINKS)
    case __NR_link:
    {
      const char *old_path = va_arg(ap, const char *);
      const char *new_path = va_arg(ap, const char *);
      ret = link(old_path, new_path);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_listen:
    {
      int sockfd = va_arg(ap, int);
      int backlog = va_arg(ap, int);
      ret = listen(sockfd, backlog);
      break;
    }
#endif

    case __NR_lseek:
    {
      int fd = va_arg(ap, int);
      off_t offset = va_arg(ap, off_t);
      int whence = va_arg(ap, int);
      ret = lseek(fd, offset, whence);
      break;
    }

    case __NR_lstat:
    {
      const char *path = va_arg(ap, const char *);
      struct stat *buf = va_arg(ap, struct stat *);
      ret = lstat(path, buf);
      break;
    }

    case __NR_lutimens:
    {
      const char *path = va_arg(ap, const char *);
      const struct timespec *times = va_arg(ap, const struct timespec *);
      ret = lutimens(path, times);
      break;
    }

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
    case __NR_mkdir:
    {
      const char *pathname = va_arg(ap, const char *);
      mode_t mode = va_arg(ap, mode_t);
      ret = mkdir(pathname, mode);
      break;
    }
#endif

    case __NR_mmap:
    {
      void *addr = va_arg(ap, void *);
      size_t length = va_arg(ap, size_t);
      int prot = va_arg(ap, int);
      int flags = va_arg(ap, int);
      int fd = va_arg(ap, int);
      off_t offset = va_arg(ap, off_t);
      mmap(addr, length, prot, flags, fd, offset);
      break;
    }

#if defined(CONFIG_MODULE)
    case __NR_modhandle:
    {
      const char *name = va_arg(ap, const char *);
      modhandle(name);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
    case __NR_mount:
    {
      const char *source = va_arg(ap, const char *);
      const char *target = va_arg(ap, const char *);
      const char *filesystemtype = va_arg(ap, const char *);
      unsigned long mountflags = va_arg(ap, unsigned long);
      const void *data = va_arg(ap, const void *);
      ret = mount(source, target, filesystemtype, mountflags, data);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_close:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      ret = mq_close(mqdes);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_getattr:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      struct mq_attr *attr = va_arg(ap, struct mq_attr *);
      ret = mq_getattr(mqdes, attr);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_notify:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      const struct sigevent *notification =
                va_arg(ap, const struct sigevent *);
      ret = mq_notify(mqdes, notification);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_open:
    {
      const char *name = va_arg(ap, const char *);
      int oflag = va_arg(ap, int);
      mode_t mode = va_arg(ap, mode_t);
      struct mq_attr *attr = va_arg(ap, struct mq_attr *);
      ret = mq_open(name, oflag, mode, attr);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_receive:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      char *msg = va_arg(ap, char *);
      size_t msg_len = va_arg(ap, size_t);
      unsigned int *prio = va_arg(ap, unsigned int *);
      ret = mq_receive(mqdes, msg, msg_len, prio);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_send:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      const char *msg = va_arg(ap, const char *);
      size_t msg_len = va_arg(ap, size_t);
      unsigned int prio = va_arg(ap, unsigned int);
      ret = mq_send(mqdes, msg, msg_len, prio);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_setattr:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      const struct mq_attr *newattr = va_arg(ap, const struct mq_attr *);
      struct mq_attr *oldattr = va_arg(ap, struct mq_attr *);
      ret = mq_setattr(mqdes, newattr, oldattr);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_timedreceive:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      char *msg = va_arg(ap, char *);
      size_t msg_len = va_arg(ap, size_t);
      unsigned int *prio = va_arg(ap, unsigned int *);
      const struct timespec *abstime = va_arg(ap, const struct timespec *);
      ret = mq_timedreceive(mqdes, msg, msg_len, prio, abstime);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_timedsend:
    {
      mqd_t mqdes = va_arg(ap, mqd_t);
      const char *msg = va_arg(ap, const char *);
      size_t msg_len = va_arg(ap, size_t);
      unsigned int prio = va_arg(ap, unsigned int);
      struct timespec *abstime = va_arg(ap, struct timespec *);
      ret = mq_timedsend(mqdes, msg, msg_len, prio, abstime);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MQUEUE)
    case __NR_mq_unlink:
    {
      const char *name = va_arg(ap, const char *);
      ret = mq_unlink(name);
      break;
    }
#endif

    case __NR_msync:
    {
      void *addr = va_arg(ap, void *);
      size_t length = va_arg(ap, size_t);
      int flags = va_arg(ap, int);
      ret = msync(addr, length, flags);
      break;
    }

    case __NR_munmap:
    {
      void *addr = va_arg(ap, void *);
      size_t length = va_arg(ap, size_t);
      ret = munmap(addr, length);
      break;
    }

    case __NR_nanosleep:
    {
      const struct timespec *req = va_arg(ap, const struct timespec *);
      struct timespec *rem = va_arg(ap, struct timespec *);
      ret = nanosleep(req, rem);
      break;
    }

    case __NR_open:
    {
      const char *path = va_arg(ap, const char *);
      int oflags = va_arg(ap, int);
      mode_t mode = va_arg(ap, mode_t);
      ret = open(path, oflags, mode);
      break;
    }

#if defined(CONFIG_BUILD_KERNEL)
    case __NR_pgalloc:
    {
      uintptr_t brkaddr = va_arg(ap, uintptr_t);
      unsigned int npages = va_arg(ap, unsigned int);
      pgalloc(brkaddr, npages);
      break;
    }
#endif

#if defined(CONFIG_PIPES) && CONFIG_DEV_PIPE_SIZE > 0
    case __NR_pipe2:
    {
      int *pipefd = va_arg(ap, int *);
      int flags = va_arg(ap, int);
      ret = pipe2(pipefd, flags);
      break;
    }
#endif

    case __NR_poll:
    {
      struct pollfd *fds = va_arg(ap, struct pollfd *);
      nfds_t nfds = va_arg(ap, nfds_t);
      int timeout = va_arg(ap, int);
      ret = poll(fds, nfds, timeout);
      break;
    }

#if !defined(CONFIG_BINFMT_DISABLE) && defined(CONFIG_LIBC_EXECFUNCS)
    case __NR_posix_spawn:
    {
      pid_t *pid = va_arg(ap, pid_t *);
      const char *path = va_arg(ap, const char *);
      const posix_spawn_file_actions_t *file_actions =
                        va_arg(ap, const posix_spawn_file_actions_t *);
      const posix_spawnattr_t *attrp = va_arg(ap, const posix_spawnattr_t *);
      const char *const *argv = va_arg(ap, const char *const *);
      const char *const *envp = va_arg(ap, const char *const *);
      ret = posix_spawn(pid, path, file_actions, attrp, argv, envp);
      break;
    }
#endif

    case __NR_ppoll:
    {
      struct pollfd *fds = va_arg(ap, struct pollfd *);
      nfds_t nfds = va_arg(ap, nfds_t);
      const struct timespec *timeout_ts =
                    va_arg(ap, const struct timespec *);
      const sigset_t *sigmask = va_arg(ap, const sigset_t *);
      ret = ppoll(fds, nfds, timeout_ts, sigmask);
      break;
    }

    case __NR_prctl:
    {
      int option = va_arg(ap, int);
      unsigned long arg2 = va_arg(ap, unsigned long);
      unsigned long arg3 = va_arg(ap, unsigned long);
      ret = prctl(option, arg2, arg3);
      break;
    }

    case __NR_pread:
    {
      int fd = va_arg(ap, int);
      void *buf = va_arg(ap, void *);
      size_t count = va_arg(ap, size_t);
      off_t offset = va_arg(ap, off_t);
      ret = pread(fd, buf, count, offset);
      break;
    }

    case __NR_pselect:
    {
      int nfds = va_arg(ap, int);
      fd_set *readfds = va_arg(ap, fd_set *);
      fd_set *writefds = va_arg(ap, fd_set *);
      fd_set *exceptfds = va_arg(ap, fd_set *);
      const struct timespec *timeout = va_arg(ap, const struct timespec *);
      const sigset_t *sigmask = va_arg(ap, const sigset_t *);
      ret = pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
      break;
    }

#if !defined(CONFIG_DISABLE_ENVIRON)
    case __NR_putenv:
    {
      const char *string = va_arg(ap, const char *);
      ret = putenv(string);
      break;
    }
#endif

    case __NR_pwrite:
    {
      int fd = va_arg(ap, int);
      const void *buf = va_arg(ap, const void *);
      size_t nbytes = va_arg(ap, size_t);
      off_t offset = va_arg(ap, off_t);
      ret = pwrite(fd, buf, nbytes, offset);
      break;
    }

    case __NR_read:
    {
      int fd = va_arg(ap, int);
      void *buf = va_arg(ap, void *);
      size_t count = va_arg(ap, size_t);
      ret = read(fd, buf, count);
      break;
    }

#if defined(CONFIG_PSEUDOFS_SOFTLINKS)
    case __NR_readlink:
    {
      const char *path = va_arg(ap, const char *);
      char *buf = va_arg(ap, char *);
      size_t bufsiz = va_arg(ap, size_t);
      ret = readlink(path, buf, bufsiz);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_recv:
    {
      int sockfd = va_arg(ap, int);
      void *buf = va_arg(ap, void *);
      size_t len = va_arg(ap, size_t);
      int flags = va_arg(ap, int);
      ret = recv(sockfd, buf, len, flags);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_recvfrom:
    {
      int sockfd = va_arg(ap, int);
      void *buf = va_arg(ap, void *);
      size_t len = va_arg(ap, size_t);
      int flags = va_arg(ap, int);
      struct sockaddr *src_addr = va_arg(ap, struct sockaddr *);
      socklen_t *addrlen = va_arg(ap, socklen_t *);
      ret = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_recvmsg:
    {
      int sockfd = va_arg(ap, int);
      struct msghdr *msg = va_arg(ap, struct msghdr *);
      int flags = va_arg(ap, int);
      ret = recvmsg(sockfd, msg, flags);
      break;
    }
#endif

    case __NR_rename:
    {
      const char *old_path = va_arg(ap, const char *);
      const char *new_path = va_arg(ap, const char *);
      ret = rename(old_path, new_path);
      break;
    }

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
    case __NR_rmdir:
    {
      const char *pathname = va_arg(ap, const char *);
      ret = rmdir(pathname);
      break;
    }
#endif

#if defined(CONFIG_MODULE)
    case __NR_rmmod:
    {
      void *handle = va_arg(ap, void *);
      ret = rmmod(handle);
      break;
    }
#endif

#if defined(CONFIG_SCHED_BACKTRACE)
    case __NR_sched_backtrace:
    {
      pid_t pid = va_arg(ap, int);
      void **buffer = va_arg(ap, void **);
      int size = va_arg(ap, int);
      int skip = va_arg(ap, int);
      ret = sched_backtrace(pid, buffer, size, skip);
      break;
    }
#endif

    case __NR_sched_getaffinity:
    {
      pid_t pid = va_arg(ap, pid_t);
      size_t cpusetsize = va_arg(ap, size_t);
      cpu_set_t *mask = va_arg(ap, cpu_set_t *);
      ret = sched_getaffinity(pid, cpusetsize, mask);
      break;
    }

    case __NR_sched_getcpu:
    {
      ret = sched_getcpu();
      break;
    }

    case __NR_sched_getparam:
    {
      pid_t pid = va_arg(ap, pid_t);
      struct sched_param *param = va_arg(ap, struct sched_param *);
      ret = sched_getparam(pid, param);
      break;
    }

    case __NR_sched_getscheduler:
    {
      pid_t pid = va_arg(ap, pid_t);
      ret = sched_getscheduler(pid);
      break;
    }

    case __NR_sched_lock:
    {
      sched_lock();
      break;
    }

    case __NR_sched_lockcount:
    {
      ret = sched_lockcount();
      break;
    }

    case __NR_sched_rr_get_interval:
    {
      pid_t pid = va_arg(ap, pid_t);
      struct timespec *interval = va_arg(ap, struct timespec *);
      ret = sched_rr_get_interval(pid, interval);
      break;
    }

    case __NR_sched_setaffinity:
    {
      pid_t pid = va_arg(ap, pid_t);
      size_t cpusetsize = va_arg(ap, size_t);
      cpu_set_t *mask = va_arg(ap, cpu_set_t *);
      ret = sched_setaffinity(pid, cpusetsize, mask);
      break;
    }

    case __NR_sched_setparam:
    {
      pid_t pid = va_arg(ap, pid_t);
      struct sched_param *param = va_arg(ap, struct sched_param *);
      ret = sched_setparam(pid, param);
      break;
    }

    case __NR_sched_setscheduler:
    {
      pid_t pid = va_arg(ap, pid_t);
      int policy = va_arg(ap, int);
      struct sched_param *param = va_arg(ap, struct sched_param *);
      ret = sched_setscheduler(pid, policy, param);
      break;
    }

    case __NR_sched_unlock:
    {
      sched_unlock();
      break;
    }

    case __NR_sched_yield:
    {
      ret = sched_yield();
      break;
    }

    case __NR_select:
    {
      int nfds = va_arg(ap, int);
      fd_set *readfds = va_arg(ap, fd_set *);
      fd_set *writefds = va_arg(ap, fd_set *);
      fd_set *exceptfds = va_arg(ap, fd_set *);
      struct timeval *timeout = va_arg(ap, struct timeval *);
      ret = select(nfds, readfds, writefds, exceptfds, timeout);
      break;
    }

#if defined(CONFIG_NET)
    case __NR_send:
    {
      int sockfd = va_arg(ap, int);
      const void *buf = va_arg(ap, const void *);
      size_t len = va_arg(ap, size_t);
      int flags = va_arg(ap, int);
      ret = send(sockfd, buf, len, flags);
      break;
    }
#endif

    case __NR_sendfile:
    {
      int out_fd = va_arg(ap, int);
      int in_fd = va_arg(ap, int);
      off_t *offset = va_arg(ap, off_t *);
      size_t count = va_arg(ap, size_t);
      ret = sendfile(out_fd, in_fd, offset, count);
      break;
    }

#if defined(CONFIG_NET)
    case __NR_sendmsg:
    {
      int sockfd = va_arg(ap, int);
      const struct msghdr *msg = va_arg(ap, const struct msghdr *);
      int flags = va_arg(ap, int);
      ret = sendmsg(sockfd, msg, flags);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_sendto:
    {
      int sockfd = va_arg(ap, int);
      const void *buf = va_arg(ap, const void *);
      size_t len = va_arg(ap, size_t);
      int flags = va_arg(ap, int);
      const struct sockaddr *dest_addr = va_arg(ap, const struct sockaddr *);
      socklen_t addrlen = va_arg(ap, socklen_t);
      ret = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
      break;
    }
#endif

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_setegid:
    {
      gid_t egid = va_arg(ap, gid_t);
      ret = setegid(egid);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_ENVIRON)
    case __NR_setenv:
    {
      const char *name = va_arg(ap, const char *);
      const char *value = va_arg(ap, const char *);
      int overwrite = va_arg(ap, int);
      ret = setenv(name, value, overwrite);
      break;
    }
#endif

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_seteuid:
    {
      uid_t euid = va_arg(ap, uid_t);
      ret = seteuid(euid);
      break;
    }
#endif

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_setgid:
    {
      gid_t gid = va_arg(ap, gid_t);
      ret = setgid(gid);
      break;
    }
#endif

    case __NR_sethostname:
    {
      const char *name = va_arg(ap, const char *);
      size_t len = va_arg(ap, size_t);
      ret = sethostname(name, len);
      break;
    }

#if !defined(CONFIG_DISABLE_POSIX_TIMERS)
    case __NR_setitimer:
    {
      int which = va_arg(ap, int);
      const struct itimerval *value = va_arg(ap, const struct itimerval *);
      struct itimerval *oldvalue = va_arg(ap, struct itimerval *);
      ret = setitimer(which, value, oldvalue);
      break;
    }
#endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_SOCKOPTS)
    case __NR_setsockopt:
    {
      int sockfd = va_arg(ap, int);
      int level = va_arg(ap, int);
      int option = va_arg(ap, int);
      const void *value = va_arg(ap, const void *);
      socklen_t value_len = va_arg(ap, socklen_t);
      ret = setsockopt(sockfd, level, option, value, value_len);
      break;
    }
#endif

    case __NR_settimeofday:
    {
      const struct timeval *tv = va_arg(ap, const struct timeval *);
      const struct timezone *tz = va_arg(ap, const struct timezone *);
      ret = settimeofday(tv, tz);
      break;
    }

#if defined(CONFIG_SCHED_USER_IDENTITY)
    case __NR_setuid:
    {
      uid_t euid = va_arg(ap, uid_t);
      ret = seteuid(euid);
      break;
    }
#endif

#if defined(CONFIG_MM_SHM)
    case __NR_shmctl:
    {
      int shmid = va_arg(ap, int);
      int cmd = va_arg(ap, int);
      struct shmid_ds *buf = va_arg(ap, struct shmid_ds *);
      ret = shmctl(shmid, cmd, buf);
      break;
    }
#endif

#if defined(CONFIG_MM_SHM)
    case __NR_shmdt:
    {
      const void *shmaddr = va_arg(ap, const void *);
      ret = shmdt(shmaddr);
      break;
    }
#endif

#if defined(CONFIG_MM_SHM)
    case __NR_shmget:
    {
      key_t key = va_arg(ap, key_t);
      size_t size = va_arg(ap, size_t);
      int shmflg = va_arg(ap, int);
      ret = shmget(key, size, shmflg);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_shutdown:
    {
      int sockfd = va_arg(ap, int);
      int how = va_arg(ap, int);
      ret = shutdown(sockfd, how);
      break;
    }
#endif

    case __NR_sigaction:
    {
      int signo = va_arg(ap, int);
      const struct sigaction *act = va_arg(ap, const struct sigaction *);
      struct sigaction *old_act = va_arg(ap, struct sigaction *);
      ret = sigaction(signo, act, old_act);
      break;
    }

    case __NR_signal:
    {
      int signo = va_arg(ap, int);
      _sa_handler_t func = va_arg(ap, _sa_handler_t);
      ret = signal(signo, func);
      break;
    }

#if defined(CONFIG_SIGNAL_FD)
    case __NR_signalfd:
    {
      int fd = va_arg(ap, int);
      sigset_t *mask = va_arg(ap, sigset_t *);
      int flags = va_arg(ap, int);
      ret = signalfd(fd, mask, flags);
      break;
    }
#endif

    case __NR_sigpending:
    {
      sigset_t *set = va_arg(ap, sigset_t *);
      ret = sigpending(set);
      break;
    }

    case __NR_sigprocmask:
    {
      int how = va_arg(ap, int);
      const sigset_t *set = va_arg(ap, const sigset_t *);
      sigset_t *oldset = va_arg(ap, sigset_t *);
      ret = sigprocmask(how, set, oldset);
      break;
    }

    case __NR_sigqueue:
    {
      int pid = va_arg(ap, int);
      int signo = va_arg(ap, int);
      union sigval value = va_arg(ap, union sigval);
      ret = sigqueue(pid, signo, value);
      break;
    }

    case __NR_sigsuspend:
    {
      const sigset_t *mask = va_arg(ap, sigset_t *);
      ret = sigsuspend(mask);
      break;
    }

    case __NR_sigtimedwait:
    {
      const sigset_t *set = va_arg(ap, sigset_t *);
      struct siginfo *value = va_arg(ap, struct siginfo *);
      struct timespec *timeout = va_arg(ap, struct timespec *);
      ret = sigtimedwait(set, value, timeout);
      break;
    }

    case __NR_sigwaitinfo:
    {
      const sigset_t *set = va_arg(ap, const sigset_t *);
      struct siginfo *info = va_arg(ap, struct siginfo *);
      ret = sigwaitinfo(set, info);
      break;
    }

#if defined(CONFIG_NET)
    case __NR_socket:
    {
      int domain = va_arg(ap, int);
      int type = va_arg(ap, int);
      int protocol = va_arg(ap, int);
      ret = socket(domain, type, protocol);
      break;
    }
#endif

#if defined(CONFIG_NET)
    case __NR_socketpair:
    {
      int domain = va_arg(ap, int);
      int type = va_arg(ap, int);
      int protocol = va_arg(ap, int);
      int *sv = va_arg(ap, int *);
      ret = socketpair(domain, type, protocol, sv);
      break;
    }
#endif

    case __NR_stat:
    {
      const char *path = va_arg(ap, const char *);
      struct stat *buf = va_arg(ap, struct stat *);
      ret = stat(path, buf);
      break;
    }

    case __NR_statfs:
    {
      const char *path = va_arg(ap, const char *);
      struct statfs *buf = va_arg(ap, struct statfs *);
      ret = statfs(path, buf);
      break;
    }

#if defined(CONFIG_PSEUDOFS_SOFTLINKS)
    case __NR_symlink:
    {
      const char *target = va_arg(ap, const char *);
      const char *linkpath = va_arg(ap, const char *);
      ret = symlink(target, linkpath);
      break;
    }
#endif

    case __NR_sync:
    {
      sync();
      break;
    }

#if defined(CONFIG_FS_SHMFS)
    case __NR_shm_open:
    {
      const char *name = va_arg(ap, const char *);
      int oflag = va_arg(ap, int);
      mode_t mode = va_arg(ap, mode_t);
      ret = shm_open(name, oflag, mode);
      break;
    }
#endif

#if defined(CONFIG_FS_SHMFS)
    case __NR_shm_unlink:
    {
      const char *name = va_arg(ap, const char *);
      ret = shm_unlink(name);
      break;
    }
#endif

#if defined(CONFIG_MM_SHM)
    case __NR_shmat:
    {
      int shmid = va_arg(ap, int);
      const void *shmaddr = va_arg(ap, const void *);
      int shmflg = va_arg(ap, int);
      void *shm_ret = shmat(shmid, shmaddr, shmflg);
      if (!shm_ret)
      {
        ret = -1;
      }

      break;
    }
#endif

    case __NR_sysinfo:
    {
      struct sysinfo *info  = va_arg(ap, struct sysinfo *);
      ret = sysinfo(info);
      break;
    }

#if !defined(CONFIG_BUILD_KERNEL)
    case __NR_task_create:
    {
      const char *name = va_arg(ap, const char *);
      int priority = va_arg(ap, int);
      int stack_size = va_arg(ap, int);
      main_t entry = va_arg(ap, main_t);
      char *const *argv = va_arg(ap, char *const *);
      ret = task_create(name, priority, stack_size, entry, argv);
      break;
    }
#endif

#if !defined(CONFIG_BUILD_KERNEL)
    case __NR_task_delete:
    {
      pid_t pid = va_arg(ap, pid_t);
      ret = task_delete(pid);
      break;
    }
#endif

#if !defined(CONFIG_BUILD_KERNEL)
    case __NR_task_restart:
    {
      pid_t pid = va_arg(ap, pid_t);
      ret = task_restart(pid);
      break;
    }
#endif

#if !defined(CONFIG_BUILD_KERNEL)
    case __NR_task_spawn:
    {
      const char *path = va_arg(ap, const char *);
      main_t entry = va_arg(ap, main_t);
      const posix_spawn_file_actions_t *file_actions =
                  va_arg(ap, const posix_spawn_file_actions_t *);
      const posix_spawnattr_t *attr = va_arg(ap, const posix_spawnattr_t *);
      char *const *argv = va_arg(ap, char *const *);
      char *const *envp = va_arg(ap, char *const *);
      ret = task_spawn(path, entry, file_actions, attr, argv, envp);
      break;
    }
#endif

    case __NR_tgkill:
    {
      pid_t pid = va_arg(ap, pid_t);
      pid_t tid = va_arg(ap, pid_t);
      int signo = va_arg(ap, int);
      ret = tgkill(pid, tid, signo);
      break;
    }

    case __NR_time:
    {
      time_t *timep = va_arg(ap, time_t *);
      ret = time(timep);
      break;
    }

#if !defined(CONFIG_DISABLE_POSIX_TIMERS)
    case __NR_timer_create:
    {
      int clockid = va_arg(ap, int);
      struct sigevent *sevp = va_arg(ap, struct sigevent *);
      timer_t *timerid = va_arg(ap, timer_t *);
      ret = timer_create(clockid, sevp, timerid);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_POSIX_TIMERS)
    case __NR_timer_delete:
    {
      timer_t timerid = va_arg(ap, timer_t);
      ret = timer_delete(timerid);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_POSIX_TIMERS)
    case __NR_timer_getoverrun:
    {
      timer_t timerid = va_arg(ap, timer_t);
      ret = timer_getoverrun(timerid);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_POSIX_TIMERS)
    case __NR_timer_gettime:
    {
      int clockid = va_arg(ap, int);
      struct itimerspec *curr_value = va_arg(ap, struct itimerspec *);
      ret = timer_gettime(clockid, curr_value);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_POSIX_TIMERS)
    case __NR_timer_settime:
    {
      int clockid = va_arg(ap, int);
      int flags = va_arg(ap, int);
      const struct itimerspec *new_value =
                    va_arg(ap, const struct itimerspec *);
      struct itimerspec *old_value = va_arg(ap, struct itimerspec *);
      ret = timer_settime(clockid, flags, new_value, old_value);
      break;
    }
#endif

#if defined(CONFIG_TIMER_FD)
    case __NR_timerfd_create:
    {
      int clockid = va_arg(ap, int);
      int flags = va_arg(ap, int);
      ret = timerfd_create(clockid, flags);
      break;
    }
#endif

#if defined(CONFIG_TIMER_FD)
    case __NR_timerfd_gettime:
    {
      int fd = va_arg(ap, int);
      struct itimerspec *curr_value = va_arg(ap, struct itimerspec *);
      ret = timerfd_gettime(fd, curr_value);
      break;
    }
#endif

#if defined(CONFIG_TIMER_FD)
    case __NR_timerfd_settime:
    {
      int fd = va_arg(ap, int);
      int flags = va_arg(ap, int);
      const struct itimerspec *new_value =
                    va_arg(ap, const struct itimerspec *);
      struct itimerspec *old_value = va_arg(ap, struct itimerspec *);
      ret = timerfd_settime(fd, flags, new_value, old_value);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
    case __NR_umount2:
    {
      const char *target = va_arg(ap, const char *);
      unsigned int flags = va_arg(ap, unsigned int);
      ret = umount2(target, flags);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
    case __NR_unlink:
    {
      const char *pathname = va_arg(ap, const char *);
      ret = unlink(pathname);
      break;
    }
#endif

#if !defined(CONFIG_DISABLE_ENVIRON)
    case __NR_unsetenv:
    {
      const char *name = va_arg(ap, const char *);
      ret = unsetenv(name);
      break;
    }
#endif

    case __NR_up_fork:
    {
      ret = up_fork();
      break;
    }

    case __NR_utimens:
    {
      const char *path = va_arg(ap, const char *);
      const struct timespec *times = va_arg(ap, const struct timespec *);
      ret = utimens(path, times);
      break;
    }

#if defined(CONFIG_SCHED_WAITPID) && defined(CONFIG_SCHED_HAVE_PARENT)
    case __NR_wait:
    {
      int *stat_loc = va_arg(ap, int *);
      ret = wait(stat_loc);
      break;
    }
#endif

#if defined(CONFIG_SCHED_WAITPID) && defined(CONFIG_SCHED_HAVE_PARENT)
    case __NR_waitid:
    {
      idtype_t idtype = (int) va_arg(ap, int);
      id_t id = va_arg(ap, id_t);
      siginfo_t *info = va_arg(ap, siginfo_t *);
      int options = va_arg(ap, int);
      ret = waitid(idtype, id, info, options);
      break;
    }
#endif

#ifdef CONFIG_SCHED_WAITPID
    case __NR_waitpid:
    {
      pid_t pid = va_arg(ap, pid_t);
      int *stat_loc = va_arg(ap, int *);
      int options = va_arg(ap, int);
      ret = waitpid(pid, stat_loc, options);
      break;
    }
#endif

    case __NR_write:
    {
      int fd = va_arg(ap, int);
      const void *buf = va_arg(ap, const void *);
      size_t count = va_arg(ap, size_t);
      ret = write(fd, buf, count);
      break;
    }

    case __NR_futex:
    {
      /* nuttx do not support futex */

      ret = -1;
      break;
    }

    case __NR_clock_getres:
    {
      clockid_t clockid = va_arg(ap, clockid_t);
      struct timespec *res = va_arg(ap, struct timespec *);
      ret = clock_getres(clockid, res);
      break;
    }

    case __NR_ext_alarm:
    {
      unsigned int seconds = va_arg(ap, unsigned int);
      ret = alarm(seconds);
      break;
    }

    default:
      ret = -1;
      break;
  }

  va_end(ap);
  return ret;
}
