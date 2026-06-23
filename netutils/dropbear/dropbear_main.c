/****************************************************************************
 * apps/netutils/dropbear/dropbear_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/socket.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "includes.h"
#include "algo.h"
#include "crypto_desc.h"
#define dropbear_main dropbear_multi_entry
#include "dbutil.h"
#undef dropbear_main
#include "dbrandom.h"
#include "netio.h"
#include "runopts.h"
#include "session.h"
#include "signkey.h"
#include "gensignkey.h"
#include "ssh.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DROPBEAR_PORT_STRING_HELPER(n) #n
#define DROPBEAR_PORT_STRING(n) DROPBEAR_PORT_STRING_HELPER(n)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef void (*dropbear_exit_handler_t)(int exitcode, FAR const char *format,
                                        va_list param) ATTRIB_NORETURN;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static jmp_buf g_session_exit_jmp;
static int g_session_exitcode;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void dropbear_session_exit(int exitcode, FAR const char *format,
                                  va_list param) noreturn_function;
static void dropbear_session_exit(int exitcode, FAR const char *format,
                                  va_list param)
{
  char exitmsg[150];
  char fullmsg[300];
  char fromaddr[60];
  int signal_pipe[2];

  vsnprintf(exitmsg, sizeof(exitmsg), format, param);

  fromaddr[0] = '\0';

  if (svr_ses.addrstring != NULL)
    {
      snprintf(fromaddr, sizeof(fromaddr), " from <%s>", svr_ses.addrstring);
    }

  if (!ses.init_done)
    {
      snprintf(fullmsg, sizeof(fullmsg), "Early exit%s: %s", fromaddr,
               exitmsg);
    }
  else if (ses.authstate.authdone)
    {
      snprintf(fullmsg, sizeof(fullmsg), "Exit (%s)%s: %s",
               ses.authstate.pw_name, fromaddr, exitmsg);
    }
  else if (ses.authstate.pw_name != NULL)
    {
      snprintf(fullmsg, sizeof(fullmsg),
               "Exit before auth%s: (user '%s', %u fails): %s",
               fromaddr, ses.authstate.pw_name, ses.authstate.failcount,
               exitmsg);
    }
  else
    {
      snprintf(fullmsg, sizeof(fullmsg), "Exit before auth%s: %s", fromaddr,
               exitmsg);
    }

  dropbear_log(LOG_INFO, "%s", fullmsg);

  signal_pipe[0] = ses.signal_pipe[0];
  signal_pipe[1] = ses.signal_pipe[1];
  session_cleanup();

  if (signal_pipe[0] > STDERR_FILENO)
    {
      m_close(signal_pipe[0]);
    }

  if (signal_pipe[1] > STDERR_FILENO && signal_pipe[1] != signal_pipe[0])
    {
      m_close(signal_pipe[1]);
    }

  memset(&ses, 0, sizeof(ses));
  memset(&svr_ses, 0, sizeof(svr_ses));

  g_session_exitcode = exitcode;
  longjmp(g_session_exit_jmp, 1);

  while (1)
    {
    }
}

static void dropbear_setup(FAR const char *port)
{
  /* Load the host key with -r so it lives in the persistent (epoch 0)
   * allocation rather than being created lazily inside a per-session malloc
   * epoch (-R), which would free it when the first session ends and leave a
   * dangling svr_opts.hostkey for the next connection.
   */

  FAR char *argv[] =
  {
    "dropbear",
    "-F",
    "-p",
    (FAR char *)port,
    "-r",
    CONFIG_NETUTILS_DROPBEAR_HOSTKEY_PATH,
    NULL
  };

  _dropbear_exit = svr_dropbear_exit;
  _dropbear_log = svr_dropbear_log;

  disallow_core();
  svr_getopts(6, argv);
  seedrandom();
  crypto_init();

#ifdef CONFIG_NETUTILS_DROPBEAR_GENERATE_HOSTKEY
  /* Generate the ECDSA host key now if it is missing.  signkey_generate()
   * with skip_exist=1 silently succeeds when the file already exists.
   */

  if (signkey_generate(DROPBEAR_SIGNKEY_ECDSA_NISTP256, 0,
                       CONFIG_NETUTILS_DROPBEAR_HOSTKEY_PATH, 1)
      == DROPBEAR_FAILURE)
    {
      dropbear_exit("failed to generate host key");
    }
#endif

  load_all_hostkeys();

  if (dropbear_auth_initialize() < 0)
    {
      dropbear_exit("failed to initialize password auth");
    }
}

static void dropbear_run_session(int childsock)
{
  dropbear_exit_handler_t saved_exit;

  saved_exit = _dropbear_exit;
  _dropbear_exit = dropbear_session_exit;

  m_malloc_set_epoch(1);

  if (setjmp(g_session_exit_jmp) == 0)
    {
      svr_session(childsock, -1);
    }

  m_malloc_free_epoch(1, 1);
  m_malloc_set_epoch(0);

  _dropbear_exit = saved_exit;

  if (g_session_exitcode != EXIT_SUCCESS)
    {
      dropbear_log(LOG_WARNING, "session exited with status %d",
                   g_session_exitcode);
    }
}

static size_t dropbear_listen_sockets(FAR int *socks, size_t sockcount,
                                      FAR int *maxfd)
{
  FAR char *errstring = NULL;
  size_t sockpos = 0;
  unsigned int i;

  for (i = 0; i < svr_opts.portcount; i++)
    {
      int nsock;
      unsigned int n;

      nsock = dropbear_listen(svr_opts.addresses[i], svr_opts.ports[i],
                              &socks[sockpos], sockcount - sockpos,
                              &errstring, maxfd, svr_opts.interface);

      if (nsock < 0)
        {
          dropbear_log(LOG_WARNING, "failed listening on '%s': %s",
                       svr_opts.ports[i],
                       errstring != NULL ? errstring : "unknown error");
          m_free(errstring);
          errstring = NULL;
          continue;
        }

      for (n = 0; n < (unsigned int)nsock; n++)
        {
          set_sock_priority(socks[sockpos + n], DROPBEAR_PRIO_LOWDELAY);
        }

      sockpos += (size_t)nsock;
    }

  return sockpos;
}

static size_t dropbear_wait_listen_sockets(FAR int *socks, size_t sockcount,
                                           FAR int *maxfd)
{
  int retries = 0;
  int retry_delay = 1;

  while (1)
    {
      size_t count;

      *maxfd = -1;
      count = dropbear_listen_sockets(socks, sockcount, maxfd);

      if (count > 0)
        {
          return count;
        }

#if CONFIG_NETUTILS_DROPBEAR_LISTEN_RETRIES > 0
      if (retries++ >= CONFIG_NETUTILS_DROPBEAR_LISTEN_RETRIES)
        {
          dropbear_exit("no listening ports available");
        }
#else
      retries++;
#endif

      dropbear_log(LOG_WARNING, "Retry %d in %d seconds...", retries,
                   retry_delay);
      sleep(retry_delay);

      retry_delay *= 2;

      if (retry_delay > CONFIG_NETUTILS_DROPBEAR_LISTEN_RETRY_MAX)
        {
          retry_delay = CONFIG_NETUTILS_DROPBEAR_LISTEN_RETRY_MAX;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *port = DROPBEAR_PORT_STRING(CONFIG_NETUTILS_DROPBEAR_PORT);
  int listensocks[MAX_LISTEN_ADDR];
  int maxfd = -1;

  if (argc > 1)
    {
      port = argv[1];
    }

  dropbear_setup(port);

  dropbear_wait_listen_sockets(listensocks, MAX_LISTEN_ADDR, &maxfd);

  printf("dropbear: listening on port %s\n", port);

  while (1)
    {
      struct sockaddr_storage remoteaddr;
      socklen_t remoteaddrlen = sizeof(remoteaddr);
      FAR char *remote_host = NULL;
      FAR char *remote_port = NULL;
      int childsock;

      childsock = accept(listensocks[0], (FAR struct sockaddr *)&remoteaddr,
                         &remoteaddrlen);

      if (childsock < 0)
        {
          continue;
        }

      getaddrstring(&remoteaddr, &remote_host, &remote_port, 0);
      dropbear_log(LOG_INFO, "connection from %s:%s",
                   remote_host != NULL ? remote_host : "?",
                   remote_port != NULL ? remote_port : "?");
      m_free(remote_host);
      m_free(remote_port);

      seedrandom();
      dropbear_run_session(childsock);
      close(childsock);
    }

  return EXIT_SUCCESS;
}
