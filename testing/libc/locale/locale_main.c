/****************************************************************************
 * apps/testing/libc/locale/locale_main.c
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

#include <sys/param.h>
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_LIBC_LOCALE_GETTEXT
#  include <libintl.h>
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct locale_name_s
{
  FAR const char *name;
  bool valid;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* "" is the C locale when LANG and LC_* are not set.  The C locale uses
 * UTF-8, so "C.UTF-8" and "C.utf8" are valid names too.
 */

static const struct locale_name_s g_names[] =
{
  { "C",           true  },
  { "POSIX",       true  },
  { "",            true  },
  { "C.UTF-8",     true  },
  { "C.utf8",      true  },
  { "xx_XX.UTF-8", false },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* The functions return the number of errors */

static int check(FAR const char *func, FAR const char *name, bool ok)
{
  if (!ok)
    {
      printf("ERROR: %s(\"%s\")\n", func, name);
      return 1;
    }

  printf("%s(\"%s\"): OK\n", func, name);
  return 0;
}

static int test_setlocale(FAR const struct locale_name_s *entry)
{
  char saved[64];
  FAR char *ret;
  bool ok;

  ret = setlocale(LC_ALL, entry->name);
  ok = (ret != NULL) == entry->valid;
  if (ok && ret != NULL)
    {
      /* The returned name must select the same locale again */

      strlcpy(saved, ret, sizeof(saved));
      ok = setlocale(LC_ALL, saved) != NULL;
    }

  setlocale(LC_ALL, "C");
  return check("setlocale", entry->name, ok);
}

static int test_newlocale(FAR const struct locale_name_s *entry)
{
  locale_t loc;

  loc = newlocale(LC_ALL_MASK, entry->name, NULL);
  if (loc != NULL)
    {
      freelocale(loc);
    }

  return check("newlocale", entry->name, (loc != NULL) == entry->valid);
}

static int test_codeset(void)
{
  bool ok;

  ok = setlocale(LC_CTYPE, "C.UTF-8") != NULL &&
       strcmp(nl_langinfo(CODESET), "UTF-8") == 0;
  setlocale(LC_ALL, "C");
  return check("nl_langinfo(CODESET) after setlocale", "C.UTF-8", ok);
}

#ifdef CONFIG_LIBC_LOCALE_GETTEXT
/* A program calls gettext() for the text of an error message after the
 * failed call and before it reads errno.  So these functions must keep
 * errno, also when there is no message catalog.
 */

static int check_gettext(FAR const char *func, FAR const char *text,
                         FAR const char *expect, int errcode)
{
  if (errcode != EPERM || strcmp(text, expect) != 0)
    {
      printf("ERROR: %s: errno %d (expected %d) text \"%s\"\n",
             func, errcode, EPERM, text);
      return 1;
    }

  printf("%s: OK\n", func);
  return 0;
}

static int test_gettext(void)
{
  FAR const char *text;
  int nerrors = 0;

  errno = EPERM;
  text = gettext("message");
  nerrors += check_gettext("gettext", text, "message", errno);

  errno = EPERM;
  text = dgettext("locale_test", "message");
  nerrors += check_gettext("dgettext", text, "message", errno);

  errno = EPERM;
  text = dcgettext("locale_test", "message", LC_MESSAGES);
  nerrors += check_gettext("dcgettext", text, "message", errno);

  errno = EPERM;
  text = ngettext("one file", "files", 2);
  nerrors += check_gettext("ngettext", text, "files", errno);

  errno = EPERM;
  text = dngettext("locale_test", "one file", "files", 2);
  nerrors += check_gettext("dngettext", text, "files", errno);

  errno = EPERM;
  text = dcngettext("locale_test", "one file", "files", 1, LC_MESSAGES);
  nerrors += check_gettext("dcngettext", text, "one file", errno);
  return nerrors;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int nerrors = 0;
  size_t i;

  printf("locale test: locale names and errno\n");

  for (i = 0; i < nitems(g_names); i++)
    {
      nerrors += test_setlocale(&g_names[i]);
      nerrors += test_newlocale(&g_names[i]);
    }

  nerrors += test_codeset();

#ifdef CONFIG_LIBC_LOCALE_GETTEXT
  nerrors += test_gettext();
#endif

  printf("locale test: %d errors\n", nerrors);
  return nerrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
