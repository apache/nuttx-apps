/****************************************************************************
 * apps/examples/nxflat/tests/tlsxx/tlsxx.cxx
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The constructor of a thread_local object runs in each thread, at the
 * first use of the object in that thread.
 */

class CCounter
{
public:
  CCounter(void);

  const char *m_name;
  int m_value;
};

/* The static thread_local member of a class template.  The emutls control
 * objects of the member and of its guard variable are in COMDAT groups.
 */

template <typename T> class CHolder
{
public:
  CHolder(void) : m_value(42)
  {
  }

  static thread_local CHolder<T> s_holder;
  T m_value;
};

template <typename T> thread_local CHolder<T> CHolder<T>::s_holder;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_nctors;
static int g_errors;
static int *g_childaddr;
static thread_local CCounter g_counter;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

CCounter::CCounter(void)
{
  m_name  = "tlsxx-counter";
  m_value = 17;
  g_nctors++;
}

static void check(const char *who, const char *what, bool ok)
{
  printf("%s: %-34s %s\n", who, what, ok ? "PASS" : "FAIL");
  if (!ok)
    {
      g_errors++;
    }
}

static void check_initial(const char *who)
{
  printf("%s: name=%s value=%d\n", who, g_counter.m_name,
         g_counter.m_value);

  check(who, "constructor set the string",
        strcmp(g_counter.m_name, "tlsxx-counter") == 0);
  check(who, "constructor set the value", g_counter.m_value == 17);
  check(who, "template member initialized",
        CHolder<int>::s_holder.m_value == 42);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* A function that is used through a pointer must have global scope:  see
 * the pthread test.
 */

extern "C" void *child_start_routine(void *arg)
{
  check_initial("CHILD");

  g_childaddr       = &g_counter.m_value;
  g_counter.m_value = 2;
  CHolder<int>::s_holder.m_value = 2;
  return NULL;
}

extern "C" int main(int argc, char **argv)
{
  pthread_t thread;
  int status;

  check_initial("MAIN");
  g_counter.m_value = 1;
  CHolder<int>::s_holder.m_value = 1;

  status = pthread_create(&thread, NULL, child_start_routine, NULL);
  if (status != 0)
    {
      printf("MAIN: pthread_create() returned %d\n", status);
      return EXIT_FAILURE;
    }

  pthread_join(thread, NULL);

  check("MAIN", "value kept after the child ran", g_counter.m_value == 1);
  check("MAIN", "template member kept",
        CHolder<int>::s_holder.m_value == 1);
  check("MAIN", "child has its own object",
        g_childaddr != &g_counter.m_value);
  check("MAIN", "one constructor call per thread", g_nctors == 2);

  if (g_errors != 0)
    {
      printf("TLS C++ test FAILED: %d error(s)\n", g_errors);
      return EXIT_FAILURE;
    }

  printf("TLS C++ test PASSED\n");
  return EXIT_SUCCESS;
}
