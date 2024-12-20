/****************************************************************************
 * apps/netutils/telnetd/telnetd_daemon.c
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

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>
#include <spawn.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/net/telnet.h>

#include "netutils/telnetd.h"
#include "netutils/netlib.h"

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

int telnetd_daemon(FAR const struct telnetd_config_s *config)
{
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

#ifdef CONFIG_SCHED_HAVE_PARENT
  struct sigaction sa;
  sigset_t blockset;
#endif
  socklen_t addrlen;
  int listensd;
  int acceptsd;
#ifdef CONFIG_NET_SOCKOPTS
  int optval;
#endif

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
      goto errout;
    }

  /* Block receipt of the SIGCHLD signal */

  sigemptyset(&blockset);
  sigaddset(&blockset, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &blockset, NULL) < 0)
    {
      nerr("ERROR: sigprocmask failed: %d\n", errno);
      goto errout;
    }
#endif /* CONFIG_SCHED_HAVE_PARENT */

  /* Create a new TCP socket to use to listen for connections */

  listensd = socket(config->d_family, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (listensd < 0)
    {
      nerr("ERROR: socket() failed for family %u: %d\n",
           config->d_family, errno);
      goto errout;
    }

#ifdef CONFIG_NET_SOCKOPTS
  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(listensd, SOL_SOCKET, SO_REUSEADDR,
                 &optval, sizeof(int)) < 0)
    {
      nerr("ERROR: setsockopt SO_REUSEADDR failure: %d\n", errno);
      goto errout_with_socket;
    }
#endif

  /* Bind the socket to a local address */

#ifdef CONFIG_NET_IPv4
  if (config->d_family == AF_INET)
    {
      addr.ipv4.sin_family      = AF_INET;
      addr.ipv4.sin_port        = config->d_port;
      addr.ipv4.sin_addr.s_addr = INADDR_ANY;
      addrlen                   = sizeof(struct sockaddr_in);
    }
  else
#endif
#ifdef CONFIG_NET_IPv6
  if (config->d_family == AF_INET6)
    {
      addr.ipv6.sin6_family     = AF_INET6;
      addr.ipv6.sin6_port       = config->d_port;
      addrlen                   = sizeof(struct sockaddr_in6);

      memset(addr.ipv6.sin6_addr.s6_addr, 0,
             sizeof(addr.ipv6.sin6_addr.s6_addr));
    }
  else
#endif
    {
      nerr("ERROR: Unsupported address family: %u", config->d_family);
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

  /* Begin accepting connections */

  for (; ; )
    {
      struct telnet_session_s session;
#ifdef CONFIG_NET_SOLINGER
      struct linger ling;
#endif
      int drvrfd;

      /* Now go silent. */

      close(0);
      close(1);
      close(2);

      ninfo("Accepting connections on port %d\n", ntohs(config->d_port));

      addrlen = sizeof(addr);
      acceptsd = accept4(listensd, &addr.generic, &addrlen, SOCK_CLOEXEC);
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

      drvrfd = open("/dev/telnet", O_RDONLY);
      if (drvrfd < 0)
        {
          nerr("ERROR: open(/dev/telnet) failed: %d\n", errno);
          goto errout_with_acceptsd;
        }

      /* Create a character device to "wrap" the accepted socket descriptor */

      ninfo("Creating the telnet driver\n");

      session.ts_sd         = acceptsd;
      session.ts_devpath[0] = '\0';

      if (ioctl(drvrfd, SIOCTELNET,
                (unsigned long)((uintptr_t)&session)) < 0)
        {
          nerr("ERROR: open(/dev/telnet) failed: %d\n", errno);
          close(drvrfd);
          goto errout_with_acceptsd;
        }

      close(drvrfd);

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

      /* Create a task to handle the connection.  The created task
       * will inherit the new stdin, stdout, and stderr.
       */

      ninfo("Starting the telnet session\n");

#ifndef CONFIG_BUILD_KERNEL
      if (config->t_entry)
        {
          pid_t pid = task_create("Telnet session",
                                  config->t_priority, config->t_stacksize,
                                  config->t_entry, NULL);
          if (pid >= 0)
            {
              continue;
            }
        }
#endif

#ifdef CONFIG_LIBC_EXECFUNCS
      if (config->t_path)
        {
          struct sched_param param;
          posix_spawnattr_t attr;
          pid_t pid;
          int ret;

          sched_getparam(0, &param);
          param.sched_priority = config->t_priority;

          posix_spawnattr_init(&attr);
          posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSCHEDPARAM);
          posix_spawnattr_setschedparam(&attr, &param);
          posix_spawnattr_setstacksize(&attr, config->t_stacksize);

          ret = posix_spawnp(&pid, config->t_path, NULL,
                             &attr, config->t_argv, NULL);
          if (ret > 0)
            {
              nerr("ERROR: Failed start the telnet session: %d\n", ret);
              errno = ret;
              goto errout_with_socket;
            }
        }
#endif
    }

errout_with_acceptsd:
  close(acceptsd);

errout_with_socket:
  close(listensd);
errout:
  return errno;
}
