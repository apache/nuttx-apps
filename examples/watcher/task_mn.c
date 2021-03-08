/****************************************************************************
 * apps/examples/watcher/task_mn.c
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
#include <sys/boardctl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/note/noteram_driver.h>
#include "task_mn.h"

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

volatile struct request_s request;
struct task_list_s watched_tasks =
{
  .head = NULL,
  .tail = NULL
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void task_mn_print_tasks_status(void)
{
  int notefd;
  struct noteram_get_taskname_s task;
  struct task_node_s *node;

  /* If the list is not empty */

  if (watched_tasks.head != NULL)
    {
      /* Open the note driver */

      notefd = open("/dev/note", O_RDONLY);
      if (notefd < 0)
        {
          printf("Error: cannot open /dev/note\n");
          return;
        }

      /* Print all the nodes */

      for (node = watched_tasks.head; node != NULL; node = node->next)
        {
          task.pid = node->task_id;
          ioctl(notefd, NOTERAM_GETTASKNAME, (unsigned long)&task);
          if (node->reset)
            {
              printf("%s fed the dog.\n", task.taskname);
            }
          else
            {
              printf("%s starved the dog.\n", task.taskname);
            }
        }

      /* Close the note driver */

      close(notefd);
    }
  else
    {
      printf("Error: Task list is empty to print\n");
    }
}

void task_mn_reset_all(void)
{
  struct task_node_s *node;

  for (node = watched_tasks.head; node != NULL; node = node->next)
    {
      node->reset = false;
    }
}

struct task_node_s *task_mn_is_task_subscribed(pid_t id)
{
  struct task_node_s *node;

  /* If list is not empty */

  if (watched_tasks.head != NULL)
    {
      /* Search for the node */

      for (node = watched_tasks.head; node != NULL; node = node->next)
        {
          if (node->task_id == id)
            {
              return node;
            }
        }
    }

  return NULL;
}

void task_mn_add_to_list(pid_t id)
{
  struct task_node_s *node;

  /* Alloc the node */

  node = malloc(sizeof(struct task_node_s));
  if (node == NULL)
    {
      fprintf(stderr, "watcher daemon: Couldn't alloc a node to list\n");
      return;
    }

  node->task_id = id;

  /* NOTE: Once a task is subscribed, its initial status is that it fed the
   * dog. This approach was used first to avoid a false-positive result,
   * e.g., the task has been subscribed immediately before the watchdog
   * expiration and it did not feed the dog within this interval,
   * so the wdt handler would be triggered even if the subscribed
   * task would feed the dog in time.
   * The second reason is that we can consider the subscription request
   * itself an advertisement that the watched task is alive and not stuck.
   */

  node->reset = true;
  node->next = NULL;

  /* If list is not empty */

  if (watched_tasks.head != NULL)
    {
      watched_tasks.tail->next = node;
    }
  else
    {
      watched_tasks.head = node;
    }

  watched_tasks.tail = node;
}

void task_mn_remove_from_list(pid_t id)
{
  struct task_node_s *prev;
  struct task_node_s *current;

  /* If list is empty */

  if (watched_tasks.head == NULL)
    {
      fprintf(stderr, "watcher daemon: List is empty\n");
      return;
    }

  /* First element */

  else if (watched_tasks.head->task_id == id)
    {
      if (watched_tasks.head == watched_tasks.tail)
        {
          free(watched_tasks.head);
          watched_tasks.head = NULL;
          watched_tasks.tail = NULL;
          return;
        }
      else
        {
          prev = watched_tasks.head;
          watched_tasks.head = prev->next;
          free(prev);
          return;
        }
    }
  else
    {
      /* Search the node */

      prev = watched_tasks.head;
      current = prev->next;
      while (current != NULL)
        {
          if (current->task_id == id)
            {
              prev->next = current->next;

              /* In case the one that will be removed is the tail */

              if (prev->next == NULL)
                {
                  watched_tasks.tail = prev;
                }

              free(current);
              return;
            }

          prev = prev->next;
          current = current->next;
        }
    }

  fprintf(stderr, "watcher daemon: This node is not in the list.\n");
}

void task_mn_get_task_name(struct noteram_get_taskname_s *task)
{
  int notefd;

  notefd = open("/dev/note", O_RDONLY);
  if (notefd < 0)
    {
      fprintf(stderr, "trace: cannot open /dev/note\n");
      return;
    }

  ioctl(notefd, NOTERAM_GETTASKNAME, (unsigned long)task);
  close(notefd);
}

void task_mn_subscribe(pid_t id)
{
  struct noteram_get_taskname_s task;

  /* Verify if the task exists in the list */

  if (task_mn_is_task_subscribed(id) != NULL)
    {
      task.pid = id;
      task_mn_get_task_name(&task);
      printf("Task %s was already subscribed\n", task.taskname);
    }
  else
    {
      /* If it doesn't, include it to the list */

      task_mn_add_to_list(id);
    }
}

void task_mn_unsubscribe(pid_t id)
{
  struct noteram_get_taskname_s task;

  /* Verify if the task exists in the list */

  if (task_mn_is_task_subscribed(id) != NULL)
    {
      /* If it does, remove it from the list */

      task_mn_remove_from_list(id);
    }
  else
    {
      task.pid = id;
      task_mn_get_task_name(&task);
      printf("Task %s is not subscribed\n", task.taskname);
    }
}

bool task_mn_all_tasks_fed(void)
{
  struct task_node_s *node;

  for (node = watched_tasks.head; node != NULL; node = node->next)
    {
      /* If at least one did not feed the dog, return false */

      if (node->reset == false)
        {
          return false;
        }
    }

  return true;
}
