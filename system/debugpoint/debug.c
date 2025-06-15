/****************************************************************************
 * apps/system/debugpoint/debug.c
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
#include <stdbool.h>

#include <nuttx/gdbstub.h>
#include <nuttx/compiler.h>
#include <syslog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_test_data[8];
static const char g_test_rodata[32] = "This is a read-only string";

/****************************************************************************
 * Name: debug_option
 ****************************************************************************/

struct debug_option
{
  int type;
  int width;
  bool cancel;
  FAR void *addr;
};

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
 * Name: debug_callback_cmd
 ****************************************************************************/

static void debug_callback_cmd(int type, FAR void *addr, size_t size,
                               FAR void *arg)
{
  /* Using syslog here since called from an interrupt handler */

  syslog(LOG_NOTICE, "debug_callback_cmd: type=%d addr=%p size=%d\n", type,
         addr, (int)size);
  gdb_debugpoint_remove(type, addr, size);
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
 * Name: parse_options
 *
 * Description:
 *   Parses command line options to set debug points (watchpoints or
 *   breakpoints) or to cancel them. The function supports setting read,
 *   write, read/write watchpoints, and breakpoints at specified addresses,
 *   as well as canceling any previously set debug points.
 *
 * Parameters:
 *   argc - The number of arguments passed to the program.
 *   argv - An array of argument strings.
 *   options - A pointer to a struct debug_option where the parsed options
 *             will be stored.
 *
 * Returns:
 *   true if the options were parsed successfully, false otherwise.
 *
 ****************************************************************************/

static bool parse_options(int argc, FAR char *argv[],
                          struct debug_option *opt)
{
  int cmd;
  while ((cmd = getopt(argc, argv, "r:w:b:x:cl:")) != -1)
    {
      switch (cmd)
        {
          case 'r':

            /* Set a read watchpoint at the specified address */

            opt->type = GDB_STOPREASON_WATCHPOINT_RO;
            opt->addr = (FAR void *)(uintptr_t)strtoul(optarg, NULL, 0);
            break;
          case 'w':

            /* Set a write watchpoint at the specified address */

            opt->type = GDB_STOPREASON_WATCHPOINT_WO;
            opt->addr = (FAR void *)(uintptr_t)strtoul(optarg, NULL, 0);
            break;
          case 'b':

            /* Set a breakpoint at the specified address */

            opt->type = GDB_STOPREASON_BREAKPOINT;
            opt->addr = (FAR void *)(uintptr_t)strtoul(optarg, NULL, 0);
            break;
          case 'x':

            /* Set a read/write watchpoint at the specified address */

            opt->type = GDB_STOPREASON_WATCHPOINT_RW;
            opt->addr = (FAR void *)(uintptr_t)strtoul(optarg, NULL, 0);
            break;
          case 'c':

            /* Cancel the watchpoint or breakpoint */

            opt->cancel = true;
            break;
          case 'l':

            /* Set the watch length to the specified address */

            opt->width = strtoul(optarg, NULL, 0);
            break;
          default:

            /* Print usage information */

            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -r addr  Set a read watchpoint at address\n");
            printf("  -w addr  Set a write watchpoint at address\n");
            printf("  -b addr  Set a breakpoint at address\n");
            printf("  -x addr  Set a read/write watchpoint at address\n");
            printf("  -c       Cancel the watchpoint or breakpoint\n");
            printf("  -l len   Set the watch length\n");
            return false;
        }
    }

    if (opt->width == 0)
      {
        /* If the watch length is not specified,
         * default to 4 bytes for watchpoints
         */

        if (opt->type == GDB_STOPREASON_WATCHPOINT_RO ||
            opt->type == GDB_STOPREASON_WATCHPOINT_WO ||
            opt->type == GDB_STOPREASON_WATCHPOINT_RW)
          {
            opt->width = 4;
          }
      }

    return true;
}

/****************************************************************************
 * Name: handle_cmd
 *
 * Description:
 *   Handles command line input to set or cancel debug points
 *   (watchpoints or breakpoints).
 *   This function parses the command line options using parse_options,
 *   and then either sets a new debug point or cancels an existing one based
 *   on the parsed options.
 *
 * Parameters:
 *   argc - The number of arguments passed to the program.
 *   argv - An array of argument strings.
 *
 * Returns:
 *   0 on success, or a negative error code on failure.
 *
 ****************************************************************************/

static int handle_cmd(int argc, FAR char *argv[])
{
  int ret;
  struct debug_option opt;

  /* Initialize the debug options structure with default values */

  opt.type = GDB_STOPREASON_NONE;
  opt.addr = NULL;
  opt.cancel = false;
  opt.width = 0;

  if (parse_options(argc, argv, &opt))
    {
      /* Check cancel option */

      if (opt.cancel && opt.type != GDB_STOPREASON_NONE && opt.addr)
        {
          /* Cancel debug point */

          ret = gdb_debugpoint_remove(opt.type, opt.addr, opt.width);
          if (ret < 0)
            {
              printf("Failed to remove debug point. Error code: %d\n", ret);
              return ret;
            }
          else
            {
              printf("Debug point successfully removed.\n");
            }
        }
      else if (opt.type != GDB_STOPREASON_NONE && opt.addr)
        {
          /* Add a new debug point */

          ret = gdb_debugpoint_add(opt.type, opt.addr, opt.width,
                                   debug_callback_cmd, NULL);
          if (ret < 0)
            {
              printf("Failed to add debug point. Error code: %d\n", ret);
              return ret;
            }
          else
            {
              printf("Debug point successfully added at address: %p\n",
                     opt.addr);
            }
        }
    }
  else
    {
      return -EINVAL;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Check is there command line options */

  if (argc > 1)
    {
      return handle_cmd(argc, argv);
    }

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
