/****************************************************************************
 * apps/interpreters/bas/bas.h
 *
 *   Copyright (c) 1999-2014 Michael Haardt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef __APPS_EXAMPLES_BAS_BAS_H
#define __APPS_EXAMPLES_BAS_BAS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STDCHANNEL 0
#define LPCHANNEL 32

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int g_bas_argc;
extern char *g_bas_argv0;
extern char **g_bas_argv;
extern bool g_bas_end;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void bas_init(int backslash_colon, int restricted, int uppercase, int lpfd);
void bas_runFile(const char *runFile);
void bas_runLine(const char *runLine);
void bas_interpreter(void);
void bas_exit(void);

#endif /* __APPS_EXAMPLES_BAS_BAS_H */
