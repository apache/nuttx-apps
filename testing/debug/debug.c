/****************************************************************************
 * apps/testing/debug/debug.c
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
#include <stdbool.h>

#include <nuttx/gdbstub.h>
#include <nuttx/compiler.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_test_data[8];
static const char g_test_rodata[] = "This is a read-only string";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trigger_func1
 ****************************************************************************/

static void test_trigger(void)
{
  printf("Calling test_trigger\n");
}

/****************************************************************************
 * Name: debug_callback
 ****************************************************************************/

static void debug_callback(int type, FAR void *addr, size_t size,
                           FAR void *arg)
{
  *(bool *)arg = true;
  gdb_debugpoint_remove(type, addr, size);
}

/****************************************************************************
 * Name: test_function
 ****************************************************************************/

nooptimiziation_function static void test_function(void (*func)(void))
{
  int ret = 0;
  bool triggered;
  printf("==== Calling test_function ====\n");

  printf("Add breakpoint at %p\n", func);
  ret = gdb_debugpoint_add(GDB_STOPREASON_BREAKPOINT, func, 0,
                          debug_callback, &triggered);

  if (ret != OK)
    {
      printf("ERROR: Registering breakpoint at %p failed with %d\n",
             func, ret);
    }

  /* Trigger the breakpoint */

  triggered = false;
  func();

  if (triggered)
    {
      printf("Pass: Breakpoint at %p triggered\n", func);
    }
  else
    {
      gdb_debugpoint_remove(GDB_STOPREASON_BREAKPOINT, func, 0);
      printf("ERROR: Breakpoint %p not triggered\n", func);
    }
}

/****************************************************************************
 * Name: test_data
 ****************************************************************************/

nooptimiziation_function static void test_data(uint8_t *addr,
                                               size_t size, bool r, bool w)
{
  int ret = 0;
  int tmp;
  int test_type;
  bool triggered;

  printf("==== Calling test_data ====\n");

  if (r && w)
    {
      test_type = GDB_STOPREASON_WATCHPOINT_RW;
      printf("Add read/write watchpoint at %p\n", addr);
    }
  else if (r)
    {
      test_type = GDB_STOPREASON_WATCHPOINT_RO;
      printf("Add read watchpoint at %p\n", addr);
    }
  else if (w)
    {
      test_type = GDB_STOPREASON_WATCHPOINT_WO;
      printf("Add write watchpoint at %p\n", addr);
    }
  else
    {
      printf("ERROR: Invalid test type\n");
      return;
    }

  ret = gdb_debugpoint_add(test_type, addr, size,
                          debug_callback, &triggered);

  if (ret != OK)
    {
      printf("ERROR: Registering read watchpoint at %p failed with %d\n",
             addr, ret);
    }

  if (r & w)
    {
      /* Trigger the watchpoint by reading the address */

      triggered = false;
      if (addr[0] == 0x55)
        {
          /* Do something to avoid compiler removing the read */

          tmp = 0xaa;
        }
      else
        {
          tmp = 0x55;
        }

      if (triggered)
        {
          printf("Pass: Read watchpoint at %p triggered\n", addr);
        }
      else
        {
          gdb_debugpoint_remove(test_type, addr, size);
          printf("ERROR: Read watchpoint at %p not triggered\n", addr);
        }

      /* Register the watchpoint again to test write watchpoints */

      ret = gdb_debugpoint_add(test_type, addr, size,
                              debug_callback, &triggered);

      if (ret != OK)
        {
          printf("ERROR: Registering read/write watchpoint at %p"
                      " failed with %d\n", addr, ret);
        }

      /* Trigger the watchpoint by writing to the address */

      triggered = false;
      addr[0] = tmp;

      if (triggered)
        {
          printf("Pass: Write watchpoint at %p triggered\n", addr);
        }
      else
        {
          gdb_debugpoint_remove(test_type, addr, size);
          printf("ERROR: Write watchpoint at %p not triggered\n", addr);
        }
    }
  else if (r)
    {
      /* Trigger the watchpoint by reading the address */

      triggered = false;
      if (addr[0] == 0x55)
        {
          /* Do something to avoid compiler removing the read */

          printf("Reading %p\n", addr);
        }

      if (triggered)
        {
          printf("Pass: Read watchpoint at %p triggered\n", addr);
        }
      else
        {
          gdb_debugpoint_remove(test_type, addr, size);
          printf("ERROR: Read watchpoint at %p not triggered\n", addr);
        }
    }
  else if (w)
    {
      /** Trigger the watchpoint by writing to the address */

      triggered = false;
      addr[0] = 0x55;

      if (triggered)
        {
          printf("Pass: Write watchpoint at %p triggered\n", addr);
        }
      else
        {
          gdb_debugpoint_remove(test_type, addr, size);
          printf("ERROR: Write watchpoint at %p not triggered\n", addr);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  printf("Testing breakpoints\n");

  /* Test breakpoint at a function */

  test_function(test_trigger);

  /* Test read watchpoint for rodata */

  test_data((uint8_t *)&g_test_rodata, sizeof(g_test_rodata), true, false);

  /* Test read watchpoint for data */

  test_data((uint8_t *)&g_test_data, sizeof(g_test_data), true, false);

  /* Test write watchpoint for data */

  test_data((uint8_t *)&g_test_data, sizeof(g_test_data), false, true);

  /* Test read/write watchpoint for data */

  test_data((uint8_t *)&g_test_data, sizeof(g_test_data), true, true);

  return 0;
}
