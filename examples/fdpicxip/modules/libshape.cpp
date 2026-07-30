/****************************************************************************
 * apps/examples/fdpicxip/modules/libshape.cpp
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

/* A C++ shared library, executed in place out of flash.
 *
 * The point of this file is g_registry.  It is a global object with a real
 * constructor, and the constructor is the only thing that ever writes
 * m_magic.  If the loader does not walk DT_INIT_ARRAY, the object simply
 * stays in .bss and every field reads back zero -- the library still loads,
 * still links, and still runs, and quietly answers wrong.  That is why the
 * demo checks for a magic rather than just printing a total: the failure
 * this is here to catch does not announce itself.
 *
 * m_total is the second half: it lives in the library's writable segment,
 * which is copied once per running instance, so two tasks sharing this
 * library's flash-resident code still count independently.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define SHAPE_MAGIC 0x5ba9e0

class Registry
{
public:
  Registry() : m_magic(SHAPE_MAGIC), m_total(0), m_adds(0)
  {
    m_marker[0] = '\0';
    syslog(LOG_INFO, "  [libshape] Registry() ran\n");
  }

  /* Runs last of the two objects, because destruction mirrors construction
   * and this library was constructed first.  So a marker written here is
   * evidence that the whole DT_FINI_ARRAY chain ran, not just the module's
   * half -- which is why the marker is written from the library rather than
   * from the module that knows the path.
   */

  ~Registry()
  {
    syslog(LOG_INFO, "  [libshape] ~Registry() ran after %d adds\n", m_adds);

    if (m_marker[0] != '\0')
      {
        char text[16];
        int  len;
        int  fd;

        len = snprintf(text, sizeof(text), "%d", m_total);

        fd = open(m_marker, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0)
          {
            /* xipfs wants the size declared up front so it can reserve one
             * contiguous extent.
             */

            if (ftruncate(fd, len) >= 0)
              {
                write(fd, text, len);
              }

            close(fd);
          }
        else
          {
            syslog(LOG_INFO, "  [libshape] marker %s failed\n", m_marker);
          }
      }
  }

  /* Where ~Registry() should record its final total.  Optional: the demo
   * does not set one and simply logs instead.
   */

  void marker(const char *path)
  {
    strlcpy(m_marker, path, sizeof(m_marker));
  }

  bool constructed() const
  {
    return m_magic == SHAPE_MAGIC;
  }

  int add(int by)
  {
    m_adds++;
    m_total += by;
    return m_total;
  }

  int total() const
  {
    return m_total;
  }

private:
  int  m_magic;
  int  m_total;
  int  m_adds;
  char m_marker[64];
};

static Registry g_registry;

/* C linkage, because the module imports these by name and the loader
 * matches dynamic symbols as plain strings.
 */

extern "C"
{
  int  shape_add(int by);
  int  shape_total(void);
  bool shape_constructed(void);
  void shape_marker(const char *path);
}

int shape_add(int by)
{
  return g_registry.add(by);
}

void shape_marker(const char *path)
{
  g_registry.marker(path);
}

int shape_total(void)
{
  return g_registry.total();
}

bool shape_constructed(void)
{
  return g_registry.constructed();
}
