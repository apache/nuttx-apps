/****************************************************************************
 * apps/include/netutils/discover.h
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

#ifndef __APPS_INCLUDE_NETUTILS_DISCOVER_H
#define __APPS_INCLUDE_NETUTILS_DISCOVER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct discover_info_s
{
  uint8_t devclass;         /* Device class, 0xff for all devices */
  const char *description;
};

/****************************************************************************
 * Name: discover_start
 *
 * Description:
 *   Start the discover daemon.
 *
 * Input Parameters:
 *
 *   info    Discover information, if NULL mconf defaults will be used.
 *
 * Return:
 *   The process ID (pid) of the new discover daemon is returned on
 *   success; A negated errno is returned if the daemon was not successfully
 *   started.
 *
 ****************************************************************************/

int discover_start(struct discover_info_s *info);

#endif /* __APPS_INCLUDE_NETUTILS_DISCOVER_H */
