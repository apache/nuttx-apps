/****************************************************************************
 * apps/netutils/telnetd/telnetd_daemon.c
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
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <spawn.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <nuttx/net/telnet.h>

#include "netutils/telnetd.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: telnetd_daemon
 *
 * Description:
 *   This function is the Telnet daemon.  It does not return (unless an
 *   error occurs).
 *
 * Parameters:
 *   Standard task start up arguments.
 *
 * Return:
 *   Does not return unless an error occurs.
 *
 ****************************************************************************/

int telnetd_daemon(int argc, FAR char *argv[])
{
  UNUSED(argc);

  FAR struct telnetd_s *daemon;
  union
  {
    struct sockaddr     generic;
#ifdef CONFIG_NET_IPv4
    struct sockaddr_in  ipv4;
#endif
#ifdef CONFIG_NET_IPv6
    struct sockaddr_in6 ipv6;
#endif
  } addr;

#ifdef CONFIG_NETUTILS_TELNETD_USE_POSIX_SPAWNP
  posix_spawn_file_actions_t file_actions;
  posix_spawnattr_t attr;
  FAR char *argv1[2];
#endif
  struct telnet_session_s session;
#ifdef CONFIG_NET_SOLINGER
  struct linger ling;
#endif
#ifdef CONFIG_SCHED_HAVE_PARENT
  struct sigaction sa;
  sigset_t blockset;
#endif
  socklen_t addrlen;
  pid_t pid;
  int listensd;
  int acceptsd;
  int drvrfd;
#ifdef CONFIG_NET_SOCKOPTS
  int optval;
#endif
  int ret;
  int fd;

  /* Get daemon startup info */

  daemon = (FAR struct telnetd_s *)((uintptr_t)strtoul(argv[1], NULL, 0));
  DEBUGASSERT(daemon != NULL);

#ifdef CONFIG_SCHED_HAVE_PARENT
  /* Call sigaction with the SA_NOCLDWAIT flag so that we do not transform
   * children into "zombies" when they terminate:  Child exit status will
   * not be retained.
   *
   * NOTE: If the SA_NOCLDWAIT flag is set when establishing a handler for
   * SIGCHLD, POSIX.1 leaves it unspecified whether a SIGCHLD signal is
   * generated when a child process terminates.  On both Linux and NuttX, a
   * SIGCHLD signal will be generated in this case.
   */

  sa.sa_handler = SIG_IGN;
  sa.sa_flags = SA_NOCLDWAIT;
  if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
      nerr("ERROR: sigaction failed: %d\n", errno);
      goto errout_with_daemon;
    }

  /* Block receipt of the SIGCHLD signal */

  sigemptyset(&blockset);
  sigaddset(&blockset, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &blockset, NULL) < 0)
    {
      nerr("ERROR: sigprocmask failed: %d\n", errno);
      goto errout_with_daemon;
    }
#endif /* CONFIG_SCHED_HAVE_PARENT */

  /* Create a new TCP socket to use to listen for connections */

  listensd = socket(daemon->family, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (listensd < 0)
    {
      nerr("ERROR: socket() failed for family %u: %d\n",
           daemon->family, errno);
      goto errout_with_daemon;
    }

#ifdef CONFIG_NET_SOCKOPTS
  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(listensd, SOL_SOCKET, SO_REUSEADDR,
                 (FAR void *)&optval, sizeof(int)) < 0)
    {
      nerr("ERROR: setsockopt SO_REUSEADDR failure: %d\n", errno);
      goto errout_with_socket;
    }
#endif

  /* Bind the socket to a local address */

#ifdef CONFIG_NET_IPv4
  if (daemon->family == AF_INET)
    {
      addr.ipv4.sin_family      = AF_INET;
      addr.ipv4.sin_port        = daemon->port;
      addr.ipv4.sin_addr.s_addr = INADDR_ANY;
      addrlen                   = sizeof(struct sockaddr_in);
    }
  else
#endif
#ifdef CONFIG_NET_IPv6
  if (daemon->family == AF_INET6)
    {
      addr.ipv6.sin6_family     = AF_INET6;
      addr.ipv6.sin6_port       = daemon->port;
      addrlen                   = sizeof(struct sockaddr_in6);

      memset(addr.ipv6.sin6_addr.s6_addr, 0,
             sizeof(addr.ipv6.sin6_addr.s6_addr));
    }
  else
#endif
    {
      nerr("ERROR: Unsupported address family: %u", daemon->family);
      goto errout_with_socket;
    }

  if (bind(listensd, &addr.generic, addrlen) < 0)
    {
      nerr("ERROR: bind failure: %d\n", errno);
      goto errout_with_socket;
    }

  /* Listen for connections on the bound TCP socket */

  if (listen(listensd, 5) < 0)
    {
      nerr("ERROR: listen failure %d\n", errno);
      goto errout_with_socket;
    }

  /* Now go silent. */

#ifndef CONFIG_DEBUG_FEATURES
  close(0);
  close(1);
  close(2);
#endif

  /* Begin accepting connections */

  for (; ; )
    {
      socklen_t accptlen;

      ninfo("Accepting connections on port %d\n", ntohs(daemon->port));

      accptlen = sizeof(addr);
      acceptsd = accept(listensd, &addr.generic, &accptlen);
      if (acceptsd < 0)
        {
          /* Just continue if a signal was received */

          if (errno == EINTR)
            {
              continue;
            }
          else
            {
              nerr("ERROR: accept failed: %d\n", errno);
              goto errout_with_socket;
            }
        }

#ifdef CONFIG_NET_SOLINGER
      /* Configure to "linger" until all data is sent when the socket is
       * closed
       */

      ling.l_onoff  = 1;
      ling.l_linger = 30;     /* timeout is seconds */
      if (setsockopt(acceptsd, SOL_SOCKET, SO_LINGER,
                     &ling, sizeof(struct linger)) < 0)
        {
          nerr("ERROR: setsockopt failed: %d\n", errno);
          goto errout_with_acceptsd;
        }
#endif

      /* Open the Telnet factory */

      fd = open("/dev/telnet", O_RDONLY);
      if (fd < 0)
        {
          nerr("ERROR: open(/dev/telnet) failed: %d\n", errno);
          goto errout_with_acceptsd;
        }

      /* Create a character device to "wrap" the accepted socket descriptor */

      ninfo("Creating the telnet driver\n");

      session.ts_sd         = acceptsd;
      session.ts_devpath[0] = '\0';

      ret = ioctl(fd, SIOCTELNET, (unsigned long)((uintptr_t)&session));
      close(fd);

      if (ret < 0)
        {
          nerr("ERROR: open(/dev/telnet) failed: %d\n", errno);
          goto errout_with_acceptsd;
        }

      /* Open the driver */

      ninfo("Opening the telnet driver at %s\n", session.ts_devpath);
      drvrfd = open(session.ts_devpath, O_RDWR);
      if (drvrfd < 0)
        {
          nerr("ERROR: Failed to open %s: %d\n", session.ts_devpath, errno);
          goto errout_with_socket;
        }

      /* Use this driver as stdin, stdout, and stderror */

      dup2(drvrfd, 0);
      dup2(drvrfd, 1);
      dup2(drvrfd, 2);

      /* And we can close our original driver fd */

      if (drvrfd > 2)
        {
          close(drvrfd);
        }

#ifdef CONFIG_NETUTILS_TELNETD_USE_POSIX_SPAWNP
      posix_spawn_file_actions_init(&file_actions);
      ret = posix_spawnattr_init(&attr);

      if (ret < 0)
        {
          nerr("ERROR in posix_spawnattr_init(): %d\n", errno);
          goto errout_with_socket;
        }

      argv1[0] = CONFIG_NETUTILS_TELNETD_SHELL_PATH;
      argv1[1] = NULL;

      ret = posix_spawnp(&pid,
                         CONFIG_NETUTILS_TELNETD_SHELL_PATH,
                         &file_actions, &attr, argv1, NULL);

      if (ret < 0)
        {
          nerr("ERROR in posix_spawnp(): %d\n", errno);
          goto errout_with_attrs;
        }
#else
      /* Create a task to handle the connection.  The created task
       * will inherit the new stdin, stdout, and stderr.
       */

      ninfo("Starting the telnet session\n");
      pid = task_create("Telnet session", daemon->priority,
                        daemon->stacksize, daemon->entry, NULL);
      if (pid < 0)
        {
          nerr("ERROR: Failed start the telnet session: %d\n", errno);
          goto errout_with_socket;
        }
#endif

      /* Forget about the connection. */

      close(0);
      close(1);
      close(2);
    }

errout_with_acceptsd:
  close(acceptsd);

#ifdef CONFIG_NETUTILS_TELNETD_USE_POSIX_SPAWNP
errout_with_attrs:
  posix_spawnattr_destroy(&attr);
#endif

errout_with_socket:
  close(listensd);
errout_with_daemon:
  free(daemon);
  return 1;
}

/****************************************************************************
 * Name: telnetd_start
 *
 * Description:
 *   Start the telnet daemon.
 *
 * Parameters:
 *   config    A pointer to a configuration structure that characterizes the
 *             telnet daemon.  This configuration structure may be defined
 *             on the caller's stack because it is not retained by the
 *             daemon.
 *
 * Return:
 *   The process ID (pid) of the new telnet daemon is returned on
 *   success; A negated errno is returned if the daemon was not successfully
 *   started.
 *
 ****************************************************************************/

int telnetd_start(FAR struct telnetd_config_s *config)
{
  FAR char *argv[2];
  pid_t pid;

#ifdef CONFIG_NETUTILS_TELNETD_USE_POSIX_SPAWNP
  posix_spawn_file_actions_t file_actions;
  posix_spawnattr_t attr;
  int ret = 0;

  posix_spawn_file_actions_init(&file_actions);
  ret = posix_spawnattr_init(&attr);

  if (ret < 0)
    {
      nerr("ERROR in posix_spawnattr_init(): %d\n", errno);
      pid = -1;
      goto errout;
    }

  argv[0] = CONFIG_NETUTILS_TELNETD_PATH;
  argv[1] = NULL;

  ret = posix_spawnp(&pid,
                     CONFIG_NETUTILS_TELNETD_PATH,
                     &file_actions, &attr, argv, NULL);

  if (ret < 0)
    {
      nerr("ERROR in posix_spawnp(): %d\n", errno);
      pid = -1;
      goto errout_with_attrs;
    }

errout_with_attrs:
  posix_spawnattr_destroy(&attr);

errout:
  return pid;
#else
  FAR struct telnetd_s *daemon;
  char arg0[sizeof("0x1234567812345678")];

  /* Allocate a state structure for the new daemon */

  daemon = (FAR struct telnetd_s *)malloc(sizeof(struct telnetd_s));
  if (!daemon)
    {
      return -ENOMEM;
    }

  /* Initialize the daemon structure */

  daemon->port      = config->d_port;
  daemon->family    = config->d_family;
  daemon->priority  = config->t_priority;
  daemon->stacksize = config->t_stacksize;
  daemon->entry     = config->t_entry;

  /* Then start the new daemon */

  snprintf(arg0, sizeof(arg0), "0x%" PRIxPTR, (uintptr_t)daemon);
  argv[0] = arg0;
  argv[1] = NULL;

  pid = task_create("Telnet daemon", config->d_priority, config->d_stacksize,
                    telnetd_daemon, argv);
  if (pid < 0)
    {
      int errval = errno;
      free(daemon);
      nerr("ERROR: Failed to start the telnet daemon: %d\n", errval);
      return -errval;
    }

  /* Return success */

  return pid;
#endif
}
