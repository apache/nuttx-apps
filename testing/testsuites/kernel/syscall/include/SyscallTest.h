/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/include/SyscallTest.h
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

#ifndef __SYSCALLTEST_H
#define __SYSCALLTEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <dirent.h>

/* Functions to help test */

int checknames(char *pfilnames, DIR *ddir);
int check_and_report_ftruncatetest(int fd, off_t offset, char data,
                                   off_t trunc_len);
int tst_fill_file(const char *path, char pattern, size_t bs,
                  size_t bcount);

char *sock_addr(const struct sockaddr *sa, socklen_t salen, char *res,
                size_t len);

/* @return port in network byte order.
 */

unsigned short get_unused_port(void(cleanup_fn)(void),
                               unsigned short family, int type);

int safe_close(int fildes);

int safe_socket(int domain, int type, int protocol);

int safe_connect(int sockfd, const struct sockaddr *addr,
                 socklen_t addrlen);

int safe_getsockname(int sockfd, struct sockaddr *addr,
                     socklen_t *addrlen);

int safe_bind(int socket, const struct sockaddr *address,
              socklen_t address_len);

int safe_listen(int socket, int backlog);

int safe_open(const char *pathname, int oflags, ...);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The test files generated during the 'syscall-test' are stored in this
 * directory
 */

#define SYSCALL_TEST_DIR "syscall_test_dir"

#define MOUNT_DIR CONFIG_TESTS_TESTSUITES_MOUNT_DIR

#define SAFE_OPEN(pathname, oflags, ...)                                \
  safe_open((pathname), (oflags), ##__VA_ARGS__)

#define SAFE_FCNTL(fd, cmd, ...)                                        \
  ({                                                                    \
    int tst_ret_ = fcntl(fd, cmd, ##__VA_ARGS__);                       \
    if (tst_ret_ == -1)                                                 \
      {                                                                 \
        syslog(LOG_ERR, "fcntl(%i,%s,...) failed", fd, #cmd);           \
        fail_msg("test fail !");                                        \
      }                                                                 \
    tst_ret_ == -1 ? 0 : tst_ret_;                                      \
  })

#define SAFE_CLOSE(fd)                                                  \
  do                                                                    \
    {                                                                   \
      safe_close((fd));                                                 \
      fd = -1;                                                          \
  } while (0)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* quickly fill a test file */

int cmtestfillfile(const char *path, char pattern, size_t bs,
                   size_t bcount);

/* setup function */

int test_nuttx_syscall_test_group_setup(void **state);

/* teardown function */

int test_nuttx_syscall_test_group_teardown(void **state);

/* tools */

int checknames(char *pfilnames, DIR *ddir);
int check_and_report_ftruncatetest(int fd, off_t offset, char data,
                                   off_t trunc_len);
int tst_fill_fd(int fd, char pattern, size_t bs, size_t bcount);
int tst_fill_file(const char *path, char pattern, size_t bs,
                  size_t bcount);
int safe_open(const char *pathname, int oflags, ...);
ssize_t safe_write(int fildes, const void *buf, size_t nbyte);
off_t safe_lseek(int fd, off_t offset, int whence);
int safe_close(int fildes);
void safe_touch(const char *file, const int lineno, const char *pathname,
                mode_t mode);
#ifdef CONFIG_NET
char *sock_addr(const struct sockaddr *sa, socklen_t salen, char *res,
                size_t len);
unsigned short get_unused_port(void(cleanup_fn)(void),
                               unsigned short family, int type);
int safe_socket(int domain, int type, int protocol);
int safe_connect(int sockfd, const struct sockaddr *addr,
                 socklen_t addrlen);
int safe_getsockname(int sockfd, struct sockaddr *addr,
                     socklen_t *addrlen);
int safe_bind(int socket, const struct sockaddr *address,
              socklen_t address_len);
int safe_listen(int socket, int backlog);
#endif

/* test case function */

/* cases/chdir_test.c
 * ***********************************************/

void test_nuttx_syscall_chdir01(FAR void **state);
void test_nuttx_syscall_chdir02(FAR void **state);

/* cases/accept_test.c
 * ***********************************************/

void test_nuttx_syscall_accept01(FAR void **state);

/* cases/getitimer_test.c
 * ***********************************************/

void test_nuttx_syscall_getitimer01(FAR void **state);

/* cases/clock_gettime_test.c
 * ****************************************/

void test_nuttx_syscall_clockgettime01(FAR void **state);

/* cases/clock_nanosleep_test.c
 * **************************************/

void test_nuttx_syscall_clocknanosleep01(FAR void **state);
void test_nuttx_syscall_clocknanosleep02(FAR void **state);

/* cases/clock_settime_test.c
 * ****************************************/

void test_nuttx_syscall_clocksettime01(FAR void **state);

/* cases/close_test.c
 * ************************************************/

void test_nuttx_syscall_close01(FAR void **state);
void test_nuttx_syscall_close02(FAR void **state);
void test_nuttx_syscall_close03(FAR void **state);

/* cases/creat_test.c
 * ************************************************/

void test_nuttx_syscall_creat01(FAR void **state);
void test_nuttx_syscall_creat02(FAR void **state);

/* cases/fcntl_test.c
 * ************************************************/

void test_nuttx_syscall_fcntl01(FAR void **state);
void test_nuttx_syscall_fcntl02(FAR void **state);
void test_nuttx_syscall_fcntl03(FAR void **state);
void test_nuttx_syscall_fcntl04(FAR void **state);
void test_nuttx_syscall_fcntl05(FAR void **state);
void test_nuttx_syscall_fcntl06(FAR void **state);

/* cases/fsatfs_test.c
 * ***********************************************/

void test_nuttx_syscall_fstatfs01(FAR void **state);

/* cases/fsync_test.c
 * ************************************************/

void test_nuttx_syscall_fsync01(FAR void **state);
void test_nuttx_syscall_fsync02(FAR void **state);
void test_nuttx_syscall_fsync03(FAR void **state);

/* cases/ftruncate_test.c
 * ********************************************/

void test_nuttx_syscall_ftruncate01(FAR void **state);

/* cases/getcwd_test.c
 * ***********************************************/

void test_nuttx_syscall_getcwd01(FAR void **state);
void test_nuttx_syscall_getcwd02(FAR void **state);

/* cases/getpid_test.c
 * ***********************************************/

void test_nuttx_syscall_getpid01(FAR void **state);

/* cases/getppid_test.c
 * **********************************************/

void test_nuttx_syscall_getppid01(FAR void **state);

/* cases/gethostname_test.c
 * ******************************************/

void test_nuttx_syscall_gethostname01(FAR void **state);

/* cases/getTimeofday_test.c
 * *****************************************/

void test_nuttx_syscall_gettimeofday01(FAR void **state);

/* cases/lseek_test.c
 * ************************************************/

void test_nuttx_syscall_lseek01(FAR void **state);
void test_nuttx_syscall_lseek07(FAR void **state);

/* cases/lstat_test.c
 * ************************************************/

void test_nuttx_syscall_lstat01(FAR void **state);

/* cases/dup_test.c
 * **************************************************/

void test_nuttx_syscall_dup01(FAR void **state);
void test_nuttx_syscall_dup02(FAR void **state);
void test_nuttx_syscall_dup03(FAR void **state);
void test_nuttx_syscall_dup04(FAR void **state);
void test_nuttx_syscall_dup05(FAR void **state);

/* cases/dup2_test.c
 * **************************************************/

void test_nuttx_syscall_dup201(FAR void **state);
void test_nuttx_syscall_dup202(FAR void **state);

/* cases/fpathconf_test.c
 * *********************************************/

void test_nuttx_syscall_fpathconf01(FAR void **state);

/* cases/getegid_test.c
 * ***********************************************/

void test_nuttx_syscall_getegid01(FAR void **state);
void test_nuttx_syscall_getegid02(FAR void **state);

/* cases/geteuid_test.c
 * ***********************************************/

void test_nuttx_syscall_geteuid01(FAR void **state);
void test_nuttx_syscall_geteuid02(FAR void **state);

/* cases/getgid_test.c
 * ***********************************************/

void test_nuttx_syscall_getgid01(FAR void **state);
void test_nuttx_syscall_getgid02(FAR void **state);

/* cases/getuid_test.c
 * ***********************************************/

void test_nuttx_syscall_getuid01(FAR void **state);
void test_nuttx_syscall_getuid02(FAR void **state);

/* cases/pathconf_test.c
 * *********************************************/

void test_nuttx_syscall_pathconf01(FAR void **state);

/* cases/pipe_test.c
 * *************************************************/

void test_nuttx_syscall_pipe01(FAR void **state);
void test_nuttx_syscall_pipe02(FAR void **state);

/* cases/pread_test.c
 * ************************************************/

void test_nuttx_syscall_pread01(FAR void **state);

/* cases/pwrite_test.c
 * ***********************************************/

void test_nuttx_syscall_pwrite01(FAR void **state);
void test_nuttx_syscall_pwrite02(FAR void **state);

/* cases/rmdir_test.c
 * ************************************************/

void test_nuttx_syscall_rmdir01(FAR void **state);
void test_nuttx_syscall_rmdir02(FAR void **state);

/* ases/syscall_truncate_test.c
 * **********************************************/

void test_nuttx_syscall_truncate01(FAR void **state);

/* cases/unlink_test.c
 * ***********************************************/

void test_nuttx_syscall_unlink01(FAR void **state);

/* cases/nansleep_test.c
 * *********************************************/

void test_nuttx_syscall_nansleep01(FAR void **state);
void test_nuttx_syscall_nansleep02(FAR void **state);

/* cases/time_test.c
 * *************************************************/

void test_nuttx_syscall_time01(FAR void **state);
void test_nuttx_syscall_time02(FAR void **state);

/* cases/timer_create_test.c
 * *****************************************/

void test_nuttx_syscall_timercreate01(FAR void **state);

/* cases/timer_delete_test.c
 * *****************************************/

void test_nuttx_syscall_timerdelete01(FAR void **state);

/* cases/timer_gettime_test.c
 * ****************************************/

void test_nuttx_syscall_timergettime01(FAR void **state);

/* cases/mkdir_test.c
 * ************************************************/

void test_nuttx_syscall_mkdir01(FAR void **state);
void test_nuttx_syscall_mkdir02(FAR void **state);
void test_nuttx_syscall_mkdir03(FAR void **state);

/*  cases/syscall_sched_test.c
 * ***********************************************/

void test_nuttx_syscall_sched01(FAR void **state);
void test_nuttx_syscall_sched02(FAR void **state);
void test_nuttx_syscall_sched03(FAR void **state);
void test_nuttx_syscall_sched04(FAR void **state);

/* cases/write_test.c
 * ************************************************/

void test_nuttx_syscall_write01(FAR void **state);
void test_nuttx_syscall_write02(FAR void **state);
void test_nuttx_syscall_write03(FAR void **state);

/* cases/read_test.c
 * *************************************************/

void test_nuttx_syscall_read01(FAR void **state);
void test_nuttx_syscall_read02(FAR void **state);
void test_nuttx_syscall_read03(FAR void **state);
void test_nuttx_syscall_read04(FAR void **state);

/* cases/symlink_test.c
 * **********************************************/

void test_nuttx_syscall_symlink01(FAR void **state);
void test_nuttx_syscall_symlink02(FAR void **state);

/* cases/socket_test.c
 * **********************************************/

void test_nuttx_syscall_sockettest01(FAR void **state);
void test_nuttx_syscall_sockettest02(FAR void **state);

/* cases/getpeername_test.c
 * **********************************************/

void test_nuttx_syscall_connect01(FAR void **state);

/* cases/getpeername_test.c
 * **********************************************/

void test_nuttx_syscall_getpeername01(FAR void **state);

/* cases/getsocketopt_test.c
 * **********************************************/

void test_nuttx_syscall_getsockopt01(FAR void **state);

/* cases/recvfrom_test.c
 * **********************************************/

void test_nuttx_syscall_recvfromtest01(FAR void **state);

/* cases/setsocketopt01_test.c
 * **********************************************/

void test_nuttx_syscall_setsockopt01(FAR void **state);

/* cases/listen_test.c
 * **********************************************/

void test_nuttx_syscall_listen01(FAR void **state);

/* cases/socketpair_test.c
 * **********************************************/

void test_nuttx_syscall_socketpair01(FAR void **state);
void test_nuttx_syscall_socketpair02(FAR void **state);

/* cases/bind_test.c
 * **********************************************/

void test_nuttx_syscall_bind01(FAR void **state);

#endif
