/****************************************************************************
 * apps/examples/watched/watched.h
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

#ifndef __APPS_EXAMPLES_WATCHER_WATCHED_H
#define __APPS_EXAMPLES_WATCHER_WATCHED_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <signal.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct watcher_info_s
{
    int sub_cmd;
    int unsub_cmd;
    int feed_cmd;
    int signal;
    pid_t watcher_pid;
    const char info_file_name[];
};

struct request_s
{
    pid_t task_id;
    volatile int code;
};

struct watched_info_s
{
    struct request_s sub_request;
    struct request_s unsub_request;
    struct request_s feed_request;
    union sigval sub_value;
    union sigval unsub_value;
    union sigval feed_value;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

bool watched_is_watcher_on(void);
int  watched_read_watcher_info(struct watched_info_s *info);
int  watched_subscribe(struct watched_info_s *info);
int  watched_unsubscribe(struct watched_info_s *info);
int  watched_feed_dog(struct watched_info_s *info);

#endif /* __APPS_EXAMPLES_WATCHER_WATCHED_H */
