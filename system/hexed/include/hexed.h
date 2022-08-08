/****************************************************************************
 * apps/system/hexed/include/hexed.h
 * Command line HEXadecimal file EDitor header
 *
 *   Copyright (c) 2011, B.ZaaR, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS
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

#ifndef __APPS_SYSTEM_HEXED_INCLUDE_HEXED_H
#define __APPS_SYSTEM_HEXED_INCLUDE_HEXED_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "bfile.h"
#include "cmdargs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Version number */

#define VER_MAJOR       0
#define VER_MINOR       2
#define VER_REVISION    2

/* hexed constants */

#define PNAME           "hexed"       /* Program name */
#define CMD_MAX_CNT     0x10          /* Command table max count */
#define OPT_BUF_SZ      0x40          /* Option buffer size */

/* Word sizes */

#define WORD_64         0x08
#define WORD_32         0x04
#define WORD_16         0x02
#define WORD_8          0x01

/* Command flags */

#define CMD_FL_CMDLINE  0x00000001    /* Command set on command line */
#define CMD_FL_QUIT     0x00000002    /* Quit after command line */
#define CMD_FL_OVERFLOW 0x00000003    /* Command overflow */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Command list */

enum
{
  /* Command line options */

  CMD_HELP = 1,
  CMD_COPY_OVER,
  CMD_COPY,
  CMD_DUMP,
  CMD_ENTER,
  CMD_FIND,
  CMD_INSERT,
  CMD_MOVE_OVER,
  CMD_MOVE,
  CMD_REMOVE,
  CMD_WORD,

  /* Internal options */

  CMD_QUIT
};

/* Command options */

struct cmdoptions_s
{
  int word;
  long dest;
  long src;
  long len;
  long cnt;
  long bytes;
  int64_t buf[OPT_BUF_SZ];
};

/* Command details */

struct command_s
{
  FAR char *cmd;
  int id;
  int flags;
  struct cmdoptions_s opts;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern FAR struct bfile_s *g_hexfile;
extern int g_wordsize;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void hexed_fatal(FAR const char *fmt, ...);
int  hexcopy(FAR struct command_s *cmd, int optc, FAR char *opt);
int  hexdump(FAR struct command_s *cmd, int optc, FAR char *opt);
int  hexenter(FAR struct command_s *cmd, int optc, FAR char *opt);
int  hexhelp(FAR struct command_s *cmd, int optc, FAR char *opt);
int  hexinsert(FAR struct command_s *cmd, int optc, FAR char *opt);
int  hexmove(FAR struct command_s *cmd, int optc, FAR char *opt);
int  hexremove(FAR struct command_s *cmd, int optc, FAR char *opt);
int  hexword(FAR struct command_s *cmd, int optc, FAR char *opt);

#endif /* __APPS_SYSTEM_HEXED_INCLUDE_HEXED_H */
