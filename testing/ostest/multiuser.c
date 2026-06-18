/****************************************************************************
 * apps/testing/ostest/multiuser.c
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

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MU_UID1   1000
#define MU_GID1   1000
#define MU_UID2   2000
#define MU_GID2   2000
#define MU_GID3   3000

#define MU_PSEUDO_PERM   "/ostest_mu_perm"
#define MU_PSEUDO_SECRET "/ostest_mu_secret"
#define MU_PSEUDO_USER   "/ostest_mu_user"

#ifndef CONFIG_LIBC_TMPDIR
#  define CONFIG_LIBC_TMPDIR "/tmp"
#endif

#define MU_TMPFS_SECRET CONFIG_LIBC_TMPDIR "/ostest_mu_secret"

#define MU_CHILD_PRIORITY 100

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mu_ctx_s
{
  int failures;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void mu_fail(FAR struct mu_ctx_s *ctx, FAR const char *fmt, ...)
{
  va_list ap;

  ctx->failures++;
  printf("  FAIL: ");
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  printf("\n");
}

static void mu_pass(FAR const char *fmt, ...)
{
  va_list ap;

  printf("  PASS: ");
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  printf("\n");
}

static void mu_check_eq(FAR struct mu_ctx_s *ctx, FAR const char *label,
                        long got, long want)
{
  if (got == want)
    {
      mu_pass("%s == %ld", label, want);
    }
  else
    {
      mu_fail(ctx, "%s: expected %ld, got %ld", label, want, got);
    }
}

static int mu_restore_root(FAR struct mu_ctx_s *ctx)
{
  if (seteuid(0) != 0)
    {
      mu_fail(ctx, "seteuid(0) restore errno=%d", errno);
      return -1;
    }

  if (setegid(0) != 0)
    {
      mu_fail(ctx, "setegid(0) restore errno=%d", errno);
      return -1;
    }

  if (geteuid() != 0 || getegid() != 0)
    {
      mu_fail(ctx, "restore root effective credentials failed "
              "(euid=%d egid=%d)", geteuid(), getegid());
      return -1;
    }

  return 0;
}

static int mu_set_effective(FAR struct mu_ctx_s *ctx, uid_t uid, gid_t gid)
{
  /* NuttX grants arbitrary seteuid/setegid only while the effective ID
   * is 0.  With real UID 0 (flat NSH), restore effective root before
   * switching user, matching nsh_switch_credentials().
   */

  if (getuid() == 0 && (geteuid() != 0 || getegid() != 0))
    {
      if (seteuid(0) != 0)
        {
          mu_fail(ctx, "seteuid(0) before switch errno=%d", errno);
          return -1;
        }

      if (setegid(0) != 0)
        {
          mu_fail(ctx, "setegid(0) before switch errno=%d", errno);
          return -1;
        }
    }

  if (seteuid(uid) != 0)
    {
      mu_fail(ctx, "seteuid(%u) errno=%d", (unsigned int)uid, errno);
      return -1;
    }

  if (setegid(gid) != 0)
    {
      mu_fail(ctx, "setegid(%u) errno=%d", (unsigned int)gid, errno);
      return -1;
    }

  if (geteuid() != (int)uid || getegid() != (int)gid)
    {
      mu_fail(ctx, "effective credential switch failed "
              "(have euid=%d egid=%d)", geteuid(), getegid());
      return -1;
    }

  return 0;
}

static int mu_expect_ok(FAR struct mu_ctx_s *ctx, FAR const char *op,
                        int ret)
{
  if (ret == 0)
    {
      mu_pass("%s", op);
      return 0;
    }

  mu_fail(ctx, "%s (ret=%d errno=%d)", op, ret, errno);
  return -1;
}

static int mu_expect_denied(FAR struct mu_ctx_s *ctx, FAR const char *op,
                            int ret)
{
  int err = errno;

  if (ret != 0 && (err == EPERM || err == EACCES))
    {
      mu_pass("%s denied errno=%d", op, err);
      return 0;
    }

  if (ret == 0)
    {
      mu_fail(ctx, "%s succeeded (expected EPERM/EACCES, euid=%d)",
              op, geteuid());
    }
  else
    {
      mu_fail(ctx, "%s (ret=%d errno=%d, expected EPERM/EACCES)",
              op, ret, err);
    }

  return -1;
}

static int mu_verify_owner(FAR struct mu_ctx_s *ctx, FAR const char *path,
                           uid_t uid, gid_t gid)
{
  struct stat st;

  if (stat(path, &st) != 0)
    {
      mu_fail(ctx, "stat(%s) errno=%d", path, errno);
      return -1;
    }

  if (st.st_uid != uid || st.st_gid != gid)
    {
      mu_fail(ctx, "%s owner expected %u:%u got %u:%u",
              path, (unsigned int)uid, (unsigned int)gid,
              (unsigned int)st.st_uid, (unsigned int)st.st_gid);
      return -1;
    }

  mu_pass("%s owner %u:%u", path, (unsigned int)uid, (unsigned int)gid);
  return 0;
}

#if defined(CONFIG_SCHED_USER_IDENTITY)

static int multiuser_effective_test(FAR struct mu_ctx_s *ctx)
{
  int ret;

  printf("multiuser: effective UID/GID switching\n");

  mu_check_eq(ctx, "initial uid", getuid(), 0);
  mu_check_eq(ctx, "initial euid", geteuid(), 0);
  mu_check_eq(ctx, "initial gid", getgid(), 0);
  mu_check_eq(ctx, "initial egid", getegid(), 0);

  ret = seteuid(MU_UID1);
  if (mu_expect_ok(ctx, "root seteuid(1000)", ret) != 0)
    {
      mu_restore_root(ctx);
      return ctx->failures;
    }

  mu_check_eq(ctx, "euid after seteuid(1000)", geteuid(), MU_UID1);

  ret = seteuid(0);
  if (mu_expect_ok(ctx, "root seteuid(0) restore", ret) != 0)
    {
      mu_restore_root(ctx);
      return ctx->failures;
    }

  mu_check_eq(ctx, "euid restored to 0", geteuid(), 0);

  ret = setegid(MU_GID2);
  if (mu_expect_ok(ctx, "root setegid(2000)", ret) != 0)
    {
      mu_restore_root(ctx);
      return ctx->failures;
    }

  mu_check_eq(ctx, "egid after setegid(2000)", getegid(), MU_GID2);

  ret = setegid(0);
  if (mu_expect_ok(ctx, "root setegid(0) restore", ret) != 0)
    {
      mu_restore_root(ctx);
      return ctx->failures;
    }

  mu_check_eq(ctx, "egid restored to 0", getegid(), 0);

  return ctx->failures;
}

#if defined(CONFIG_SCHED_WAITPID) && !defined(CONFIG_BUILD_KERNEL)

static int multiuser_suid_child(int argc, FAR char *argv[])
{
  struct mu_ctx_s ctx =
  {
    0
  };

  int ret;

  (void)argc;
  (void)argv;

  printf("multiuser_suid_child: saved set-UID/GID semantics\n");

  ret = setuid(MU_UID1);
  if (mu_expect_ok(&ctx, "setuid(1000) as root", ret) != 0)
    {
      return EXIT_FAILURE;
    }

  mu_check_eq(&ctx, "uid after setuid(1000)", getuid(), MU_UID1);
  mu_check_eq(&ctx, "euid after setuid(1000)", geteuid(), MU_UID1);

  ret = seteuid(0);
  mu_expect_denied(&ctx, "non-root seteuid(0)", ret);
  mu_check_eq(&ctx, "euid unchanged after denied seteuid(0)", geteuid(),
              MU_UID1);

  ret = seteuid(MU_UID1);
  mu_expect_ok(&ctx, "non-root seteuid(1000)", ret);

  ret = setegid(MU_GID2);
  mu_expect_ok(&ctx, "root-group setegid(2000)", ret);
  mu_check_eq(&ctx, "egid after setegid(2000)", getegid(), MU_GID2);

  ret = setegid(0);
  mu_expect_ok(&ctx, "root-group setegid(0) restore", ret);

  ret = setgid(MU_GID3);
  mu_expect_ok(&ctx, "setgid(3000)", ret);
  mu_check_eq(&ctx, "gid after setgid(3000)", getgid(), MU_GID3);
  mu_check_eq(&ctx, "egid after setgid(3000)", getegid(), MU_GID3);

  ret = setegid(0);
  mu_expect_denied(&ctx, "non-root setegid(0)", ret);
  mu_check_eq(&ctx, "egid unchanged after denied setegid(0)", getegid(),
              MU_GID3);

  printf("multiuser_suid_child: %d failure(s)\n", ctx.failures);
  return ctx.failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int multiuser_suid_test(FAR struct mu_ctx_s *ctx)
{
  pid_t pid;
  int status;

  printf("multiuser: saved set-UID/GID semantics (child task)\n");

  pid = task_create("mu_suid", MU_CHILD_PRIORITY, STACKSIZE,
                    multiuser_suid_child, NULL);
  if (pid < 0)
    {
      mu_fail(ctx, "task_create(mu_suid) failed");
      return ctx->failures;
    }

  if (waitpid(pid, &status, 0) != pid)
    {
      mu_fail(ctx, "waitpid(mu_suid) failed errno=%d", errno);
      return ctx->failures;
    }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
    {
      mu_fail(ctx, "mu_suid child exited with status=%d", status);
    }
  else
    {
      mu_pass("mu_suid child completed successfully");
    }

  mu_check_eq(ctx, "parent euid after child", geteuid(), 0);
  mu_check_eq(ctx, "parent egid after child", getegid(), 0);

  return ctx->failures;
}

#endif /* CONFIG_SCHED_WAITPID && !CONFIG_BUILD_KERNEL */

#if defined(CONFIG_FS_PERMISSION) && defined(CONFIG_PSEUDOFS_ATTRIBUTES) && \
    defined(CONFIG_PSEUDOFS_FILE)

static int multiuser_open_secret(FAR struct mu_ctx_s *ctx,
                                 FAR const char *path, int expect_ok)
{
  int fd;

  fd = open(path, O_RDONLY);
  if (expect_ok)
    {
      if (fd < 0)
        {
          mu_fail(ctx, "open(%s) errno=%d (expected success)", path, errno);
          return -1;
        }

      close(fd);
      mu_pass("open(%s) allowed", path);
      return 0;
    }

  if (fd >= 0)
    {
      close(fd);
      mu_fail(ctx, "open(%s) succeeded (expected EACCES, euid=%d)",
              path, geteuid());
      return -1;
    }

  if (errno != EACCES)
    {
      mu_fail(ctx, "open(%s) errno=%d (expected EACCES)", path, errno);
      return -1;
    }

  mu_pass("open(%s) denied with EACCES", path);
  return 0;
}

static int multiuser_pseudofs_test(FAR struct mu_ctx_s *ctx)
{
  int fd;
  int ret;

  printf("multiuser: pseudoFS chmod/chown/open permissions\n");

  unlink(MU_PSEUDO_PERM);
  unlink(MU_PSEUDO_SECRET);
  unlink(MU_PSEUDO_USER);

  fd = open(MU_PSEUDO_PERM, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      mu_fail(ctx, "create %s errno=%d", MU_PSEUDO_PERM, errno);
      goto out;
    }

  close(fd);
  mu_verify_owner(ctx, MU_PSEUDO_PERM, 0, 0);

  ret = chmod(MU_PSEUDO_PERM, 0600);
  mu_expect_ok(ctx, "root chmod(0600)", ret);

  if (mu_set_effective(ctx, MU_UID1, MU_GID1) != 0)
    {
      goto out;
    }

  ret = chmod(MU_PSEUDO_PERM, 0777);
  mu_expect_denied(ctx, "non-owner chmod(0777)", ret);

  ret = chown(MU_PSEUDO_PERM, 0, 0);
  mu_expect_denied(ctx, "non-root chown(0,0)", ret);

  if (mu_restore_root(ctx) != 0)
    {
      goto out;
    }

  ret = chown(MU_PSEUDO_PERM, MU_UID1, MU_GID1);
  mu_expect_ok(ctx, "root chown to 1000:1000", ret);
  mu_verify_owner(ctx, MU_PSEUDO_PERM, MU_UID1, MU_GID1);

  if (mu_set_effective(ctx, MU_UID1, MU_GID1) != 0)
    {
      goto out;
    }

  ret = chmod(MU_PSEUDO_PERM, 0777);
  mu_expect_ok(ctx, "owner chmod(0777)", ret);

  if (mu_restore_root(ctx) != 0)
    {
      goto out;
    }

  fd = open(MU_PSEUDO_SECRET, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0)
    {
      mu_fail(ctx, "create %s errno=%d", MU_PSEUDO_SECRET, errno);
      goto out;
    }

  close(fd);
  mu_verify_owner(ctx, MU_PSEUDO_SECRET, 0, 0);

  if (mu_set_effective(ctx, MU_UID1, MU_GID1) != 0)
    {
      goto out;
    }

  multiuser_open_secret(ctx, MU_PSEUDO_SECRET, 0);

  if (mu_restore_root(ctx) != 0)
    {
      goto out;
    }

  multiuser_open_secret(ctx, MU_PSEUDO_SECRET, 1);

  if (mu_set_effective(ctx, MU_UID1, MU_GID1) != 0)
    {
      goto out;
    }

  fd = open(MU_PSEUDO_USER, O_CREAT | O_WRONLY, 0644);
  if (fd < 0)
    {
      mu_fail(ctx, "create %s as euid=%d errno=%d",
              MU_PSEUDO_USER, geteuid(), errno);
      goto out;
    }

  close(fd);
  mu_verify_owner(ctx, MU_PSEUDO_USER, MU_UID1, MU_GID1);

  if (mu_set_effective(ctx, MU_UID2, MU_GID2) != 0)
    {
      goto out;
    }

  multiuser_open_secret(ctx, MU_PSEUDO_USER, 1);

out:
  mu_restore_root(ctx);
  unlink(MU_PSEUDO_PERM);
  unlink(MU_PSEUDO_SECRET);
  unlink(MU_PSEUDO_USER);

  return ctx->failures;
}

#endif /* FS_PERMISSION && PSEUDOFS_ATTRIBUTES && PSEUDOFS_FILE */

#if defined(CONFIG_FS_PERMISSION) && defined(CONFIG_FS_TMPFS)

static int multiuser_tmpfs_test(FAR struct mu_ctx_s *ctx)
{
  int fd;

  printf("multiuser: tmpfs open permission enforcement\n");

  unlink(MU_TMPFS_SECRET);

  fd = open(MU_TMPFS_SECRET, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0)
    {
      mu_fail(ctx, "create %s errno=%d", MU_TMPFS_SECRET, errno);
      goto out;
    }

  close(fd);
  mu_verify_owner(ctx, MU_TMPFS_SECRET, 0, 0);

  if (mu_set_effective(ctx, MU_UID1, MU_GID1) != 0)
    {
      goto out;
    }

  multiuser_open_secret(ctx, MU_TMPFS_SECRET, 0);

  if (mu_restore_root(ctx) != 0)
    {
      goto out;
    }

  multiuser_open_secret(ctx, MU_TMPFS_SECRET, 1);

out:
  mu_restore_root(ctx);
  unlink(MU_TMPFS_SECRET);

  return ctx->failures;
}

#endif /* CONFIG_FS_PERMISSION && CONFIG_FS_TMPFS */

#if defined(CONFIG_LIBC_PASSWD_FILE)

static int multiuser_passwd_test(FAR struct mu_ctx_s *ctx)
{
  FAR struct passwd *pwd;

  printf("multiuser: passwd lookup after credential drop\n");

  if (mu_set_effective(ctx, MU_UID1, MU_GID1) != 0)
    {
      return ctx->failures;
    }

  pwd = getpwnam("root");
  if (pwd == NULL)
    {
      mu_fail(ctx, "getpwnam(root) failed errno=%d after seteuid(%d)",
              errno, MU_UID1);
    }
  else
    {
      mu_pass("getpwnam(root) uid=%u", (unsigned int)pwd->pw_uid);
    }

  pwd = getpwnam("testuser");
  if (pwd == NULL)
    {
      mu_fail(ctx, "getpwnam(testuser) failed errno=%d", errno);
    }
  else if (pwd->pw_uid != MU_UID1)
    {
      mu_fail(ctx, "getpwnam(testuser) uid=%u (expected %u)",
              (unsigned int)pwd->pw_uid, (unsigned int)MU_UID1);
    }
  else
    {
      mu_pass("getpwnam(testuser) uid=%u", (unsigned int)pwd->pw_uid);
    }

  mu_restore_root(ctx);
  return ctx->failures;
}

#endif /* CONFIG_LIBC_PASSWD_FILE */

#endif /* CONFIG_SCHED_USER_IDENTITY */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int multiuser_test(void)
{
  struct mu_ctx_s ctx =
  {
    0
  };

#if defined(CONFIG_SCHED_USER_IDENTITY)

  printf("multiuser_test: start\n");

  multiuser_effective_test(&ctx);

#if defined(CONFIG_SCHED_WAITPID) && !defined(CONFIG_BUILD_KERNEL)
  multiuser_suid_test(&ctx);
#else
  printf("multiuser: skipping saved set-UID/GID child test "
         "(need CONFIG_SCHED_WAITPID)\n");
#endif

#if defined(CONFIG_FS_PERMISSION) && defined(CONFIG_PSEUDOFS_ATTRIBUTES) && \
    defined(CONFIG_PSEUDOFS_FILE)
  multiuser_pseudofs_test(&ctx);
#else
  printf("multiuser: skipping pseudoFS permission tests "
         "(need FS_PERMISSION, PSEUDOFS_ATTRIBUTES, PSEUDOFS_FILE)\n");
#endif

#if defined(CONFIG_FS_PERMISSION) && defined(CONFIG_FS_TMPFS)
  multiuser_tmpfs_test(&ctx);
#else
  printf("multiuser: skipping tmpfs permission tests "
         "(need FS_PERMISSION and FS_TMPFS)\n");
#endif

#if defined(CONFIG_LIBC_PASSWD_FILE)
  multiuser_passwd_test(&ctx);
#else
  printf("multiuser: skipping passwd lookup test "
         "(need LIBC_PASSWD_FILE)\n");
#endif

  mu_restore_root(&ctx);

  printf("multiuser_test: %d failure(s)\n", ctx.failures);

#else

  printf("multiuser_test: CONFIG_SCHED_USER_IDENTITY disabled\n");

#endif

  return ctx.failures;
}
