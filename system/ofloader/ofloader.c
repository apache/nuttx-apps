/****************************************************************************
 * apps/system/ofloader/ofloader.c
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

#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <mqueue.h>
#include <sys/ioctl.h>
#include <sys/boardctl.h>

#include "ofloader.h"

/****************************************************************************
 * Private Typess
 ****************************************************************************/

struct devinfo_s
{
  int fd;
  off_t pos;
  off_t base;
  size_t size;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int get_partinfo(FAR struct devinfo_s *devinfo)
{
  struct partition_info_s info;
  int ret;

  ret = ioctl(devinfo->fd, BIOC_PARTINFO, &info);
  if (ret < 0)
    {
      return -errno;
    }

  devinfo->base = info.sectorsize * info.startsector;
  devinfo->size = info.sectorsize * info.numsectors;
  return 0;
}

static void free_devinfo(FAR struct devinfo_s *devinfo, size_t count)
{
  if (devinfo == NULL || count == 0)
    {
      return;
    }

  while (count-- != 0)
    {
      if (devinfo[count].fd > 0)
        {
          close(devinfo[count].fd);
        }
    }

  free(devinfo);
}

static FAR struct devinfo_s *parse_devinfo(FAR size_t *index)
{
  char table[] = CONFIG_SYSTEM_OFLOADER_TABLE;
  FAR struct devinfo_s *devinfo = NULL;
  FAR struct devinfo_s *tmp;
  FAR char *save_once;
  FAR char *save;
  FAR char *p;

  *index = 0;
  p = strtok_r(table, ";", &save_once);
  while (p != NULL)
    {
      p = strtok_r(p, ",", &save);
      if (p == NULL)
        {
          goto out;
        }

      tmp = realloc(devinfo, (*index + 1) * sizeof(struct devinfo_s));
      if (tmp == NULL)
        {
          goto out;
        }

      devinfo = tmp;
      devinfo[*index].fd = open(p, O_RDWR);
      if (devinfo[*index].fd < 0)
        {
          goto out;
        }

      p = strtok_r(NULL, ",", &save);
      devinfo[*index].base = strtoul(p, NULL, 0);
      if (devinfo[*index].base == 0)
        {
          if (get_partinfo(&devinfo[*index]) < 0)
            {
              (*index)++;
              goto out;
            }

          goto again;
        }

      p = strtok_r(NULL, ",", &save);
      if (p == NULL)
        {
          (*index)++;
          goto out;
        }

      devinfo[*index].size = strtoul(p, NULL, 0);
      if (devinfo[*index].size == 0)
        {
          (*index)++;
          goto out;
        }

again:
      devinfo[*index].pos = devinfo[*index].base;
      p = strtok_r(NULL, ";", &save_once);
      (*index)++;
    }

  return devinfo;

out:
  free_devinfo(devinfo, *index);
  return NULL;
}

static ssize_t read_safe(int fd, FAR uint8_t *buff, size_t size)
{
  ssize_t i = 0;

  while (i < size)
    {
      ssize_t ret = read(fd, buff + i, size - i);
      if (ret < 0)
        {
          return -errno;
        }

      i += ret;
    }

  return i;
}

static ssize_t write_safe(int fd, FAR uint8_t *buff, size_t size)
{
  ssize_t i = 0;

  while (i < size)
    {
      ssize_t ret = write(fd, buff + i, size - i);
      if (ret < 0)
        {
          return -errno;
        }

      i += ret;
    }

  return i;
}

static ssize_t handle(FAR struct ofloader_msg *msg,
                      FAR struct devinfo_s *devinfo, size_t count)
{
  FAR uint8_t *buff = NULL;
  ssize_t ret = 0;
  size_t index;

  for (index = 0; index < count; index++)
    {
      if (msg->addr >= devinfo[index].base &&
          msg->addr < devinfo[index].base + devinfo[index].size)
        {
          break;
        }
    }

  if (index == count)
    {
      return -ENODEV;
    }

  if (devinfo[index].pos != msg->addr)
    {
      off_t off = msg->addr - devinfo[index].pos;

      off = lseek(devinfo[index].fd, off, SEEK_CUR);
      if (off < 0)
        {
          OFLOADER_DEBUG("lseek error\n");
          return -errno;
        }

      devinfo[index].pos = msg->addr;
    }

  OFLOADER_DEBUG("index %zu pos %" PRIxOFF "\n", index, devinfo[index].pos);
  OFLOADER_DEBUG("atcion %d addr %" PRIxOFF "size %zu\n",
                 msg->atcion, msg->addr, msg->size);
  switch (msg->atcion)
    {
      case OFLOADER_SYNC:
        ret = fsync(devinfo[index].fd);
        if (ret < 0)
          {
            ret = -errno;
          }

        break;
      case OFLOADER_VERIFY:
        buff = malloc(msg->size);
        if (buff == NULL)
          {
            OFLOADER_DEBUG("verify no mem\n");
            return -ENOMEM;
          }

        ret = read_safe(devinfo[index].fd, buff, msg->size);
        if (ret < 0)
          {
            OFLOADER_DEBUG("verify read error\n");
            free(buff);
            break;
          }

        devinfo[index].pos += ret;
        if (memcmp(msg->buff, buff, ret) != 0)
          {
            OFLOADER_DEBUG("verify error\n");
            ret = -EIO;
            free(buff);
            break;
          }

        free(buff);
        break;
      case OFLOADER_READ:
        ret = read_safe(devinfo[index].fd, msg->buff, msg->size);
        if (ret < 0)
          {
            OFLOADER_DEBUG("read error\n");
            break;
          }

        devinfo[index].pos += ret;
        break;
      case OFLOADER_WRITE:
        ret = write_safe(devinfo[index].fd, msg->buff, msg->size);
        if (ret < 0)
          {
            OFLOADER_DEBUG("write error\n");
            break;
          }

        devinfo[index].pos += ret;
        break;
    }

  return ret;
}

static pthread_addr_t fake_idle(pthread_addr_t arg)
{
  for (; ; )
    {
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ofl_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct devinfo_s *devinfo;
  FAR struct ofloader_msg *msg;
  struct mq_attr mqattr;
  pthread_attr_t attr;
  pthread_t thread;
  size_t count;
  size_t i;
  mqd_t mq;

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#  ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#  endif
#endif

  memset(&mqattr, 0, sizeof(struct mq_attr));
  mqattr.mq_msgsize = sizeof(msg);
  mqattr.mq_maxmsg  = 10;

  if (g_create_idle)
    {
      pthread_attr_init(&attr);
      attr.priority = 1;
      pthread_create(&thread, &attr, fake_idle, NULL);
      pthread_attr_destroy(&attr);
    }

  mq = mq_open(OFLOADER_QNAME, O_CREAT | O_RDWR, 0660, &mqattr);
  if (mq < 0)
    {
      OFLOADER_DEBUG("mq_open error:%d\n", errno);
      return -errno;
    }

  devinfo = parse_devinfo(&count);
  if (devinfo == NULL)
    {
      OFLOADER_DEBUG("parsdevinfo error\n");
      mq_close(mq);
      return -EINVAL;
    }

  for (i = 0; i < count; i++)
    {
      OFLOADER_DEBUG("seq %d, base %" PRIxOFF ", size %zx\n",
                     i, devinfo[i].base, devinfo[i].size);
    }

  while (1)
    {
      if (mq_receive(mq, (FAR char *)&msg, sizeof(msg), NULL) < 0)
        {
          OFLOADER_DEBUG(" mq_receive error %d\n", -errno);
          continue;
        }

      if (handle(msg, devinfo, count) < 0)
        {
          msg->atcion = OFLOADER_ERROR;
        }
      else
        {
          msg->atcion = OFLOADER_FINSH;
        }
    }

  /* Never run to here */

  free_devinfo(devinfo, count);
  mq_close(mq);
  return 0;
}
