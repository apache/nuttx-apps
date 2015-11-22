/****************************************************************************
 * apps/system/hexed/include/cmdargs.h
 * Command line argument parsing header
 *
 *   Copyright (c) 2010, B.ZaaR, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   The names of contributors may not be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_SYSTEM_HEXED_INCLUDE_CMDARGS_H
#define __APPS_SYSTEM_HEXED_INCLUDE_CMDARGS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Command arguments flags */
/* Flags set by the parser */

#define CMDARGS_FL_SETARG       0x00000001  /* Argument set with '-' */
#define CMDARGS_FL_LONGARG      0x00000002  /* long argument set with '--' */
#define CMDARGS_FL_SINGLE       0x00000004  /* Single stepping args */

/* flags to keep set between single stepping args */

#define CMDARGS_FL_RESET        (CMDARGS_FL_SINGLE | CMDARGS_FL_SETARG)

/* Flags set by the caller
 *
 * CMDARGS_FL_OPT    - Options accepted
 * CMDARGS_FL_OPTREQ - Options required
 */

#define CMDARGS_FL_OPT          0x00010000
#define CMDARGS_FL_OPTREQ       (0x00020000 | CMDARGS_FL_OPT)

#define CMDARGS_FL_CALLMASK(fl) (fl & 0xffff0000)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Argument list table */

struct arglist_s
{
  FAR char *name;
  int flags;
};

/* Command argument details */

struct cmdargs_s
{
  int idx;                    /* Last argument parsed */
  int flags;                  /* Argument flags */
  int argid;                  /* Argument id in struct arglist_s */
  FAR char *arg;              /* Argument string */
  FAR char *opt;              /* Options string */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern FAR struct cmdargs_s *g_cmdargs;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int parsecmdargs(FAR char *argv[], FAR const struct arglist_s *arglist);

#endif /* __APPS_SYSTEM_HEXED_INCLUDE_CMDARGS_H */
