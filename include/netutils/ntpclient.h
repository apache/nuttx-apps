/****************************************************************************
 * apps/include/netutils/ntpclient.h
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
