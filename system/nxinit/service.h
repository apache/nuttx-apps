/****************************************************************************
 * apps/system/nxinit/service.h
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

#ifndef __APPS_SYSTEM_NXINIT_SERVICE_H
#define __APPS_SYSTEM_NXINIT_SERVICE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/list.h>

#include <stdint.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Ref to Android Init
 * URL: https://android.googlesource.com/platform/system/core/+/
 *      android16-release/init/service.h#42
 */
#define SVC_DISABLED    (1 << 0)  /* do not autostart with class */
#define SVC_ONESHOT     (1 << 1)  /* do not restart on exit */
#define SVC_RUNNING     (1 << 2)  /* currently active */
#define SVC_RESTARTING  (1 << 3)  /* waiting to restart */

/* This service should be stopped with SIGTERM instead of SIGKILL.
 * Will still be SIGKILLed after timeout period of 200 ms.
 */

#define SVC_GENTLE_KILL (1 << 13)

/* Flags below are new added.
 */

/* Override the previous definition for a service with the same name */

#define SVC_OVERRIDE    (1 << 29)

/* Already sent SIGKILL, and should not send again. */

#define SVC_SIGKILL     (1 << 30)

/* Should be removed immediately after stopped */

#define SVC_REMOVE      (1 << 31)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct service_class_s
{
  struct list_node node;     /* Service class list node */
  FAR char name[0];
};

struct service_s
{
  struct list_node node;     /* Service list node */
  struct list_node classes;  /* Class header, struct service_class_s */

  uint32_t flags;

  /* The explanation of the `argv` parameter.
   *
   * | Index   |   Value     |      Details      |
   * |---------|-------------|-------------------|
   * | argv[0] | "service"   | Statement keyword |
   * | argv[1] | name        | Service name      |
   * | argv[2] | argv[0]     | Service pathname  |
   * | argv[n] | argv[n - 2] | Service arguments |
   */

  FAR char *argv[CONFIG_SYSTEM_NXINIT_SERVICE_ARGS_MAX];
  int argc;

  struct timespec time_started;
  struct timespec time_kill;
  int restart_period;
  pid_t pid;

  /* The "target" of service option "reboot_on_failure" */

#ifdef CONFIG_BOARDCTL_RESET
  int reset_reason;
#endif
};

struct service_manager_s
{
  struct list_node services; /* Service header, struct service_s */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int  init_service_refresh(FAR struct service_manager_s *sm);
void init_service_reap(FAR struct service_s *service, int status);
int  init_service_start(FAR struct service_s *service);
int  init_service_stop(FAR struct service_s *service);
int  init_service_start_by_class(FAR struct service_manager_s *sm,
                                 FAR const char *name);
int  init_service_stop_by_class(FAR struct service_manager_s *sm,
                                FAR const char *name);
FAR struct service_s *
init_service_find_by_name(FAR struct service_manager_s *sm,
                          FAR const char *name);
FAR struct service_s *
init_service_find_by_pid(FAR struct service_manager_s *sm, const int pid);
int init_service_parse(FAR const struct parser_s *parser,
                       bool create, FAR char *buf);
int init_service_check(FAR const struct parser_s *parser);

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
void init_dump_service(FAR struct service_s *s);
void init_dump_services(FAR struct list_node *head);
#else
#  define init_dump_service(s)
#  define init_dump_services(h)
#endif
#endif /* __APPS_SYSTEM_NXINIT_SERVICE_H */
