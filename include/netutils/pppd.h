/****************************************************************************
 * apps/include/netutils/pppd.h
 *
 *   Copyright (C) 2015 Brennan Ashton. All rights reserved.
 *   Author: Brennan Ashton <bashton@brennanashton.com>
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

  FAR const char* connect_script;
  FAR const char* disconnect_script;
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
 * Public Functions
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
