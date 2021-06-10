/****************************************************************************
 * apps/system/mdio/mdio_main.c
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
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include <net/if.h>
#include <arpa/inet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_HELLO_IPADDR     0xc0a8eacd
#define CONFIG_HELLO_PORT       81

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Socket related variables */

static int g_listen_fd;

static struct sockaddr_in g_sa;

int initialize_socket(void)
{
  /* Prepare listen address */

  g_sa.sin_family      = AF_INET;
  g_sa.sin_port        = HTONS(CONFIG_HELLO_PORT);
  g_sa.sin_addr.s_addr = HTONL(CONFIG_HELLO_IPADDR);

  /* Open listening socket */

  g_listen_fd = socket(g_sa.sin_family, SOCK_DGRAM, 0);

  /* Verify socket creation */

  if (!(g_listen_fd > 0))
    {
      return -1;
    }

  return 0;
}

int get_phy_id(void)
{
  int ret;
  uint16_t phy_id;
  struct ifreq ifr;

  /* Prepare ifreq */

  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

  ret = ioctl(g_listen_fd, SIOCGMIIPHY, (unsigned long) &ifr);

  printf("call to %s() ", "ioctl");

  if (ret != 0)
    {
      printf("failed! (value %d, errno %d)\n", ret, errno);
      return -1;
    }

  phy_id = ifr.ifr_mii_phy_id;

  return phy_id;
}

int get_phy_reg(uint16_t phy_id, uint16_t reg_num, uint16_t *val)
{
  int ret;
  struct ifreq ifr;

  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

  ifr.ifr_mii_phy_id = phy_id;
  ifr.ifr_mii_reg_num = reg_num;

  ret = ioctl(g_listen_fd, SIOCGMIIREG, (unsigned long) &ifr);

  if (ret == OK)
    {
      *val = ifr.ifr_mii_val_out;
    }

  return ret;
}

int set_phy_reg(uint16_t phy_id, uint16_t reg_num, uint16_t val)
{
  int ret;
  struct ifreq ifr;

  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

  ifr.ifr_mii_phy_id = phy_id;
  ifr.ifr_mii_reg_num = reg_num;
  ifr.ifr_mii_val_in = val;

  ret = ioctl(g_listen_fd, SIOCSMIIREG, (unsigned long) &ifr);

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mdio_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  int i;

  /* test related */

  uint16_t phy_id;
  uint16_t reg_num;
  uint16_t val_in;
  uint16_t val_out;

  /* -- SETUP ------------------------------------------------------------- */

  if (argc == 1)
    {
      printf("usage:\n");
      printf("\n");
      printf("  %s phy_id reg_no          -- read register\n", argv[0]);
      printf("  %s phy_id reg_no value    -- write register\n", argv[0]);
      printf("\n");
      return -1;
    }

  initialize_socket();

  if (argc == 4) /* Write to register */
    {
      phy_id = strtol(argv[1], NULL, 16);
      reg_num = strtol(argv[2], NULL, 16);
      val_in = strtol(argv[3], NULL, 16);

      ret = set_phy_reg(phy_id, reg_num, val_in);
      if (ret != 0)
        {
          printf("%s() returned %d\n", "set_phy_reg", ret);
        }
    }
  else if (argc == 3) /* Read from register */
    {
      phy_id = strtol(argv[1], NULL, 16);
      reg_num = strtol(argv[2], NULL, 16);

      ret = get_phy_reg(phy_id, reg_num, &val_out);
      if (ret != 0)
        {
          printf("%s() returned %d\n", "get_phy_reg", ret);
        }
      else
        {
          printf("0x%4x\n", val_out);
        }
    }
  else if (argc == 2)
    {
      phy_id = strtol(argv[1], NULL, 16);

      for (i = 0; i < 32; i++)
        {
          get_phy_reg(phy_id, i, &val_out);
          printf("phy[%d][%d] = 0x%4x\n", phy_id, i, val_out);
        }
    }
  else
    {
      /* Read the PHY address */

      phy_id = get_phy_id();

      if (phy_id < 1)
        {
          /* failed (can not be negative) */

          printf("getting phy id failed\n");
        }
      else
        {
          printf("phy_id = %d\n", phy_id);
        }
    }

  return 0;
}
