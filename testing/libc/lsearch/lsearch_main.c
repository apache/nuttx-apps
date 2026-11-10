/****************************************************************************
 * apps/testing/libc/lsearch/lsearch_main.c
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

#include <search.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The width of a table element: not the size of a basic type */

#define NAME_WIDTH 6

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* An element of a queue: the forward and the backward pointer first */

struct qelem_s
{
  FAR struct qelem_s *forw;
  FAR struct qelem_s *back;
  int value;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* The key starts with '=' and the elements do not: a match also means that
 * lfind() passed the key as the first argument, as POSIX requires.
 */

static int compare_key(FAR const void *key, FAR const void *elem)
{
  FAR const char *k = key;

  if (k[0] != '=')
    {
      return -1;
    }

  return strncmp(k + 1, elem, NAME_WIDTH - 1);
}

static int compare_name(FAR const void *key, FAR const void *elem)
{
  return strncmp(key, elem, NAME_WIDTH);
}

static int check(FAR const char *name, bool ok)
{
  printf("%s: %s\n", name, ok ? "OK" : "ERROR");
  return ok ? 0 : 1;
}

static int test_lfind(void)
{
  char table[4][NAME_WIDTH] =
  {
    "cat", "dog", "emu", "dog"
  };

  char key[NAME_WIDTH] = "=dog";
  FAR char *p;
  size_t nel = 4;
  int nerrors = 0;

  p = lfind(key, table, &nel, NAME_WIDTH, compare_key);
  nerrors += check("lfind() returns the first match",
                   p == table[1] && nel == 4);

  strcpy(key, "=yak");
  p = lfind(key, table, &nel, NAME_WIDTH, compare_key);
  nerrors += check("lfind() returns NULL for a missing key",
                   p == NULL && nel == 4);

  strcpy(key, "=cat");
  nel = 0;
  p = lfind(key, table, &nel, NAME_WIDTH, compare_key);
  nerrors += check("lfind() finds nothing in an empty table",
                   p == NULL && nel == 0);

  return nerrors;
}

static int test_lsearch(void)
{
  char table[4][NAME_WIDTH] =
  {
    "cat", "dog"
  };

  char key[NAME_WIDTH] = "dog";
  FAR char *p;
  size_t nel = 2;
  int nerrors = 0;

  p = lsearch(key, table, &nel, NAME_WIDTH, compare_name);
  nerrors += check("lsearch() finds a present key and adds nothing",
                   p == table[1] && nel == 2);

  /* A missing key is copied to the end of the table, all of its bytes */

  memcpy(key, "emu\0xy", NAME_WIDTH);
  p = lsearch(key, table, &nel, NAME_WIDTH, compare_name);
  nerrors += check("lsearch() adds a missing key at the end",
                   p == table[2] && nel == 3 &&
                   memcmp(table[2], key, NAME_WIDTH) == 0);

  p = lsearch(key, table, &nel, NAME_WIDTH, compare_name);
  nerrors += check("lsearch() then finds the key it added",
                   p == table[2] && nel == 3);

  nel = 0;
  p = lsearch(key, table, &nel, NAME_WIDTH, compare_name);
  nerrors += check("lsearch() adds to an empty table",
                   p == table[0] && nel == 1 &&
                   memcmp(table[0], key, NAME_WIDTH) == 0);

  return nerrors;
}

static int test_linear_queue(void)
{
  struct qelem_s a;
  struct qelem_s b;
  struct qelem_s c;
  struct qelem_s d;
  int nerrors = 0;

  memset(&a, 0xff, sizeof(a));
  insque(&a, NULL);
  nerrors += check("insque(e, NULL) starts a linear queue",
                   a.forw == NULL && a.back == NULL);

  /* Insert after the last element, then between two elements */

  insque(&c, &a);
  insque(&b, &a);
  nerrors += check("insque() links after an element",
                   a.forw == &b && b.back == &a && b.forw == &c &&
                   c.back == &b && c.forw == NULL && a.back == NULL);

  insque(&d, &c);
  nerrors += check("insque() links after the last element",
                   c.forw == &d && d.back == &c && d.forw == NULL);

  /* a b c d: remove the middle, the last and then the first element */

  remque(&b);
  nerrors += check("remque() unlinks a middle element",
                   a.forw == &c && c.back == &a);

  remque(&d);
  nerrors += check("remque() unlinks the last element", c.forw == NULL);

  remque(&a);
  nerrors += check("remque() unlinks the first element",
                   c.back == NULL && c.forw == NULL);

  return nerrors;
}

static int test_circular_queue(void)
{
  struct qelem_s h;
  struct qelem_s a;
  struct qelem_s b;
  int nerrors = 0;

  /* The head of a circular queue points to itself */

  h.forw = &h;
  h.back = &h;

  insque(&a, &h);
  nerrors += check("insque() into an empty circular queue",
                   h.forw == &a && h.back == &a &&
                   a.forw == &h && a.back == &h);

  insque(&b, &a);
  nerrors += check("insque() closes the circle",
                   h.forw == &a && a.forw == &b && b.forw == &h &&
                   h.back == &b && b.back == &a && a.back == &h);

  remque(&a);
  nerrors += check("remque() from a circular queue",
                   h.forw == &b && b.back == &h &&
                   b.forw == &h && h.back == &b);

  remque(&b);
  nerrors += check("remque() of the last element leaves the head",
                   h.forw == &h && h.back == &h);

  return nerrors;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int nerrors = 0;

  nerrors += test_lfind();
  nerrors += test_lsearch();
  nerrors += test_linear_queue();
  nerrors += test_circular_queue();

  printf("lsearch test: %d errors\n", nerrors);
  return nerrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
