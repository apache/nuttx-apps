/****************************************************************************
 * apps/system/uorb/listener.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include <unistd.h>
#include <fcntl.h>

#include <uORB/uORB.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ORB_MAX_PRINT_NAME 32
#define ORB_TOP_WAIT_TIME  1000
#define ORB_DATA_DIR       "/data/uorb/"

#if defined(CONFIG_DEBUG_UORB) && !defined(CONFIG_LIBC_FLOATINGPOINT)
#error "Enable CONFIG_LIBC_FLOATINGPOINT, required to see debug output"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct listen_object_s
{
  SLIST_ENTRY(listen_object_s) node; /* Node of object info list */

  struct orb_object object; /* Object id */
  orb_abstime timestamp;    /* Time of lastest generation */
  unsigned long generation; /* Latest generation */
  FAR FILE *file;
};

SLIST_HEAD(listen_list_s, listen_object_s);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int listener_get_state(FAR struct orb_object *object,
                              FAR struct orb_state *state);
static int listener_add_object(FAR struct listen_list_s *objlist,
                               FAR struct orb_object *object);
static void listener_delete_object_list(FAR struct listen_list_s *objlist);
static int listener_generate_object_list(FAR struct listen_list_s *objlist,
                                         FAR const char *filter);
static int listener_print(FAR const struct orb_metadata *meta, int fd);
static void listener_monitor(FAR struct listen_list_s *objlist,
                             int nb_objects, float topic_rate,
                             int topic_latency, int nb_msgs,
                             int timeout, bool record);
static int listener_update(FAR struct listen_list_s *objlist,
                           FAR struct orb_object *object);
static void listener_top(FAR struct listen_list_s *objlist,
                         FAR const char *filter,
                         bool only_once);
static int listener_create_dir(FAR char *dir, size_t size);
static int listener_record(FAR const struct orb_metadata *meta, int fd,
                           FAR FILE *file);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_should_exit = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(void)
{
  uorbinfo_raw("\n\
Utility to listen on uORB topics and print the data to the console.\n\
\n\
The listener can be exited any time by pressing Ctrl+C, Esc, or Q.\n\
\n\
We use cmd 'uorb_listener <t1,t2,...> -n 1' to print once snapshot for\n\
specify topics if command exists or all the topic objects when command\n\
doesn't exists. If topic isn't updated, the snapshot for this topic is\n\
empty.\n\
\n\
We use cmd 'uorb_listener t1,t2,... [arguments...]' to print the topics\n\
specified. The following arguments apply to all topics.\n\
\n\
We use cmd 'uorb_listener ... -n num(>1) ...' to print all the topic\n\
objects message until the number of received is equal to the specified\n\
number.\n\
\n\
We can print the messages all the time when we do not specify the number\n\
of message.\n\
\n\
listener <command> [arguments...]\n\
 Commands:\n\
\t<topics_name> Topic name. Multi name are separated by ','\n\
\t[-h       ]  Listener commands help\n\
\t[-f       ]  Record uorb data to file\n\
\t[-n <val> ]  Number of messages, default: 0\n\
\t[-r <val> ]  Subscription rate (unlimited if 0), default: 0\n\
\t[-b <val> ]  Subscription maximum report latency in us(unlimited if 0),\n\
\t             default: 0\n\
\t[-t <val> ]  Time of listener, in seconds, default: 5\n\
\t[-T       ]  Top, continuously print updating objects\n\
\t[-l       ]  Top only execute once.\n\
  ");
}

/****************************************************************************
 * Name: creat_record_path
 *
 * Input Parameters:
 *   path   The path of the storage file.
 *
 * Description:
 *   Create the path where files are stored by default.
 *
 * Returned Value:
 *   0 on success, otherwise negative errno.
 ****************************************************************************/

static int listener_create_dir(FAR char *dir, size_t size)
{
  FAR struct tm *tm_info;
  time_t t;

  time(&t);
  tm_info = gmtime(&t);

  if (0 == strftime(dir, size, ORB_DATA_DIR "%Y%m%d%H%M%S/", tm_info))
    {
      return -EINVAL;
    }

  if (access(ORB_DATA_DIR, F_OK) != 0)
    {
      mkdir(ORB_DATA_DIR, 0777);
    }

  if (access(dir, F_OK) != 0)
    {
      mkdir(dir, 0777);
    }

  return OK;
}

/****************************************************************************
 * Name: listener_get_state
 *
 * Description:
 *   Get object's current state.
 *
 * Input Parameters:
 *   object   Given object
 *   state    Returned state.
 *
 * Returned Value:
 *   0 on success, otherwise negative errno.
 ****************************************************************************/

static int listener_get_state(FAR struct orb_object *object,
                              FAR struct orb_state *state)
{
  int ret;
  int fd;

  fd = orb_open(object->meta->o_name, object->instance, O_DIRECT);
  if (fd < 0)
    {
      return fd;
    }

  ret = orb_get_state(fd, state);
  orb_close(fd);
  return ret;
}

/****************************************************************************
 * Name: listener_add_object
 *
 * Description:
 *   Alloc object node and add to list.
 *
 * Input Parameters:
 *   object     Object to add.
 *   objlist    List to modify.
 *
 * Returned Value:
 *   0 on success, otherwise return negative errno.
 ****************************************************************************/

static int listener_add_object(FAR struct listen_list_s *objlist,
                               FAR struct orb_object *object)
{
  FAR struct listen_object_s *tmp;
  struct orb_state state;
  int ret;

  tmp = malloc(sizeof(struct listen_object_s));
  if (tmp == NULL)
    {
      return -ENOMEM;
    }

  ret = listener_get_state(object, &state);
  tmp->object.meta     = object->meta;
  tmp->object.instance = object->instance;
  tmp->timestamp       = orb_absolute_time();
  tmp->generation      = ret < 0 ? 0 : state.generation;
  tmp->file            = NULL;
  SLIST_INSERT_HEAD(objlist, tmp, node);
  return 0;
}

/****************************************************************************
 * Name: listener_update
 *
 * Description:
 *   Update object list, print imformation if given object has new data.
 *
 * Input Parameters:
 *   object     Object to check state.
 *   objlist    List to update.
 *
 * Returned Value:
 *   0 on success.
 ****************************************************************************/

static int listener_update(FAR struct listen_list_s *objlist,
                           FAR struct orb_object *object)
{
  FAR struct listen_object_s *old = NULL;
  FAR struct listen_object_s *tmp;
  int ret;

  /* Check wether object already exist in old list */

  SLIST_FOREACH(tmp, objlist, node)
    {
      if (tmp->object.meta == object->meta &&
          tmp->object.instance == object->instance)
        {
          old = tmp;
          break;
        }
    }

  if (old)
    {
      /* If object existed in old list, print and update. */

      struct orb_state state;
      orb_abstime now_time;
      unsigned long delta_time;
      unsigned long delta_generation;

      now_time = orb_absolute_time();
      ret = listener_get_state(object, &state);
      if (ret < 0)
        {
          return ret;
        }

      delta_time       = now_time - old->timestamp;
      delta_generation = state.generation - old->generation;
      if (delta_generation && delta_time)
        {
          unsigned long frequency;

          frequency = (state.max_frequency ? state.max_frequency : 1000000)
                      * delta_generation / delta_time;
          uorbinfo_raw("\033[K" "%-*s %2u %4" PRIu32 " %4lu "
                       "%2" PRIu32 " %4u",
                       ORB_MAX_PRINT_NAME,
                       object->meta->o_name,
                       object->instance,
                       state.nsubscribers,
                       frequency,
                       state.queue_size,
                       object->meta->o_size);
          old->generation = state.generation;
          old->timestamp  = now_time;
        }
    }
  else
    {
      /* If object not existed in old list, alloc one */

      ret = listener_add_object(objlist, object);
      if (ret < 0)
        {
          return ret;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: listener_delete_object_list
 *
 * Description:
 *   free object list.
 *
 * Input Parameters:
 *   objlist    List to free.
 *
 * Returned Value:
 *   None.
 ****************************************************************************/

static void listener_delete_object_list(FAR struct listen_list_s *objlist)
{
  FAR struct listen_object_s *tmp;

  while (!SLIST_EMPTY(objlist))
    {
      tmp = SLIST_FIRST(objlist);
      SLIST_REMOVE_HEAD(objlist, node);
      free(tmp);
    }

  SLIST_INIT(objlist);
}

/****************************************************************************
 * Name: listener_generate_object_list
 *
 * Description:
 *   Update / alloc object list by scan ORB_SENSOR_PATH.
 *
 * Input Parameters:
 *   objlist    List to update / alloc.
 *   filter     Specified topic names.
 *
 * Returned Value:
 *   File number under ORB_SENSOR_PATH on success.
 *   Negative errno on failure.
 ****************************************************************************/

static int listener_generate_object_list(FAR struct listen_list_s *objlist,
                                         FAR const char *filter)
{
  FAR struct dirent *entry;
  struct orb_object object;
  char name[ORB_PATH_MAX];
  FAR DIR *dir;
  size_t len;
  int cnt = 0;

  /* First traverse all objects in filter */

  if (filter)
    {
      FAR const char *tmp;
      FAR const char *member = filter;

      do
        {
          while (*member == ',')
            {
              member++;
            }

          tmp = strchr(member, ',');
          len = tmp ? tmp - member : strlen(member);
          if (!len)
            {
              return cnt;
            }

          strlcpy(name, member, len + 1);
          member = tmp;
          object.meta = orb_get_meta(name);
          if (object.meta)
            {
              object.instance = 0;
              while (1)
                {
                  if (isdigit(name[len - 1]))
                    {
                      object.instance = name[len - 1] - '0';
                    }

                  if (listener_update(objlist, &object) >= 0)
                    {
                      cnt++;
                      if (isdigit(name[len - 1]))
                        {
                          break;
                        }
                    }

                  if (orb_exists(object.meta, object.instance++) < 0)
                    {
                      break;
                    }
                }
            }
        }
      while (tmp);

      return cnt;
    }

  /* Traverse all objects under ORB_SENSOR_PATH */

  dir = opendir(ORB_SENSOR_PATH);
  if (!dir)
    {
      return 0;
    }

  while ((entry = readdir(dir)))
    {
      /* Get meta data and instance number through file name */

      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        {
          continue;
        }

      strlcpy(name, entry->d_name, ORB_PATH_MAX);

      len = strlen(name) - 1;
      object.instance = name[len] - '0';
      name[len] = 0;

      if (filter)
        {
          /* Example:
           *   objects: aaa0, aaa1, aaa2, bbb0, bbb1
           *   filter:  "aaa bbb1"
           * Object list we get:
           *   aaa0, aaa1, aaa2, bbb1.
           */

          FAR const char *str = strstr(filter, name);
          if (!str || (str[len] && str[len] != ',' &&
                       str[len] != object.instance + '0'))
            {
              continue;
            }
        }

      object.meta = orb_get_meta(entry->d_name);
      if (!object.meta)
        {
          continue;
        }

      /* Update object infomation to list. */

      if (listener_update(objlist, &object) < 0)
        {
          uorbinfo_raw("listener %s failed", object.meta->o_name);
          continue;
        }

      cnt++;
    }

  closedir(dir);
  return cnt;
}

/****************************************************************************
 * Name: listener_print
 *
 * Description:
 *   Print topic data by its print_message callback.
 *
 * Input Parameters:
 *   meta         The uORB metadata.
 *   fd           Subscriber handle.
 *
 * Returned Value:
 *   0 on success copy, otherwise -1
 ****************************************************************************/

static int listener_print(FAR const struct orb_metadata *meta, int fd)
{
  char buffer[meta->o_size];
  int ret;

  ret = orb_copy(meta, fd, buffer);
#ifdef CONFIG_DEBUG_UORB
  if (ret == OK && meta->o_format != NULL)
    {
      orb_info(meta->o_format, meta->o_name, buffer);
    }
#endif

  return ret;
}

/****************************************************************************
 * Name: listener_record
 *
 * Description:
 *   record topic data.
 *
 * Input Parameters:
 *   meta   The uORB metadata.
 *   fd     Subscriber handle.
 *   file   Save file handle.
 *
 * Returned Value:
 *   0 on success copy, otherwise -1
 ****************************************************************************/

static int listener_record(FAR const struct orb_metadata *meta, int fd,
                           FAR FILE *file)
{
  char buffer[meta->o_size];
  int ret;

  ret = orb_copy(meta, fd, buffer);
#ifdef CONFIG_DEBUG_UORB
  if (ret == OK && meta->o_format != NULL)
    {
      ret = orb_fprintf(file, meta->o_format, buffer);
    }
#else
  (void)file;
#endif

  return ret;
}

/****************************************************************************
 * Name: listener_monitor
 *
 * Description:
 *   Moniter objects by subscribe and print data.
 *
 * Input Parameters:
 *   objlist        List of objects to subscribe.
 *   nb_objects     Length of objects list.
 *   topic_rate     Subscribe frequency.
 *   topic_latency  Subscribe report latency.
 *   nb_msgs        Subscribe amount of messages.
 *   timeout        Maximum poll waiting time , ms.
 *
 * Returned Value:
 *   None
 ****************************************************************************/

static void listener_monitor(FAR struct listen_list_s *objlist,
                             int nb_objects, float topic_rate,
                             int topic_latency, int nb_msgs,
                             int timeout, bool record)
{
  FAR struct pollfd *fds;
  char path[PATH_MAX];
  FAR int *recv_msgs;
  float interval = topic_rate ? (1000000 / topic_rate) : 0;
  int nb_recv_msgs = 0;
  FAR char *dir;
  int i = 0;

  FAR struct listen_object_s *tmp;

  fds = malloc(nb_objects * sizeof(struct pollfd));
  if (!fds)
    {
      return;
    }

  recv_msgs = calloc(nb_objects, sizeof(int));
  if (!recv_msgs)
    {
      free(fds);
      return;
    }

  /* Prepare pollfd for all objects */

  SLIST_FOREACH(tmp, objlist, node)
    {
      int fd;

      fd = orb_subscribe_multi(tmp->object.meta, tmp->object.instance);
      if (fd < 0)
        {
          fds[i].fd     = -1;
          fds[i].events = 0;
          continue;
        }
      else
        {
          fds[i].fd     = fd;
          fds[i].events = POLLIN;
        }

      if (interval != 0)
        {
          orb_set_interval(fd, (unsigned)interval);

          if (topic_latency != 0)
            {
              orb_set_batch_interval(fd, topic_latency);
            }
        }

      i++;
    }

  if (record)
    {
      listener_create_dir(path, sizeof(path));
      dir = path + strlen(path);

      SLIST_FOREACH(tmp, objlist, node)
        {
          sprintf(dir, "%s%d.csv", tmp->object.meta->o_name,
                  tmp->object.instance);
          tmp->file = fopen(path, "w");
          if (tmp->file != NULL)
            {
#ifdef CONFIG_DEBUG_UORB
              fprintf(tmp->file, "%s,%d,%d,%s\n", tmp->object.meta->o_format,
                      tmp->object.meta->o_size, tmp->object.instance,
                      tmp->object.meta->o_name);
#endif

              uorbinfo_raw("creat file:[%s]", path);
            }
          else
            {
              uorbinfo_raw("file creat failed!meta name:%s,instance:%d",
                           tmp->object.meta->o_name, tmp->object.instance);
            }
        }
    }

  /* Loop poll and print recieved messages */

  while ((!nb_msgs || nb_recv_msgs < nb_msgs) && !g_should_exit)
    {
      if (poll(&fds[0], nb_objects, timeout * 1000) > 0)
        {
          i = 0;
          SLIST_FOREACH(tmp, objlist, node)
            {
              if (fds[i].revents & POLLIN)
                {
                  nb_recv_msgs++;
                  recv_msgs[i]++;

                  if (tmp->file != NULL)
                    {
                      if (listener_record(tmp->object.meta, fds[i].fd,
                                          tmp->file) < 0)
                        {
                          uorberr("Listener record %s data failed!",
                                  tmp->object.meta->o_name);
                        }
                    }
                  else
                    {
                      if (listener_print(tmp->object.meta, fds[i].fd) != 0)
                        {
                          uorberr("Listener callback failed");
                        }
                    }

                  if (nb_msgs && nb_recv_msgs >= nb_msgs)
                    {
                      break;
                    }
                }

              i++;
            }
        }
      else if (errno != EINTR)
        {
          uorbinfo_raw("Waited for %d seconds without a message. "
                       "Giving up. err:%d", timeout, errno);
          break;
        }
    }

  i = 0;
  SLIST_FOREACH(tmp, objlist, node)
    {
      if (fds[i].fd < 0)
        {
          uorbinfo_raw("Object name:%s%d, subscribe fail",
                       tmp->object.meta->o_name, tmp->object.instance);
        }
      else
        {
          if (topic_latency)
            {
              orb_set_batch_interval(fds[i].fd, 0);
            }

          orb_unsubscribe(fds[i].fd);
          uorbinfo_raw("Object name:%s%d, recieved:%d",
                       tmp->object.meta->o_name, tmp->object.instance,
                       recv_msgs[i]);
        }

      if (tmp->file != NULL)
        {
          fflush(tmp->file);
          fclose(tmp->file);
        }

      i++;
    }

  uorbinfo_raw("Total number of received Message:%d/%d",
               nb_recv_msgs, nb_msgs ? nb_msgs : nb_recv_msgs);
  free(fds);
  free(recv_msgs);
}

/****************************************************************************
 * Name: listener_top
 *
 * Description:
 *   Continuously print updating objects, like the unix 'top' command.
 *   Exited when the user presses the enter key.
 *
 * Input Parameters:
 *   objlist    List of objects.
 *   filter     Specific topic names.
 *   only_once  Print only once, then exit.
 *
 * Returned Value:
 *   None.
 ****************************************************************************/

static size_t listen_length(FAR struct listen_list_s *objlist)
{
  struct listen_object_s *tmp;
  size_t count = 0;

  SLIST_FOREACH(tmp, objlist, node)
    {
      count++;
    }

  return count;
}

static void listener_top(FAR struct listen_list_s *objlist,
                         FAR const char *filter,
                         bool only_once)
{
  bool quit = false;
  struct pollfd fds;

  fds.fd     = STDIN_FILENO;
  fds.events = POLLIN;

  uorbinfo_raw("\033[2J\n"); /* clear screen */

  do
    {
      /* Wait a while, quit if user input some thing */

      if (poll(&fds, 1, ORB_TOP_WAIT_TIME) > 0)
        {
          char c;

          if (read(STDIN_FILENO, &c, 1) > 0)
            {
              quit = true;
              break;
            }
        }

      /* Then Update object list and print changes. */

      if (!only_once)
        {
          uorbinfo_raw("\033[H"); /* move cursor to top left corner */
        }

      uorbinfo_raw("\033[K" "current objects: %zu", listen_length(objlist));
      uorbinfo_raw("\033[K" "%-*s INST #SUB RATE #Q SIZE",
                   ORB_MAX_PRINT_NAME - 2, "NAME");

      if (listener_generate_object_list(objlist, filter) < 0)
        {
          uorberr("Failed to update object list");
          return;
        }

      if (!only_once)
        {
          uorbinfo_raw("\033[0J"); /* Clear the rest of the screen */
        }
    }
  while (!quit && !only_once);
}

static void exit_handler(int signo)
{
  (void)signo;
  g_should_exit = true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct listen_object_s *tmp;
  struct listen_list_s objlist;
  float topic_rate = 0;
  int topic_latency = 0;
  int nb_msgs       = 0;
  int timeout       = 5;
  bool top          = false;
  bool record       = false;
  bool only_once    = false;
  FAR char *filter  = NULL;
  int ret;
  int ch;

  g_should_exit = false;
  if (signal(SIGINT, exit_handler) == SIG_ERR)
    {
      return 1;
    }

  /* Pasrse Argument */

  while ((ch = getopt(argc, argv, "r:b:n:t:Tflh")) != EOF)
    {
      switch (ch)
      {
        case 'r':
          topic_rate = atof(optarg);
          if (topic_rate < 0)
            {
              goto error;
            }
          break;

        case 'b':
          topic_latency = strtol(optarg, NULL, 0);
          if (topic_latency < 0)
            {
              goto error;
            }
          break;

        case 'n':
          nb_msgs = strtol(optarg, NULL, 0);
          if (nb_msgs < 0)
            {
              goto error;
            }
          break;

        case 't':
          timeout = strtol(optarg, NULL, 0);
          if (timeout < 0)
            {
              goto error;
            }
          break;

#ifdef CONFIG_DEBUG_UORB
        case 'f':
          record = true;
          break;
#endif

        case 'T':
          top = true;
          break;

        case 'l':
          only_once = true;
          break;

        case 'h':
        default:
          goto error;
        }
    }

  if (optind < argc)
    {
      filter = argv[optind];
    }

  /* Alloc list and exec command */

  SLIST_INIT(&objlist);
  ret = listener_generate_object_list(&objlist, filter);
  if (ret <= 0)
    {
      return 0;
    }

  if (top)
    {
      listener_top(&objlist, filter, only_once);
    }
  else
    {
      uorbinfo_raw("\nMonitor objects num:%d", ret);
      SLIST_FOREACH(tmp, &objlist, node)
        {
          uorbinfo_raw("object_name:%s, object_instance:%d",
                       tmp->object.meta->o_name,
                       tmp->object.instance);
        }

      listener_monitor(&objlist, ret, topic_rate, topic_latency,
                       nb_msgs, timeout, record);
    }

  listener_delete_object_list(&objlist);
  return 0;

error:
  usage();
  return 1;
}
