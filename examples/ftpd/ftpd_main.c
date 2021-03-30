/****************************************************************************
 * examples/ftpd/ftpd_main.c
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <debug.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "netutils/netlib.h"
#include "netutils/ftpd.h"

#include "ftpd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* If FTP is used and both IPv6 and IPv4 are enabled, then we need to
 * pick one.
 */

#ifdef CONFIG_NET_IPv6
#  define ADDR_FAMILY AF_INET6
#else
#  define ADDR_FAMILY AF_INET
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct fptd_account_s g_ftpdaccounts[] =
{
  { FTPD_ACCOUNTFLAG_SYSTEM, "root",      "abc123", NULL },
  { FTPD_ACCOUNTFLAG_GUEST,  "ftp",       NULL,     NULL },
  { FTPD_ACCOUNTFLAG_GUEST,  "anonymous", NULL,     NULL },
};
#define NACCOUNTS (sizeof(g_ftpdaccounts) / sizeof(struct fptd_account_s))

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* To minimize the probability of name collisitions, all FTPD example
 * global data is maintained in a single instance of a structure.
 */

struct ftpd_globals_s g_ftpdglob;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fptd_netinit
 ****************************************************************************/

static void fptd_netinit(void)
{
#ifndef CONFIG_EXAMPLES_FTPD_NONETINIT
  struct in_addr addr;
#ifdef CONFIG_EXAMPLES_FTPD_NOMAC
  uint8_t mac[IFHWADDRLEN];
#endif

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_FTPD_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_FTPD_IPADDR);
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_FTPD_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_FTPD_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

#endif /* CONFIG_EXAMPLES_FTPD_NONETINIT */
}

/****************************************************************************
 * Name: ftpd_accounts
 ****************************************************************************/

static void ftpd_accounts(FTPD_SESSION handle)
{
  FAR const struct fptd_account_s *account;
  int i;

  printf("Adding accounts:\n");
  for (i = 0; i < NACCOUNTS; i++)
    {
      account = &g_ftpdaccounts[i];

      printf("%d. %s account: USER=%s PASSWORD=%s HOME=%s\n", i + 1,
            (account->flags & FTPD_ACCOUNTFLAG_SYSTEM) ? "Root" : "User",
            (!account->user) ? "(none)" : account->user,
            (!account->password) ? "(none)" : account->password,
            (!account->home) ? "(none)" : account->home);

      ftpd_adduser(handle, account->flags, account->user,
                   account->password, account->home);
    }
}

/****************************************************************************
 * Name: ftpd_daemon
 ****************************************************************************/

int ftpd_daemon(int s_argc, char **s_argv)
{
  FTPD_SESSION handle;
  int ret;

  /* The FTPD daemon has been started */

  g_ftpdglob.running = true;
  printf("FTP daemon [%d] started\n", g_ftpdglob.pid);

  /* Open FTPD */

#if ADDR_FAMILY == AF_INET6
  handle = ftpd_open(AF_INET6);
#else
  handle = ftpd_open(AF_INET);
#endif

  if (!handle)
    {
      printf("FTP daemon [%d] failed to open FTPD\n", g_ftpdglob.pid);
      g_ftpdglob.running = false;
      g_ftpdglob.stop    = false;
      g_ftpdglob.pid     = -1;
      return EXIT_FAILURE;
    }

  /* Configure accounts */

  ftpd_accounts(handle);

  /* Then drive the FTPD server. */

  while (g_ftpdglob.stop == 0)
    {
      /* If ftpd_session returns success, it means that a new FTP session
       * has been started.
       */

      ret = ftpd_session(handle, 5000);

      /* If any interesting happened (i.e., any thing other than a timeout),
       * then report the interesting event.
       */

      if (ret != -ETIMEDOUT)
        {
          printf("FTP daemon [%d] ftpd_session returned %d\n",
                 g_ftpdglob.pid, ret);
        }
    }

  /* Close the FTPD server and exit. */

  printf("FTP daemon [%d] stopping\n", g_ftpdglob.pid);
  g_ftpdglob.running = false;
  g_ftpdglob.stop    = false;
  g_ftpdglob.pid     = -1;
  ftpd_close(handle);

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpd_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Check if we have already initialized the network */

  if (!g_ftpdglob.initialized)
    {
      /* Bring up the network */

      printf("Initializing the network\n");
      fptd_netinit();

      /* Initialize daemon state */

      g_ftpdglob.initialized = true;
      g_ftpdglob.pid         = -1;
      g_ftpdglob.stop        = false;
      g_ftpdglob.running     = false;
    }

  /* Then start the new daemon (if it is not already running) */

  if (g_ftpdglob.stop && g_ftpdglob.running)
    {
      printf("Waiting for FTP daemon [%d] to stop\n", g_ftpdglob.pid);
      return EXIT_FAILURE;
    }

  if (!g_ftpdglob.running)
    {
      printf("Starting the FTP daemon\n");
      g_ftpdglob.pid = task_create("FTP daemon", CONFIG_EXAMPLES_FTPD_PRIO,
                                   CONFIG_EXAMPLES_FTPD_STACKSIZE,
                                   ftpd_daemon, NULL);
      if (g_ftpdglob.pid < 0)
        {
          printf("Failed to start the FTP daemon: %d\n", errno);
          return EXIT_FAILURE;
        }
    }
  else
    {
      printf("FTP daemon [%d] is running\n", g_ftpdglob.pid);
    }

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: ftpd_stop
 ****************************************************************************/

int ftpd_stop_main(int s_argc, char **s_argv)
{
  if (!g_ftpdglob.initialized || !g_ftpdglob.running)
    {
      printf("The FTP daemon not running\n");
      return EXIT_FAILURE;
    }

  printf("Stopping the FTP daemon, pid=%d\n", g_ftpdglob.pid);
  g_ftpdglob.stop = true;
  return EXIT_SUCCESS;
}
