/****************************************************************************
 * apps/wireless/iwpan/src/iwpan.c
 *
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Anthony Merlino <anthony@vergeaero.com>
 *           Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of  source code must  retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "wireless/ieee802154.h"
#include "wireless/iwpan.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Describes one command */

struct iwpan_command_s
{
  FAR const char *name;
  uint8_t noptions;
  CODE void *handler;
};

/* Generic form of a command handler */

typedef void (*cmd1_t)(int sock, FAR const char *arg1);
typedef void (*cmd2_t)(int sock, FAR const char *arg1,
                       FAR const char *arg2);
typedef void (*cmd3_t)(int sock, FAR const char *arg1,
                       FAR const char *arg2, FAR const char *arg3);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void iwpan_showusage(FAR const char *progname, int exitcode);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iwpan_showusage
 *
 * Description:
 *   Show program usage.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void iwpan_showusage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage:\n", progname);
  fprintf(stderr, "\t%s <command> <ifname> [OPTIONS]\n");
  fprintf(stderr, "\nWhere supported commands an [OPTIONS] appear below\n");
  fprintf(stderr, "\nRadio Query:\n");
  fprintf(stderr, "\t%s show <ifname>\n", progname);
  fprintf(stderr, "\nRadio Settings:\n");
  fprintf(stderr, "\t%s cca <ifname> <cca>\n", progname);
  fprintf(stderr, "\t%s chan <ifname> <channel>\n", progname);
  fprintf(stderr, "\t%s devmode <ifname> <devmode>\n", progname);
  fprintf(stderr, "\t%s eaddr <ifname> <eaddr>\n", progname);
  fprintf(stderr, "\t%s panid <ifname> <panid>\n", progname);
  fprintf(stderr, "\t%s promisc <ifname> <true|false>\n", progname);
  fprintf(stderr, "\t%s saddr <ifname> <saddr>\n", progname);
  fprintf(stderr, "\t%s txpwr <ifname> <txpwr>\n", progname);
  fprintf(stderr, "\nMAC Commands:\n");
  fprintf(stderr, "\t%s assoc <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s disassoc <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s get <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s gts <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s poll <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s rxenab <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s scan <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s set <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s start <ifname> TBD\n", progname);
  fprintf(stderr, "\t%s sync <ifname> TBD\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int iwpan_main(int argc, char *argv[])
#endif
{
  return EXIT_SUCCESS;
}
