/****************************************************************************
 * apps/examples/watcher/task_mn.h
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

#ifndef __EXAMPLES_WATCHER_TASK_MN_H
#define __EXAMPLES_WATCHER_TASK_MN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/note/noteram_driver.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct request_s
{
    pid_t task_id;
    int code;
};

struct task_node_s
{
    pid_t task_id;
    bool reset;
    struct task_node_s *next;
};

struct task_list_s
{
    struct task_node_s *head;
    struct task_node_s *tail;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern volatile struct request_s request;
extern struct task_list_s watched_tasks;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void task_mn_print_tasks_status(void);
void task_mn_reset_all(void);
struct task_node_s *task_mn_is_task_subscribed(pid_t id);
void task_mn_add_to_list(pid_t id);
void task_mn_remove_from_list(pid_t id);
void task_mn_get_task_name(struct noteram_get_taskname_s *task);
void task_mn_subscribe(pid_t id);
void task_mn_unsubscribe(pid_t id);
bool task_mn_all_tasks_fed(void);

#endif                                 /* __EXAMPLES_WATCHER_TASK_MN_H */
