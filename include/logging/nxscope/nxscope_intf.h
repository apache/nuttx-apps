/****************************************************************************
 * apps/include/logging/nxscope/nxscope_intf.h
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

#ifndef __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_INTF_H
#define __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_INTF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_LOGGING_NXSCOPE_INTF_SERIAL
#  include <termios.h>
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* Forward declaration */

struct nxscope_intf_s;

/* Nxscope interface ops */

struct nxscope_intf_ops_s
{
  /* Send data */

  CODE int (*send)(FAR struct nxscope_intf_s *s, FAR uint8_t *buff, int len);

  /* Receive data */

  CODE int (*recv)(FAR struct nxscope_intf_s *s, FAR uint8_t *buff, int len);
};

/* Nxscope interface */

struct nxscope_intf_s
{
  /* Initialized flag */

  bool initialized;

  /* Nxscope interface private data */

  FAR void *priv;

  /* Nxscope interface ops */

  FAR struct nxscope_intf_ops_s *ops;
};

#ifdef CONFIG_LOGGING_NXSCOPE_INTF_DUMMY
/* Nxscope dummy interface configuration */

struct nxscope_dummy_cfg_s
{
  int res;                      /* Reserved */
};
#endif

#ifdef CONFIG_LOGGING_NXSCOPE_INTF_SERIAL
/* Nxscope serial interface configuration */

struct nxscope_ser_cfg_s
{
  FAR char *path;               /* Device path */
  bool      nonblock;           /* Nonblocking operation */
  speed_t   baud;               /* Baud rate. Ignored if set to 0 */
};
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_LOGGING_NXSCOPE_INTF_DUMMY
/****************************************************************************
 * Name: nxscope_dummy_init
 ****************************************************************************/

int nxscope_dummy_init(FAR struct nxscope_intf_s *intf,
                       FAR struct nxscope_dummy_cfg_s *cfg);

/****************************************************************************
 * Name: nxscope_dummy_deinit
 ****************************************************************************/

void nxscope_dummy_deinit(FAR struct nxscope_intf_s *intf);
#endif

#ifdef CONFIG_LOGGING_NXSCOPE_INTF_SERIAL
/****************************************************************************
 * Name: nxscope_ser_init
 ****************************************************************************/

int nxscope_ser_init(FAR struct nxscope_intf_s *intf,
                     FAR struct nxscope_ser_cfg_s *cfg);

/****************************************************************************
 * Name: nxscope_ser_deinit
 ****************************************************************************/

void nxscope_ser_deinit(FAR struct nxscope_intf_s *intf);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif  /* __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_INTF_H */
