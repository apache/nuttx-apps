/****************************************************************************
 * apps/testing/drivers/pcitest/pcitest.c
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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <nuttx/pci/pci_ep_test.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pci_test_s
{
  const char    *device;
  char          barnum;
  bool          legacyirq;
  unsigned int  msinum;
  unsigned int  msixnum;
  int           irqtype;
  bool          set_irqtype;
  bool          get_irqtype;
  bool          clear_irq;
  bool          read;
  bool          write;
  bool          copy;
  unsigned long size;
  bool          use_dma;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_result[] =
{
  "NOT OKAY",
  "OKAY"
};
static const char *g_irq[] =
{
  "LEGACY",
  "MSI",
  "MSI-X"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int run_test(const struct pci_test_s *test)
{
  struct pci_ep_test_param_s param;
  int ret = -EINVAL;
  int fd;

  memset(&param, 0, sizeof(param));
  fd = open(test->device, O_RDWR);
  if (fd < 0)
    {
      perror("can't open PCI Endpoint Test device");
      return -ENODEV;
    }

  if (test->barnum >= 0 && test->barnum <= 5)
    {
      ret = ioctl(fd, PCITEST_BAR, test->barnum);
      fprintf(stdout, "BAR%d:\t\t", test->barnum);
      if (ret < 0)
        {
          fprintf(stdout, "TEST FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->set_irqtype)
    {
      ret = ioctl(fd, PCITEST_SET_IRQTYPE, test->irqtype);
      fprintf(stdout, "SET IRQ TYPE TO %s:\t\t", g_irq[test->irqtype]);
      if (ret < 0)
        {
          fprintf(stdout, "FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->get_irqtype)
    {
      ret = ioctl(fd, PCITEST_GET_IRQTYPE);
      fprintf(stdout, "GET IRQ TYPE:\t\t");
      if (ret < 0)
        {
          fprintf(stdout, "FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_irq[ret]);
        }
    }

  if (test->clear_irq)
    {
      ret = ioctl(fd, PCITEST_CLEAR_IRQ);
      fprintf(stdout, "CLEAR IRQ:\t\t");
      if (ret < 0)
        {
          fprintf(stdout, "FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->legacyirq)
    {
      ret = ioctl(fd, PCITEST_LEGACY_IRQ, 0);
      fprintf(stdout, "LEGACY IRQ:\t");
      if (ret < 0)
        {
          fprintf(stdout, "TEST FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->msinum > 0 && test->msinum <= 32)
    {
      ret = ioctl(fd, PCITEST_MSI, test->msinum);
      fprintf(stdout, "MSI%d:\t\t", test->msinum);
      if (ret < 0)
        {
          fprintf(stdout, "TEST FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->msixnum > 0 && test->msixnum <= 2048)
    {
      ret = ioctl(fd, PCITEST_MSIX, test->msixnum);
      fprintf(stdout, "MSI-X%d:\t\t", test->msixnum);
      if (ret < 0)
        {
          fprintf(stdout, "TEST FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->write)
    {
      param.size = test->size;
      if (test->use_dma)
        {
          param.flags = PCITEST_FLAGS_USE_DMA;
        }

      ret = ioctl(fd, PCITEST_WRITE, &param);
      fprintf(stdout, "WRITE (%7ld bytes):\t\t", test->size);
      if (ret < 0)
        {
          fprintf(stdout, "TEST FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->read)
    {
      param.size = test->size;
      if (test->use_dma)
        {
          param.flags = PCITEST_FLAGS_USE_DMA;
        }

      ret = ioctl(fd, PCITEST_READ, &param);
      fprintf(stdout, "READ (%7ld bytes):\t\t", test->size);
      if (ret < 0)
        {
          fprintf(stdout, "TEST FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  if (test->copy)
    {
      param.size = test->size;
      if (test->use_dma)
        {
          param.flags = PCITEST_FLAGS_USE_DMA;
        }

      ret = ioctl(fd, PCITEST_COPY, &param);
      fprintf(stdout, "COPY (%7ld bytes):\t\t", test->size);
      if (ret < 0)
        {
          fprintf(stdout, "TEST FAILED\n");
        }
      else
        {
          fprintf(stdout, "%s\n", g_result[ret]);
        }
    }

  fflush(stdout);
  close(fd);

  return ret < 0 ? ret : 1 - ret; /* return 0 if test succeeded */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int c;
  struct pci_test_s *test;

  test = calloc(1, sizeof(*test));
  if (!test)
    {
      perror("Fail to allocate memory for pci_test\n");
      return -ENOMEM;
    }

  /* Since '0' is a valid BAR number, initialize it to -1 */

  test->barnum = -1;

  /* Set default size as 100KB */

  test->size = 0x19000;

  /* Set default endpoint device */

  test->device = "/dev/pci-ep-test.0";

  while ((c = getopt(argc, argv, "D:b:m:x:i:deIlhrwcs:")) != EOF)
    {
    switch (c)
      {
        case 'D':
          test->device = optarg;
          continue;
        case 'b':
          test->barnum = atoi(optarg);
          if (test->barnum < 0 || test->barnum > 5)
            {
              goto usage;
            }

          continue;
        case 'l':
          test->legacyirq = true;
          continue;
        case 'm':
          test->msinum = atoi(optarg);
          if (test->msinum < 1 || test->msinum > 32)
            {
              goto usage;
            }

          continue;
        case 'x':
          test->msixnum = atoi(optarg);
          if (test->msixnum < 1 || test->msixnum > 2048)
            {
              goto usage;
            }

          continue;
        case 'i':
          test->irqtype = atoi(optarg);
          if (test->irqtype < 0 || test->irqtype > 2)
            {
              goto usage;
            }

          test->set_irqtype = true;
          continue;
        case 'I':
          test->get_irqtype = true;
          continue;
        case 'r':
          test->read = true;
          continue;
        case 'w':
          test->write = true;
          continue;
        case 'c':
          test->copy = true;
          continue;
        case 'e':
          test->clear_irq = true;
          continue;
        case 's':
          test->size = strtoul(optarg, NULL, 0);
          continue;
        case 'd':
          test->use_dma = true;
          continue;
        case 'h':
        default:
usage:
          fprintf(stderr,
            "usage: %s [options]\n"
            "Options:\n"
            "\t-D <dev> \n"
            "\t    PCI e test device {default: /dev/pci-ep-test.0}\n"
            "\t-b <bar num>     BAR test (bar number between 0..5)\n"
            "\t-m <msi num>     MSI test (msi number between 1..32)\n"
            "\t-x <msix num>    \tMSI-X test (msix number between 1..2048)\n"
            "\t-i <irq type>  \n"
            "\t    Set IRQ type (0 - Legacy, 1 - MSI, 2 - MSI-X)\n"
            "\t-e           Clear IRQ\n"
            "\t-I           Get current IRQ type configured\n"
            "\t-d           Use DMA\n"
            "\t-l           Legacy IRQ test\n"
            "\t-r           Read buffer test\n"
            "\t-w           Write buffer test\n"
            "\t-c           Copy buffer test\n"
            "\t-s <size>        Size of buffer {default: 100KB}\n"
            "\t-h           Print this help message\n",
            argv[0]);
            return -EINVAL;
      }
    }

  return run_test(test);
}
