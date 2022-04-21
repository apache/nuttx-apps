/****************************************************************************
 * apps/include/netutils/pppd.h
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

#ifndef __APPS_INCLUDE_NETUTILS_PPPD_H
#define __APPS_INCLUDE_NETUTILS_PPPD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Required configuration settings:
 *
 *   CONFIG_NETUTILS_PPPD_PAP - PPPD PAP authentication support
 *     Default: n
 */

#define TTYNAMSIZ               16
#define PAP_USERNAME_SIZE       16
#define PAP_PASSWORD_SIZE       16

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct pppd_settings_s
{
  /* Serial Interface */

  char ttyname[TTYNAMSIZ];

#ifdef CONFIG_NETUTILS_PPPD_PAP
  /* PAP Authentication Settings */

  char pap_username[PAP_USERNAME_SIZE];
  char pap_password[PAP_PASSWORD_SIZE];
#endif /* CONFIG_NETUTILS_PPPD_PAP */

  /* Chat Scripts */

  FAR const char *connect_script;
  FAR const char *disconnect_script;
};

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
 * Public Functions Definitions
 ****************************************************************************/

/****************************************************************************
 * Name: pppd
 *
 * Description:
 *   Create an pppd connection
 *
 * Input Parameters:
 *    pppd_settings, setting struct for the ppp connection
 *
 * Returned Value:
 *   Returns termination state, blocking as long as the connection is up
 *
 ****************************************************************************/

int pppd(const struct pppd_settings_s *ppp_settings);

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif /* __APPS_INCLUDE_NETUTILS_PPPD_H */
