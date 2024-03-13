/****************************************************************************
 * apps/testing/kasantest/kasantest.c
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
#include <nuttx/mm/kasan.h>

#include <stdio.h>
#include <syslog.h>
#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define KASAN_TEST_MEM_SIZE 128

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_kasan_test_buffer[KASAN_TEST_MEM_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void kasan_test(char *p, size_t size)
{
  size_t i;

  for (i = 0; i < size + 64; i++)
    {
      syslog(LOG_SYSLOG,
             "Access Buffer[%zu] : %zu address: %p", size, i, &p[i]);
      p[i]++;
      syslog(LOG_SYSLOG, "read: %02x -- Successful\n", p[i]);
    }
}

static void kasan_test_use_after_free(void)
{
  char *ptr = malloc(KASAN_TEST_MEM_SIZE);

  if (ptr == NULL)
    {
      syslog(LOG_SYSLOG, "Failed to allocate memory\n");
      return;
    }

  syslog(LOG_SYSLOG, "KASan test use after free\n");
  strcpy(ptr, "kasan test use after free");
  free(ptr);
  printf("%s\n", ptr);
}

static void kasan_test_heap_memory_out_of_bounds(char *str)
{
  char  *endptr;
  size_t size;
  char  *ptr;

  size = strtoul(str, &endptr, 0);
  if (*endptr != '\0')
    {
      printf("Conversion failed: Not a valid integer.\n");
      return;
    }

  ptr = zalloc(size);
  if (ptr == NULL)
    {
      syslog(LOG_SYSLOG, "Failed to allocate memory\n");
      return;
    }

  syslog(LOG_SYSLOG,
         "KASan test accessing heap memory out of bounds completed\n");
  kasan_test(ptr, size);
}

static void kasan_test_global_variable_out_of_bounds(void)
{
  syslog(LOG_SYSLOG,
         "KASan test accessing global variable out of bounds\n");
  kasan_test(g_kasan_test_buffer, KASAN_TEST_MEM_SIZE);
}

static void *mm_stampede_thread(void *arg)
{
  char *p = (char *)arg;

  syslog(LOG_SYSLOG, "Child thread is running");
  kasan_test(p, KASAN_TEST_MEM_SIZE);
  pthread_exit(NULL);
}

static void kasan_test_memory_stampede(void)
{
  pthread_t thread;
  char array[KASAN_TEST_MEM_SIZE];

  syslog(LOG_SYSLOG, "KASan test accessing memory stampede\n");
  pthread_create(&thread, NULL, mm_stampede_thread, kasan_reset_tag(&array));
  pthread_join(thread, NULL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: kasantest_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  /* NutttX cannot check the secondary release
   * because the mm module has closed kasan instrumentation
   */

  if (argc < 2)
    {
      printf("Usage: %s <test_option>\n", argv[0]);
      printf("Available test options:\n");
      printf("  -u       : Test use after free\n");
      printf("  -h <arg> : Test heap memory out of bounds (provide size)\n");
      printf("  -g       : Test global variable out of bounds\n");
      printf("  -s       : Test memory stampede\n");
      return 0;
    }
  else if (strncmp(argv[1], "-u", 2) == 0)
    {
      kasan_test_use_after_free();
    }
  else if (strncmp(argv[1], "-h", 2) == 0 && argc == 3)
    {
      kasan_test_heap_memory_out_of_bounds(argv[2]);
    }
  else if (strncmp(argv[1], "-g", 2) == 0)
    {
      kasan_test_global_variable_out_of_bounds();
    }
  else if (strncmp(argv[1], "-s", 2) == 0)
    {
      kasan_test_memory_stampede();
    }
  else
    {
      printf("Unknown test option: %s\n", argv[1]);
    }

  syslog(LOG_SYSLOG, "KASan test failed, please check\n");
  return 0;
}
