/****************************************************************************
 * apps/examples/fdpicxip/modules/cxxuser.cpp
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

/* A C++ module using a C++ shared library.
 *
 * Two instances of this run at once in the demo.  Between them they show
 * the three things that have to hold:
 *
 *   - constructors ran, in both objects.  Neither magic can be right by
 *     accident: an unconstructed global is zero.
 *   - the library's constructors ran before this module's, which is the
 *     ordering DT_NEEDED implies and the loader has to honour.
 *   - the library's data is private per instance.  One copy of its code
 *     sits in flash; the totals do not interleave.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

extern "C"
{
  int  shape_add(int by);
  int  shape_total(void);
  bool shape_constructed(void);
  void shape_marker(const char *path);
}

#define USER_MAGIC 0x0c1a55

class Instance
{
public:
  Instance() : m_magic(USER_MAGIC), m_libwasready(shape_constructed())
  {
  }

  bool constructed() const
  {
    return m_magic == USER_MAGIC;
  }

  /* Whether the library was already constructed when this object's own
   * constructor ran.  Sampling it here is the only way to observe the
   * ordering: by the time main() runs, both have been constructed either
   * way.
   */

  bool libwasready() const
  {
    return m_libwasready;
  }

private:
  int  m_magic;
  bool m_libwasready;
};

static Instance g_self;

/* Exit status is a bitmask of what failed, so one run reports on all four
 * properties independently.  Kept in step with the reader in
 * apps/testing/fs/xipfs/.
 */

#define USER_FAIL_OWN_CTOR  0x01
#define USER_FAIL_LIB_CTOR  0x02
#define USER_FAIL_ORDER     0x04
#define USER_FAIL_PRIVATE   0x08

/****************************************************************************
 * Name: record
 *
 * Description:
 *   Write a number to a file, for a caller that cannot rely on this task's
 *   exit status.
 *
 *   It cannot, in general.  waitpid() only retains the status of a child
 *   that has already exited when CONFIG_SCHED_CHILD_STATUS is set, and that
 *   depends on CONFIG_SCHED_HAVE_PARENT.  With two instances running for
 *   the same length of time, the second has always exited by the time the
 *   first waitpid() returns, so its status is simply gone.  A file survives
 *   the task, and survives the module being unloaded.
 *
 ****************************************************************************/

static void record(const char *path, int value)
{
  char text[16];
  int  len;
  int  fd;

  if (path == NULL)
    {
      return;
    }

  len = snprintf(text, sizeof(text), "%d", value);

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    {
      syslog(LOG_INFO, "[user] cannot record to %s\n", path);
      return;
    }

  /* xipfs wants the size declared up front so it can reserve one
   * contiguous extent.
   */

  if (ftruncate(fd, len) >= 0)
    {
      write(fd, text, len);
    }

  close(fd);
}

extern "C" int main(int argc, char *argv[])
{
  int seed = (argc > 1) ? atoi(argv[1]) : 1;
  int fails = 0;
  int i;

  /* Optional second argument: a path for the library's destructor to record
   * its final total in.  A destructor runs at unload, after this task is
   * gone, so leaving a file behind is the only way a test can observe that
   * it ran at all.  The demo omits it and reads the log instead.
   */

  if (argc > 2)
    {
      shape_marker(argv[2]);
    }

  syslog(LOG_INFO, "[user %d] own ctor ran: %s, library ctor ran: %s, "
         "library first: %s\n",
         seed,
         g_self.constructed()    ? "yes" : "NO",
         shape_constructed()     ? "yes" : "NO",
         g_self.libwasready()    ? "yes" : "NO");

  /* Every check runs and contributes a bit, rather than the first failure
   * returning early.  A caller that reports these individually can then say
   * which property broke instead of just that something did -- and none of
   * its assertions pass merely because an earlier one stopped the run.
   */

  if (!g_self.constructed())
    {
      fails |= USER_FAIL_OWN_CTOR;
    }

  if (!shape_constructed())
    {
      fails |= USER_FAIL_LIB_CTOR;
    }

  if (!g_self.libwasready())
    {
      fails |= USER_FAIL_ORDER;
    }

  for (i = 0; i < 3; i++)
    {
      shape_add(seed);
      usleep(100000);
    }

  if (shape_total() != seed * 3)
    {
      fails |= USER_FAIL_PRIVATE;
    }

  syslog(LOG_INFO, "[user %d] library total = %d (expected %d) -- %s\n",
         seed, shape_total(), seed * 3,
         fails == 0 ? "PASS" : "FAIL");

  /* Optional third argument: where to record the bitmask, for a caller that
   * cannot rely on the exit status surviving.  See record() above.
   */

  if (argc > 3)
    {
      record(argv[3], fails);
    }

  return fails;
}
