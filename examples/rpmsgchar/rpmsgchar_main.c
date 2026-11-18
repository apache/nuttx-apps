/****************************************************************************
 * apps/examples/rpmsgchar/rpmsgchar_main.c
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

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/rpmsg/rpmsg.h>

/****************************************************************************
 * Private types
 ****************************************************************************/

struct rpmsg_char_arg_s
{
  const char *type;
  const char *ctrl;
  const char *name;
  uint32_t    src;
  uint32_t    dst;

  /* Private use */

  int         ctrl_fd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void rpmsg_char_usage(const char *progname)
{
  printf("Usage: %s [-t <type>] [-c <ctrl>] [-n <name>] [-s <src>] "
         "[-d <dst>]\n"
         "  type: the test type, now we support: echo, destroy\n"
         "  ctrl: the rpmsg ctrl char device name, e.g, /dev/rpmsg/ap\n"
         "  name: the rpmsg char device name you want to create, will\n"
         "        append \"rpmsg-raw-\" or \"rpmsg-\", e.g, if set\n"
         "        to \"test\", the rpmsg char device name will be\n"
         "        /dev/rpmsgX (Linux) or /dev/rpmsg-test (NuttX)\n"
         "  src : the rpmsg char device source address, default is ANY\n"
         "  dst : the rpmsg char device destination address, default is\n"
         "        ANY\n",
         progname);
}

static int rpmsg_char_arg_prase(struct rpmsg_char_arg_s *args,
                                int argc, char *argv[])
{
  int opt;

  /* Set the default value */

  memset(args, 0, sizeof(*args));
  args->src  = RPMSG_ADDR_ANY;
  args->dst  = RPMSG_ADDR_ANY;
  args->type = "echo";

  while ((opt = getopt(argc, argv, "h:t:c:n:s:d")) != -1)
    {
      switch (opt)
        {
          case 'h':
            rpmsg_char_usage(argv[0]);
            return 0;
          case 't':
            args->type = optarg;
            break;
          case 'c':
            args->ctrl = optarg;
            break;
          case 'n':
            args->name = optarg;
            break;
          case 's':
            args->src = strtoul(optarg, NULL, 0);
            break;
          case 'd':
            args->dst = strtoul(optarg, NULL, 0);
            break;
          default:
            printf("ERROR: arguments are error, print usage:\n");
            rpmsg_char_usage(argv[0]);
            return ERROR;
        }
    }

  if (!((args->src == RPMSG_ADDR_ANY && args->dst == RPMSG_ADDR_ANY) ||
        (args->src != RPMSG_ADDR_ANY && args->dst != RPMSG_ADDR_ANY)))
    {
      printf("Invalid src:0x%" PRIx32 " dst:0x%" PRIx32 " name: %s\n",
             args->src, args->dst, args->name);
      return ERROR;
    }

  return 0;
}

static ssize_t rpmsg_char_echo_test(struct rpmsg_char_arg_s *args)
{
  struct rpmsg_endpoint_info eptinfo;
  char devname[32];
  char buf[12];
  ssize_t ret;
  int count;
  int fd;

  /* Create rpmsg char device */

  eptinfo.src = args->src;
  eptinfo.dst = args->dst;
  strlcpy(eptinfo.name, args->name, sizeof(eptinfo.name));
  if (args->src == RPMSG_ADDR_ANY && args->dst == RPMSG_ADDR_ANY)
    {
      /* Should send the NS message to peer */

      ret = ioctl(args->ctrl_fd, RPMSG_CREATE_DEV_IOCTL, &eptinfo);
    }
  else /* args->src != RPMSG_ADDR_ANY && args->dst != RPMSG_ADDR_ANY */
    {
      ret = ioctl(args->ctrl_fd, RPMSG_CREATE_EPT_IOCTL, &eptinfo);
    }

  assert(ret >= 0);

  snprintf(devname, sizeof(devname), "/dev/rpmsg-%s", args->name);
  fd = open(devname, O_RDWR);
  assert(fd >= 0);

  printf("Start the echo test\n");
  for (count = 0; count < 1000; count++)
    {
      ret = write(fd, "Hello World\n", 12);
      assert(ret == 12);

      ret = read(fd, buf, 12);
      assert(ret == 12);
      assert(memcmp(buf, "Hello World\n", 12) == 0);
    }

  printf("Finish the echo test\n");

  /* Destroy the rpmsg char device */

  if (args->src == RPMSG_ADDR_ANY && args->dst == RPMSG_ADDR_ANY)
    {
      ret = ioctl(args->ctrl_fd, RPMSG_RELEASE_DEV_IOCTL, &eptinfo);
    }
  else /* args->src != RPMSG_ADDR_ANY && args->dst != RPMSG_ADDR_ANY */
    {
      ret = ioctl(fd, RPMSG_DESTROY_EPT_IOCTL, 0);
    }

  assert(ret >= 0);
  close(fd);
  return OK;
}

static ssize_t rpmsg_char_destroy_test(struct rpmsg_char_arg_s *args)
{
  struct rpmsg_endpoint_info eptinfo;
  char devname[32];
  ssize_t ret;
  char buf;
  int fd;
  int i;

  printf("Start the destroy test\n");
  eptinfo.src = RPMSG_ADDR_ANY;
  eptinfo.dst = RPMSG_ADDR_ANY;
  for (i = 0; i < 100; i++)
    {
      snprintf(eptinfo.name, sizeof(eptinfo.name), "%s%d", args->name, i);
      ret = ioctl(args->ctrl_fd, RPMSG_CREATE_EPT_IOCTL, &eptinfo);
      if (ret < 0)
        {
          printf("Failed to create endpoint i=%d %d\n", i, errno);
          return ret;
        }
    }

  for (i = 0; i < 100; i++)
    {
      snprintf(devname, sizeof(devname), "/dev/rpmsg-%s%d", args->name, i);
      fd = open(devname, O_RDWR);
      assert(fd >= 0);

      buf = 0xaa;
      ret = write(fd, &buf, 1);
      assert(ret == 1);

      ret = read(fd, &buf, 1);
      assert(ret == 1 && buf == 0xaa);

      ret = ioctl(fd, RPMSG_DESTROY_EPT_IOCTL, 0);
      assert(ret >= 0);

      close(fd);
    }

  printf("End the destroy test\n");
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct rpmsg_char_arg_s args;
  int ret;

  if (rpmsg_char_arg_prase(&args, argc, argv) < 0)
    {
      return ERROR;
    }

  args.ctrl_fd = open(args.ctrl, O_RDWR);
  if (args.ctrl_fd < 0)
    {
      printf("Failed to open %s %d\n", args.ctrl, errno);
      return ERROR;
    }

  if (strcmp(args.type, "echo") == 0)
    {
      ret = rpmsg_char_echo_test(&args);
    }
  else if (strcmp(args.type, "destroy") == 0)
    {
      ret = rpmsg_char_destroy_test(&args);
    }
  else
    {
      printf("Invalid test type %s\n", args.type);
      rpmsg_char_usage(argv[0]);
      ret = ERROR;
    }

  close(args.ctrl_fd);
  return ret;
}
