/****************************************************************************
 * netutils/thttpd/thttpd_alloc.c
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

#include <sys/types.h>
#include <stdlib.h>
#include <malloc.h>
#include <debug.h>
#include <errno.h>

#include "config.h"
#include "thttpd_alloc.h"

#ifdef CONFIG_THTTPD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_THTTPD_MEMDEBUG
static int    g_nallocations = 0;
static int    g_nfreed       = 0;
static size_t g_allocated    = 0;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Generate debugging statistics */

#ifdef CONFIG_THTTPD_MEMDEBUG
void httpd_memstats(void)
{
  static struct mallinfo mm;

  ninfo("%d allocations (%lu bytes), %d freed\n",
       g_nallocations, (unsigned long)g_allocated, g_nfreed);

  /* Get the current memory usage */

  mm = mallinfo();

  ninfo("arena: %08x ordblks: %08x mxordblk: %08x uordblks: %08x "
        "fordblks: %08x\n",
       mm.arena, mm.ordblks, mm.mxordblk, mm.uordblks, mm.fordblks);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_THTTPD_MEMDEBUG
FAR void *httpd_malloc(size_t nbytes)
{
  void *ptr = malloc(nbytes);
  if (!ptr)
    {
      nerr("ERROR: Allocation of %d bytes failed\n", nbytes);
    }
  else
    {
      ninfo("Allocated %d bytes at %p\n", nbytes, ptr);
      g_nallocations++;
      g_allocated += nbytes;
    }

  httpd_memstats();
  return ptr;
}
#endif

#ifdef CONFIG_THTTPD_MEMDEBUG
FAR void *httpd_realloc(FAR void *oldptr, size_t oldsize, size_t newsize)
{
  void *ptr = realloc(oldptr, newsize);
  if (!ptr)
    {
      nerr("ERROR: Re-allocation from %d to %d bytes failed\n",
           oldsize, newsize);
    }
  else
    {
      ninfo("Re-allocated form %d to %d bytes (from %p to %p)\n",
            oldsize, newsize, oldptr, ptr);
      g_allocated += (newsize - oldsize);
    }

  httpd_memstats();
  return ptr;
}
#endif

#ifdef CONFIG_THTTPD_MEMDEBUG
void httpd_free(FAR void *ptr)
{
  free(ptr);
  g_nfreed++;
  ninfo("Freed memory at %p\n", ptr);
  httpd_memstats();
}
#endif

#ifdef CONFIG_THTTPD_MEMDEBUG
FAR char *httpd_strdup(const char *str)
{
  FAR char *newstr = strdup(str);
  if (!newstr)
    {
      nerr("ERROR: strdup of %s failed\n", str);
    }
  else
    {
      ninfo("strdup'ed %s\n", str);
      g_nallocations++;
      g_allocated += (strlen(str)+1);
    }

  httpd_memstats();
  return newstr;
}
#endif

/* Helpers to implement dynamically allocated strings */

void httpd_realloc_str(char **pstr, size_t *maxsize, size_t size)
{
  size_t oldsize;
  if (*maxsize == 0)
    {
      *maxsize = MAX(CONFIG_THTTPD_MINSTRSIZE,
                     size + CONFIG_THTTPD_REALLOCINCR);
      *pstr    = NEW(char, *maxsize + 1);
    }
  else if (size > *maxsize)
    {
      oldsize  = *maxsize;
      *maxsize = MAX(oldsize * 2, size * 5 / 4);
      *pstr    = httpd_realloc(*pstr, oldsize + 1, *maxsize + 1);
    }
  else
    {
      return;
    }

  if (!*pstr)
    {
      nerr("ERROR: out of memory reallocating a string to %d bytes\n",
           *maxsize);
      exit(1);
    }
}

#endif /* CONFIG_THTTPD */
