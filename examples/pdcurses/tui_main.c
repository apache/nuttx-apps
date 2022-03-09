/****************************************************************************
 * apps/examples/pdcurses/tui.c
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
 *   Acknowledgement: some ideas were borrowed from Mark Hessling's
 *                    version of the 'testcurs' program.
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "tui.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Change this if source at other location */

#define FNAME  "../demos/tui.c"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void address(void);
static void sub0(void);
static void sub1(void);
static void sub2(void);
static void sub3(void);
static void func1(void);
static void func2(void);
static void subfunc1(void);
static void subfunc2(void);
static void subsub(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_fieldname[6] =
{
  "Name", "Street", "City", "State", "Country", (char *)0
};

menu g_mainmenu[] =
{
  {"Asub", sub0, "Go inside first submenu"},
  {"Bsub", sub1, "Go inside second submenu"},
  {"Csub", sub2, "Go inside third submenu"},
  {"Dsub", sub3, "Go inside fourth submenu"},
  {"", (FUNC)0, ""}             /* always add this as the last item! */
};

static const menu g_submenu0[] =
{
  {"Exit", tui_exit, "Terminate program"},
  {"", (FUNC)0, ""}
};

static const menu g_submenu1[] =
{
  {"OneBeep", func1, "Sound one beep"},
  {"TwoBeeps", func2, "Sound two beeps"},
  {"", (FUNC)0, ""}
};

static const menu g_submenu2[] =
{
  {"Browse", subfunc1, "Source file lister"},
  {"Input", subfunc2, "Interactive file lister"},
  {"Address", address, "Get address data"},
  {"", (FUNC)0, ""}
};

static const menu g_submenu3[] =
{
  {"SubSub", subsub, "Go inside sub-submenu"},
  {"", (FUNC)0, ""}
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void address(void)
{
  char *fieldbuf[5];
  WINDOW *wbody = bodywin();
  int field = 50;
  int i;

  for (i = 0; i < 5; i++)
    {
      fieldbuf[i] = calloc(1, field + 1);
    }

  if (getstrings(g_fieldname, fieldbuf, field) != KEY_ESC)
    {
      for (i = 0; g_fieldname[i]; i++)
        {
          wprintw(wbody, "%10s : %s\n", g_fieldname[i], fieldbuf[i]);
        }

      wrefresh(wbody);
    }

  for (i = 0; i < 5; i++)
    {
      free(fieldbuf[i]);
    }
}

static char *getfname(char *desc, char *fname, int field)
{
  char *fieldname[2];
  char *fieldbuf[1];

  fieldname[0] = desc;
  fieldname[1] = 0;
  fieldbuf[0]  = fname;

  return (getstrings((const char **)fieldname, fieldbuf, field) == KEY_ESC) ? NULL : fname;
}

static void showfile(char *fname)
{
  FILE *fp;
  char buf[MAXSTRLEN];
  bool ateof = false;
  int bh = bodylen();
  int i;

  statusmsg("FileBrowser: Hit key to continue, Q to quit");

  if ((fp = fopen(fname, "r")) != NULL) /* file available? */
    {
      while (!ateof)
        {
          clsbody();

          for (i = 0; i < bh - 1 && !ateof; i++)
            {
              buf[0] = '\0';
              fgets(buf, MAXSTRLEN, fp);

              if (strlen(buf))
                {
                  bodymsg(buf);
                }
              else
                {
                  ateof = true;
                }
            }

          switch (waitforkey())
            {
            case 'Q':
            case 'q':
            case 0x1b:
              ateof = true;
              break;
            }
        }

      fclose(fp);
    }
  else
    {
      sprintf(buf, "ERROR: file '%s' not found", fname);
      errormsg(buf);
    }
}

static void sub0(void)
{
  domenu(g_submenu0);
}

static void sub1(void)
{
  domenu(g_submenu1);
}

static void sub2(void)
{
  domenu(g_submenu2);
}

static void sub3(void)
{
  domenu(g_submenu3);
}

static void func1(void)
{
  beep();
  bodymsg("One beep! ");
}

static void func2(void)
{
  beep();
  bodymsg("Two beeps! ");
  beep();
}

static void subfunc1(void)
{
  showfile(FNAME);
}

static void subfunc2(void)
{
  char fname[MAXSTRLEN];

  strcpy(fname, FNAME);
  if (getfname("File to browse:", fname, 50))
    {
      showfile(fname);
    }
}

static void subsub(void)
{
  domenu(g_submenu2);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_LIBC_LOCALE
  setlocale(LC_ALL, "");
#endif

  startmenu(g_mainmenu, "TUI - 'textual user interface' demonstration program");
  return 0;
}
