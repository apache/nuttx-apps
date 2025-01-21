/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_i2c_read.c
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

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GET_I2C_SLAVE_ADDR 0x8
#define GET_I2C_FRE 100000
#define GET_I2C_SLAVE_PATH "/dev/i2cslv0"
#define GET_I2C_MASTER_PATH "/dev/i2c0"
#define SIZE_OF_BUFFER 4
#define COUNT_OF_TEST 100
#define I2C_PATH_MAX 512
#ifndef CONFIG_I2C_SLAVE_WRITEBUFSIZE
#  define GET_I2C_WRITEBUFSIZE SIZE_OF_BUFFER
#else
#  define GET_I2C_WRITEBUFSIZE CONFIG_I2C_SLAVE_WRITEBUFSIZE
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct i2c_state_s
{
  char pathname_master[I2C_PATH_MAX];
  char pathname_slave[I2C_PATH_MAX];
  uint16_t addr;
  uint32_t frequency;
  int master_fd;
  int slave_fd;
};

static struct i2c_state_s g_i2read =
{
  .pathname_master = GET_I2C_MASTER_PATH,
  .pathname_slave = GET_I2C_SLAVE_PATH,
  .addr = GET_I2C_SLAVE_ADDR,
  .frequency = GET_I2C_FRE,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2c_information
 ****************************************************************************/

static void i2c_information(FAR struct i2c_state_s *i2c_state)
{
  printf("  [masterpath] selects the I2C Master device.\n"
         "  Default: %s Current: %s\n",
         GET_I2C_MASTER_PATH, i2c_state->pathname_master);
  printf("  [slavepath] selects the I2C Slave device.\n"
         "  Default: %s Current: %s\n",
         GET_I2C_SLAVE_PATH, i2c_state->pathname_slave);
  printf("  [frequency] selects the I2C frequency.\n"
         "  Default: %d Current: %ld\n",
         GET_I2C_FRE, i2c_state->frequency);
  printf("  [slaveAddress] the address of i2c Slave, Convert to decimal.\n"
         "  Default: %d Current: %d\n",
         GET_I2C_SLAVE_ADDR, i2c_state->addr);
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname,
                       FAR struct i2c_state_s *i2c_state, int exitcode)
{
  printf("Usage: %s"
         "<masterpath> <slavepath> <frequency> <slaveAddress> \n", progname);
  i2c_information(i2c_state);
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(FAR struct i2c_state_s *i2c_state,
                              int argc, FAR char **argv)
{
  int converted;
  int ch = 1;

  while (ch < argc)
    {
      switch (ch)
        {
          case 1:
            strlcpy(i2c_state->pathname_master, argv[1],
                    sizeof(i2c_state->pathname_master));
            break;

          case 2:
            strlcpy(i2c_state->pathname_slave, argv[2],
                    sizeof(i2c_state->pathname_slave));
            break;

          case 3:
            converted = atoi(argv[3]);
            if (converted < 1 || converted > INT_MAX)
              {
                printf("signal out of range: %d\n", converted);
                show_usage(argv[3], i2c_state, EXIT_FAILURE);
              }

            i2c_state->frequency = (uint32_t)converted;
            break;

          case 4:
          converted = atoi(argv[4]);
          if (converted < 1 || converted > INT_MAX)
            {
              printf("signal out of range: %d\n", converted);
              show_usage(argv[4], i2c_state, EXIT_FAILURE);
            }

          i2c_state->addr = (uint16_t)converted;
          break;

          default:
            printf("input error parameter\n");
            break;
        }

      ch += 1;
    }
}

/****************************************************************************
 * master read
 ****************************************************************************/

static int master_read_i2c(int fd)
{
  struct i2c_transfer_s i2c_transfer;
  uint8_t buffer[SIZE_OF_BUFFER];
  struct i2c_msg_s i2c_msg[1];
  int ret;
  int i;

  if (SIZE_OF_BUFFER > GET_I2C_WRITEBUFSIZE)
    {
      printf("test buffer size %d bigger than slave_writebufsize %d\n", \
      SIZE_OF_BUFFER, GET_I2C_WRITEBUFSIZE);
      return -1;
    }

  for (i = 0; i < SIZE_OF_BUFFER; i++)
    {
      buffer[i] = 0;
    }

  i2c_msg[0].addr = g_i2read.addr;
  i2c_msg[0].flags = 1;
  i2c_msg[0].buffer = buffer;
  i2c_msg[0].length = SIZE_OF_BUFFER;
  i2c_msg[0].frequency = g_i2read.frequency;

  i2c_transfer.msgv = (struct i2c_msg_s *)i2c_msg;
  i2c_transfer.msgc = 1;

  ret = ioctl(fd, I2CIOC_TRANSFER, (unsigned long)&i2c_transfer);
  if (ret < 0)
    {
      printf("read_i2c failed\n");
    }

  printf("master read \t");
  for (i = 0; i < SIZE_OF_BUFFER; i++)
    {
      printf("%d\t", buffer[i]);
    }

  printf("\n");
  return ret;
}

/****************************************************************************
 * master read iic
 ****************************************************************************/

static FAR void *master_read_thread(FAR void *arg)
{
  int cnt = 0;

  while (cnt <= COUNT_OF_TEST)
    {
      sleep(1);
      master_read_i2c(g_i2read.master_fd);
      cnt += 1;
    }

  pthread_exit(NULL);
}

/****************************************************************************
 * slave poll write
 ****************************************************************************/

static FAR void *slave_write_thread(FAR void *arg)
{
  char buffer[SIZE_OF_BUFFER];
  struct pollfd rfds;
  int icnt = 0;
  int num = 0;
  int ret;
  int i;

  rfds.fd = g_i2read.slave_fd;

  if (SIZE_OF_BUFFER > GET_I2C_WRITEBUFSIZE)
    {
      printf("test buffer size %d bigger than slave_writebufsize %d\n", \
      SIZE_OF_BUFFER, GET_I2C_WRITEBUFSIZE);
      return NULL;
    }

  while (icnt <= COUNT_OF_TEST)
    {
      for (i = 0; i < SIZE_OF_BUFFER; i++)
        {
          buffer[i] = i + num;
        }

      rfds.events = POLLOUT;
      rfds.revents = 0;

      ret = write(rfds.fd, buffer, SIZE_OF_BUFFER);
      if (ret < 0)
        {
          printf("write failed\n");
        }

      while (true)
        {
          int n = poll(&rfds, 1, -1);
          if (n == 0)
            {
              printf("time out\n");
              continue;
            }

          if (n < 0)
            {
              printf("poll err n= %d\n", n);
              continue;
            }

          if (rfds.revents == POLLOUT)
            {
              printf("slave write success \t");
              for (i = 0; i < SIZE_OF_BUFFER; i++)
                {
                  printf("%d\t", buffer[i]);
                }

              printf("\n");
              break;
            }
        }

      num++;
      icnt += 1;
    }

  pthread_exit(NULL);
}

/****************************************************************************
 * test_i2c_read_main
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  pthread_t t_id1;
  pthread_t t_id2;
  if (argc != 5)
    {
      printf("Error input\n");
      printf("Command: cmocka_driver_i2c_slave \
             <masterpath> <slavepath> <frequency> <slaveAddress> \n");
      return -1;
    }

  parse_commandline(&g_i2read, argc, argv);
  i2c_information(&g_i2read);

  g_i2read.master_fd = open(g_i2read.pathname_master, O_RDWR);
  if (g_i2read.master_fd < 0)
    {
      printf("open iic master failed\n");
      return -1;
    }

  g_i2read.slave_fd = open(g_i2read.pathname_slave, O_RDWR);
  if (g_i2read.slave_fd < 0)
    {
      close(g_i2read.master_fd);
      printf("open iic slave failed\n");
      return -1;
    }

  if (pthread_create(&t_id1, NULL, master_read_thread, NULL) < 0)
    {
      close(g_i2read.master_fd);
      close(g_i2read.slave_fd);
      printf("master_read_thread create failed\n");
      return -1;
    }

  if (pthread_create(&t_id2, NULL, slave_write_thread, NULL) < 0)
    {
      close(g_i2read.master_fd);
      close(g_i2read.slave_fd);
      printf("slave_write_thread create failed\n");
      return -1;
    }

  pthread_join(t_id1, NULL);
  pthread_join(t_id2, NULL);

  close(g_i2read.master_fd);
  close(g_i2read.slave_fd);

  printf("i2c read test end\n");
  return 0;
}
