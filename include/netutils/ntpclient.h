/****************************************************************************
 * apps/include/netutils/ntpclient.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_NETUTILS_NTPCLIENT_H
#define __APPS_INCLUDE_NETUTILS_NTPCLIENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/socket.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NETUTILS_NTPCLIENT_SERVERIP
#  define CONFIG_NETUTILS_NTPCLIENT_SERVERIP 0x0a000001
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_PORTNO
#  define CONFIG_NETUTILS_NTPCLIENT_PORTNO 123
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_STACKSIZE
#  define CONFIG_NETUTILS_NTPCLIENT_STACKSIZE 2048
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_SERVERPRIO
#  define CONFIG_NETUTILS_NTPCLIENT_SERVERPRIO 100
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC
#  define CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC 60
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_SIGWAKEUP
#  define CONFIG_NETUTILS_NTPCLIENT_SIGWAKEUP 18
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: ntpc_dualstack_family()
 *
 * Description:
 *   Set the protocol family used (AF_INET, AF_INET6 or AF_UNSPEC)
 *
 ****************************************************************************/

void ntpc_dualstack_family(int family);

/****************************************************************************
 * Name: ntpc_start_with_list
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start_with_list(FAR const char *ntp_server_list);

/****************************************************************************
 * Name: ntpc_start
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start(void);

/****************************************************************************
 * Name: ntpc_stop
 *
 * Description:
 *   Stop the NTP daemon
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.  The current
 *   implementation only returns success.
 *
 ****************************************************************************/

int ntpc_stop(void);

/****************************************************************************
 * Name: ntpc_status
 *
 * Description:
 *   Get a status of the NTP daemon
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

struct ntpc_status_s
{
  /* the latest samples */

  unsigned int nsamples;
  struct
  {
    int64_t offset;
    int64_t delay;
    FAR const struct sockaddr *srv_addr;
    struct sockaddr_storage _srv_addr_store;
  }
  samples[CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES];
};

int ntpc_status(struct ntpc_status_s *statusp);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_NTPCLIENT_H */
