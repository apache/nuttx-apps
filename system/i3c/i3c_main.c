/****************************************************************************
 * apps/system/i3c/i3c_main.c
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
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <nuttx/i3c/i3c_driver.h>
#include <nuttx/i3c/device.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Device naming */

#define DEVNAME_FMT    "/dev/i3c%d"
#define DEVNAME_FMTLEN (8 + 3 + 1)

/****************************************************************************
 * Private Type
 ****************************************************************************/

struct i3c_ioc_priv_xfer
{
  uint8_t rnw;       /* encodes the transfer direction. true for a read, false for a write */
  uint16_t len;      /* Length of data buffer buffers, in bytes */
  FAR uint8_t *data; /*  Holds pointer to userspace buffer with transmit data */
  uint8_t pad[5];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR const char *g_sopts = "b:m:p:r:w:h:g";
static const struct option g_lopts[] =
{
  {"bus",     required_argument,  NULL, 'b' },
  {"manufid", required_argument,  NULL, 'm' },
  {"partid",  required_argument,  NULL, 'p' },
  {"read",    required_argument,  NULL, 'r' },
  {"write",   required_argument,  NULL, 'w' },
  {"get",     required_argument,  NULL, 'g' },
  {"command", required_argument,  NULL, 'c' },
  {"help",    no_argument,        NULL, 'h' },
  {0, 0, 0, 0}
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: print_usage
 ****************************************************************************/

static void print_usage(FAR const char *name)
{
  fprintf(stdout, "usage: %s options...\n", name);
  fprintf(stdout, "  options:\n");
  fprintf(stdout, "  -b --bus     <bus>          bus to use.\n");
  fprintf(stdout, "  -m --manufid <manufid>      manufacturer ID "
                  "(upper 16 bits of PID).\n");
  fprintf(stdout, "  -p --partid  <partid>       part ID "
                  "(lower 32 bits of PID).\n");
  fprintf(stdout, "  -r --read    <data length>  read data.\n");
  fprintf(stdout, "  -w --write   <data block>   Write data block.\n");
  fprintf(stdout, "  -g --get     <data block>   get a dev info.\n");
  fprintf(stdout, "  -h --help    Output usage message and exit.\n");
}

/****************************************************************************
 * Name: rx_args_to_xfer
 ****************************************************************************/

static int rx_args_to_xfer(int length, int rnw, FAR uint8_t **data,
                           FAR char *arg)
{
  int32_t len = strtol(arg, NULL, 0);
  FAR uint8_t *tmp;

  tmp = calloc(len, sizeof(uint8_t));
  if (!tmp)
    {
      return -ENOMEM;
    }

  rnw = 1;
  length = len;
  *data = tmp;

  return 0;
}

/****************************************************************************
 * Name: w_args_to_xfer
 ****************************************************************************/

static int w_args_to_xfer(uint8_t length, FAR uint8_t **data, FAR char *arg)
{
  FAR char *data_ptrs[256];
  FAR uint8_t *tmp;
  int i = 0;
  int len;

  data_ptrs[i] = strtok(arg, ",");
  while (data_ptrs[i] && i < 255)
    {
      data_ptrs[++i] = strtok(NULL, ",");
    }

  tmp = calloc(i, sizeof(uint8_t));
  if (!tmp)
    {
      return -ENOMEM;
    }

  for (len = 0; len < i; len++)
    {
      tmp[len] = (uint8_t)strtol(data_ptrs[len], NULL, 0);
    }

  length = len;
  *data = tmp;

  return 0;
}

/****************************************************************************
 * Name: print_rx_data
 ****************************************************************************/

static void print_rx_data(FAR struct i3c_ioc_priv_xfer *xfer)
{
  FAR uint8_t *tmp;
  uint32_t i;

  tmp = calloc(xfer->len, sizeof(uint8_t));
  if (!tmp)
    {
      return;
    }

  memcpy(tmp, (FAR void *)(uintptr_t)xfer->data,
         xfer->len * sizeof(uint8_t));

  fprintf(stdout, "  received data:\n");
  for (i = 0; i < xfer->len; i++)
    {
      fprintf(stdout, "    0x%02x\n", tmp[i]);
    }

  free(tmp);
}

/****************************************************************************
 * Name: i3c_transfers
 ****************************************************************************/

static int i3c_transfers(int argc, FAR char *argv[], const int fd,
                         uint16_t manufid, uint16_t partid, int nxfers)
{
  FAR struct i3c_ioc_priv_xfer *xfers;
  struct i3c_transfer_s transfers;
  int ret = 0;
  int opt;
  int i;

  if (nxfers <= 0)
    {
      return EXIT_FAILURE;
    }

  transfers.manufid = manufid;
  transfers.partid = partid;

  /* init receive or send works */

  xfers = calloc(nxfers, sizeof(*xfers));
  if (!xfers)
    {
      return -ENOMEM;
    }

  optind = 1;
  while ((opt = getopt_long(argc, argv, g_sopts, g_lopts, NULL)) != EOF)
    {
      switch (opt)
        {
          case 'h':
          case 'm':
          case 'p':
          case 'b':
            break;
          case 'r':
            if (rx_args_to_xfer(xfers->len, xfers->rnw, &xfers->data,
                                optarg))
              {
                ret = EXIT_FAILURE;
                goto err_free;
              }

            break;
          case 'w':
            if (w_args_to_xfer(xfers->len, &xfers->data, optarg))
              {
                ret = EXIT_FAILURE;
                goto err_free;
              }

            break;
        }
    }

  transfers.nxfers = nxfers;
  transfers.xfers = (FAR struct i3c_priv_xfer *)xfers;

  if (ioctl(fd, I3CIOC_PRIV_XFERS, transfers) < 0)
    {
      fprintf(stdout, "Error: transfer failed!\n");
      ret = EXIT_FAILURE;
      goto err_free;
    }

  /* printf the received data */

  for (i = 0; i < nxfers; i++)
    {
      fprintf(stdout, "Success on message %d\n", i);
      if (xfers[i].rnw)
        {
          print_rx_data(&xfers[i]);
        }
    }

  ret = EXIT_SUCCESS;

err_free:
  for (i = 0; i < nxfers; i++)
    {
      free(xfers[i].data);
    }

  free(xfers);
  return ret;
}

/****************************************************************************
 * Name: i3c_get_devinfo
 ****************************************************************************/

static int i3c_get_devinfo(int argc, FAR char *argv[], int fd,
                           uint16_t manufid, uint16_t partid)
{
  struct i3c_transfer_s transfers;
  struct i3c_device_info dev_info;

  transfers.manufid = manufid;
  transfers.partid = partid;
  transfers.info = &dev_info;

  if (ioctl(fd, I3CIOC_GET_DEVINFO, transfers) < 0)
    {
      fprintf(stderr, "Error: transfer failed: %s\n", strerror(errno));
      return EXIT_FAILURE;
    }

  printf("i3c_device_info - pid % "PRIi64" \n", dev_info.pid);
  printf("i3c_device_info - bcr %d\n", dev_info.bcr);
  printf("i3c_device_info - dcr %d\n", dev_info.dcr);
  printf("i3c_device_info - static_addr %d\n", dev_info.static_addr);
  printf("i3c_device_info - dyn_addr %d\n", dev_info.dyn_addr);
  printf("i3c_device_info - hdr_cap %d\n", dev_info.hdr_cap);
  printf("i3c_device_info - max_read_ds %d\n", dev_info.max_read_ds);
  printf("i3c_device_info - max_write_ds %d\n", dev_info.max_write_ds);
  printf("i3c_device_info - max_ibi_len %d\n", dev_info.max_ibi_len);
  printf("i3c_device_info - max_read_turnaround %d\n",
         dev_info.max_read_turnaround);
  printf("i3c_device_info - max_read_len %d\n", dev_info.max_read_len);
  printf("i3c_device_info - max_write_len %d\n", dev_info.max_write_len);

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uint16_t manufid = 0;
  uint16_t partid = 0;
  int bus_num = CONFIG_I3CTOOL_DEFBUS;
  char devname[DEVNAME_FMTLEN];
  int ret = EXIT_FAILURE;
  int nxfers = 0;
  int action;
  int opt;
  int fd;

  if (argc < 2 || (argv[1][0] != '-') || (argv[1][0] == '-' && !argv[1][1]))
    {
      print_usage(argv[0]);
      return ret;
    }

  while ((opt = getopt_long(argc, argv, g_sopts, g_lopts, NULL)) != EOF)
    {
      switch (opt)
        {
          case 'b':
            {
              bus_num = strtol(optarg, NULL, 0);
              break;
            }

          case 'm':
            {
              manufid = (uint16_t)strtol(optarg, NULL, 0);
              break;
            }

          case 'p':
            {
              partid = (uint16_t)strtol(optarg, NULL, 0);
              break;
            }

          case 'r':
          case 'w':
            {
              action = opt;
              nxfers++;
              break;
            }

          case 'g':
            {
              action = opt;
              break;
            }

          default:
            {
              print_usage(argv[0]);
              return ret;
            }
        }
    }

  /* bus number and target device info */

  fprintf(stdout, "bus_num is 0x%x, manufid is 0x%04x, partid is 0x%04x\n",
          bus_num, manufid, partid);

  /* i3c driver node to open */

  memset(devname, 0, DEVNAME_FMTLEN);
  snprintf(devname, DEVNAME_FMTLEN, DEVNAME_FMT, bus_num);
  fd = open(devname, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stdout, "open i3c driver %s failed\n", devname);
      return fd;
    }

  fprintf(stdout, "begin to parser test data.\n");

  switch (action)
    {
      case 'r':
      case 'w':
        fprintf(stdout, "opt - w/r, nxfers %d\n", nxfers);
        ret = i3c_transfers(argc, argv, fd, manufid, partid, nxfers);
        break;
      case 'g':
        fprintf(stdout, "opt - g\n");
        ret = i3c_get_devinfo(argc, argv, fd, manufid, partid);
        break;
      default:
        print_usage(argv[0]);
    }

  close(fd);
  return ret;
}
