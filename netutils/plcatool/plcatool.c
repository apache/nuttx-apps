/****************************************************************************
 * apps/netutils/plcatool/plcatool.c
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <sys/ioctl.h>

#include "oa_tc14.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ARGS_REMAIN(argc, pos) ((argc) - (pos))

#define PLCATOOL_CFG_SET_BIT 0x8000
#define PLCATOOL_CFG_SET(cfg, field, val) \
    do {(cfg)->field = PLCATOOL_CFG_SET_BIT | ((val) & 0xff);} while (0)

#define PLCATOOL_CFG_VAL(cfg, field)    ((cfg)->field & 0xff)

#define NODE_ID_MIN   0
#define NODE_ID_MAX   255

#define NODE_CNT_MIN  1
#define NODE_CNT_MAX  255

#define TO_TMR_MIN    0
#define TO_TMR_MAX    255

#define BURST_CNT_MIN 0
#define BURST_CNT_MAX 255

#define BURST_TMR_MIN 0
#define BURST_TMR_MAX 255

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum plcatool_cmd_e
{
  PLCA_CMD_SET,
  PLCA_CMD_GET,
  PLCA_CMD_STATUS,
  PLCA_CMD_HELP
};

/* Lower 8 bits are value, the most significant bit indicates whether set */

struct plcatool_cfg_s
{
  enum plcatool_cmd_e cmd;

  FAR char *ifname;

  uint8_t phy;

  uint16_t enable;
  uint16_t node_cnt;
  uint16_t node_id;
  uint16_t to_tmr;
  uint16_t burst_cnt;
  uint16_t burst_tmr;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int get_num(FAR const char *str, FAR int *result);
static int parse_args(int argc, FAR char *argv[],
                      FAR struct plcatool_cfg_s *cfg);
static int write_plca_mmd(FAR const char *ifname, int phy,
                          uint16_t address, uint16_t data);
static int read_plca_mmd(FAR const char *ifname, int phy,
                         uint16_t address, FAR uint16_t *data);
static int verify_support(FAR struct plcatool_cfg_s *cfg,
                          FAR bool *supported);
static int plcatool_set(FAR struct plcatool_cfg_s *cfg);
static int plcatool_get(FAR struct plcatool_cfg_s *cfg);
static int plcatool_status(FAR struct plcatool_cfg_s *cfg);
static void plcatool_usage(bool err);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int get_num(FAR const char *str, FAR int *result)
{
  FAR char *endptr;
  *result = (int)strtol(str, &endptr, 0);

  if (*endptr != '\0')
    {
      return ERROR;
    }

  return OK;
}

static int parse_args(int argc, FAR char *argv[],
                      FAR struct plcatool_cfg_s *cfg)
{
  FAR const char *cmd;
  int argpos = 1;

  if (argc < 2)
    {
      return ERROR;
    }

  if (strcmp(argv[argpos++], "--phy") == 0)
    {
      int phynum;

      if (ARGS_REMAIN(argc, argpos) < 3) /* at least phynum, cmd, intf */
        {
          return ERROR;
        }

      if (get_num(argv[argpos++], &phynum))
        {
          return ERROR;
        }

      if (0 <= phynum && phynum <= 31)
        {
          cfg->phy = phynum;
        }
      else
        {
          return ERROR;
        }
    }
  else
    {
      argpos = 1;
      cfg->phy = 0;
    }

  cmd = argv[argpos++];

  if (strcmp(cmd, "set") == 0)
    {
      cfg->cmd = PLCA_CMD_SET;

      if (ARGS_REMAIN(argc, argpos) < 3) /* name, at least one param-value */
        {
          return ERROR;
        }

      cfg->ifname = argv[argpos++];

      while (ARGS_REMAIN(argc, argpos) >= 2) /* at least one param-value */
        {
          FAR const char *param = argv[argpos++];
          FAR const char *value = argv[argpos++];

          if (strcmp(param, "enable") == 0)
            {
              if (cfg->enable)
                {
                  return ERROR;
                }

              if (strcmp(value, "on") == 0)
                {
                  PLCATOOL_CFG_SET(cfg, enable, 1);
                }
              else if (strcmp(value, "off") == 0)
                {
                  PLCATOOL_CFG_SET(cfg, enable, 0);
                }
              else
                {
                  return ERROR;
                }
            }
          else if (strcmp(param, "node-id") == 0)
            {
              int n;

              if (cfg->node_id)
                {
                  return ERROR;
                }

              if (get_num(value, &n))
                {
                  return ERROR;
                }

              if (NODE_ID_MIN <= n && n <= NODE_ID_MAX)
                {
                  PLCATOOL_CFG_SET(cfg, node_id, n);
                }
              else
                {
                  fprintf(stderr, "node-id out of range\n");
                  return ERROR;
                }
            }
          else if (strcmp(param, "node-cnt") == 0)
            {
              int n;

              if (cfg->node_cnt)
                {
                  return ERROR;
                }

              if (get_num(value, &n))
                {
                  fprintf(stderr, "Not a valid integer\n");
                  return ERROR;
                }

              if (NODE_CNT_MIN <= n && n <= NODE_CNT_MAX)
                {
                  PLCATOOL_CFG_SET(cfg, node_cnt, n);
                }
              else
                {
                  fprintf(stderr, "node-cnt out of range\n");
                  return ERROR;
                }
            }
          else if (strcmp(param, "to-tmr") == 0)
            {
              int n;

              if (cfg->to_tmr)
                {
                  return ERROR;
                }

              if (get_num(value, &n))
                {
                  fprintf(stderr, "Not a valid integer\n");
                  return ERROR;
                }

              if (TO_TMR_MIN <= n && n <= TO_TMR_MAX)
                {
                  PLCATOOL_CFG_SET(cfg, to_tmr, n);
                }
              else
                {
                  fprintf(stderr, "to_tmr out of range\n");
                  return ERROR;
                }
            }
          else if (strcmp(param, "burst-cnt") == 0)
            {
              int n;

              if (cfg->burst_cnt)
                {
                  return ERROR;
                }

              if (get_num(value, &n))
                {
                  fprintf(stderr, "Not a valid integer\n");
                  return ERROR;
                }

              if (BURST_CNT_MIN <= n && n <= BURST_CNT_MAX)
                {
                  PLCATOOL_CFG_SET(cfg, burst_cnt, n);
                }
              else
                {
                  fprintf(stderr, "burst-cnt out of range\n");
                  return ERROR;
                }
            }
          else if (strcmp(param, "burst-tmr") == 0)
            {
              int n;

              if (cfg->burst_tmr)
                {
                  return ERROR;
                }

              if (get_num(value, &n))
                {
                  fprintf(stderr, "Not a valid integer\n");
                  return ERROR;
                }

              if (BURST_TMR_MIN <= n && n <= BURST_TMR_MAX)
                {
                  PLCATOOL_CFG_SET(cfg, burst_tmr, n);
                }
              else
                {
                  fprintf(stderr, "burst-tmr out of range\n");
                  return ERROR;
                }
            }
          else
            {
              return ERROR;
            }
        }

      if (ARGS_REMAIN(argc, argpos) != 0)
        {
          return ERROR;
        }
    }
  else if (strcmp(cmd, "get") == 0)
    {
      cfg->cmd = PLCA_CMD_GET;

      if (ARGS_REMAIN(argc, argpos) != 1) /* ifname */
        {
          return ERROR;
        }

      cfg->ifname = argv[argpos++];
      if (strlen(cfg->ifname) > IFNAMSIZ)
        {
          fprintf(stderr, "No such interface\n");
          return ERROR;
        }
    }
  else if (strcmp(cmd, "status") == 0)
    {
      cfg->cmd = PLCA_CMD_STATUS;

      if (ARGS_REMAIN(argc, argpos) != 1) /* ifname */
        {
          return ERROR;
        }

      cfg->ifname = argv[argpos++];
      if (strlen(cfg->ifname) > IFNAMSIZ)
        {
          fprintf(stderr, "No such interface\n");
          return ERROR;
        }
    }
  else if (strcmp(cmd, "-h") == 0)
    {
      cfg->cmd = PLCA_CMD_HELP;

      if (ARGS_REMAIN(argc, argpos) != 0)
        {
          return ERROR;
        }
    }
  else
    {
      return ERROR;
    }

  return OK;
}

static int write_plca_mmd(FAR const char *ifname, int phy,
                          uint16_t address, uint16_t data)
{
  struct ifreq req;
  int retval;
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  strcpy(req.ifr_name, ifname);
  req.ifr_ifru.ifru_mii_data.phy_id = mdio_phy_id_c45(OA_TC14_PLCA_MMD, phy);
  req.ifr_ifru.ifru_mii_data.reg_num = address;
  req.ifr_ifru.ifru_mii_data.val_in = data;
  retval = ioctl(sockfd, SIOCSMIIREG, (unsigned long)(&req));
  if (retval)
    {
      close(sockfd);
      fprintf(stderr, "Write unsuccessful\n");
      return ERROR;
    }

  close(sockfd);
  return OK;
}

static int read_plca_mmd(FAR const char *ifname, int phy,
                         uint16_t address, FAR uint16_t *data)
{
  struct ifreq req;
  int retval;
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  strcpy(req.ifr_name, ifname);
  req.ifr_ifru.ifru_mii_data.phy_id = mdio_phy_id_c45(OA_TC14_PLCA_MMD, phy);
  req.ifr_ifru.ifru_mii_data.reg_num = address;

  retval = ioctl(sockfd, SIOCGMIIREG, (unsigned long)(&req));
  if (retval)
    {
      close(sockfd);
      fprintf(stderr, "Read unsuccessful\n");
      return ERROR;
    }

  *data = req.ifr_ifru.ifru_mii_data.val_out;

  close(sockfd);
  return OK;
}

static int verify_support(FAR struct plcatool_cfg_s *cfg,
                          FAR bool *supported)
{
  uint16_t reg;
  if (read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_IDVER_ADDR, &reg))
    {
      return ERROR;
    }

  *supported = reg == OA_TC14_IDVER_VAL ? true : false;

  return OK;
}

static int plcatool_set(FAR struct plcatool_cfg_s *cfg)
{
  uint16_t reg;
  bool supported;

  /* Verify the correct address map code */

  if (verify_support(cfg, &supported))
    {
      return EIO;
    }

  if (!supported)
    {
      fprintf(stderr, "Device not supported\n");
      return EINVAL;
    }

  /* node-cnt, node-id */

  if (cfg->node_cnt || cfg->node_id)
    {
      if (read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_CTRL1_ADDR, &reg))
        {
          return EIO;
        }

      if (cfg->node_cnt)
        {
          int val = PLCATOOL_CFG_VAL(cfg, node_cnt);
          reg &= ~OA_TC14_CTRL1_NCNT_MASK;
          reg |= oa_tc14_field(val, CTRL1_NCNT);
        }

      if (cfg->node_id)
        {
          int val = PLCATOOL_CFG_VAL(cfg, node_id);
          reg &= ~OA_TC14_CTRL1_ID_MASK;
          reg |= oa_tc14_field(val, CTRL1_ID);
        }

      if (write_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_CTRL1_ADDR, reg))
        {
          return EIO;
        }
    }

  /* to-tmr */

  if (cfg->to_tmr)
    {
      int val = PLCATOOL_CFG_VAL(cfg, to_tmr);
      reg = oa_tc14_field(val, TOTMR_TOT);

      if (write_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_TOTMR_ADDR, reg))
        {
          return EIO;
        }
    }

  /* burst-cnt, burst-tmr */

  if (cfg->burst_cnt || cfg->burst_tmr)
    {
      if (read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_BURST_ADDR, &reg))
        {
          return EIO;
        }

      if (cfg->burst_cnt)
        {
          int val = PLCATOOL_CFG_VAL(cfg, burst_cnt);
          reg &= ~OA_TC14_BURST_MAXBC_MASK;
          reg |= oa_tc14_field(val, BURST_MAXBC);
        }

      if (cfg->burst_tmr)
        {
          int val = PLCATOOL_CFG_VAL(cfg, burst_tmr);
          reg &= ~OA_TC14_BURST_BTMR_MASK;
          reg |= oa_tc14_field(val, BURST_BTMR);
        }

      if (write_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_BURST_ADDR, reg))
        {
          return EIO;
        }
    }

  /* enable */

  if (cfg->enable)
    {
      int val;
      if (read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_CTRL0_ADDR, &reg))
        {
          return EIO;
        }

      val = PLCATOOL_CFG_VAL(cfg, enable);
      reg &= ~OA_TC14_CTRL0_EN_MASK;
      reg |= oa_tc14_field(val, CTRL0_EN);

      if (write_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_CTRL0_ADDR, reg))
        {
          return EIO;
        }

      read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_CTRL0_ADDR, &reg);
    }

  return OK;
}

static int plcatool_get(FAR struct plcatool_cfg_s *cfg)
{
  uint16_t ctrl0;
  uint16_t ctrl1;
  uint16_t totmr;
  uint16_t burst;
  bool supported;

  /* Verify the correct address map code */

  if (verify_support(cfg, &supported))
    {
      return EIO;
    }

  if (!supported)
    {
      fprintf(stderr, "Device not supported\n");
      return EINVAL;
    }

  if (read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_CTRL0_ADDR, &ctrl0) ||
      read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_CTRL1_ADDR, &ctrl1) ||
      read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_TOTMR_ADDR, &totmr) ||
      read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_BURST_ADDR, &burst))
    {
      return EIO;
    }

  cfg->enable    = oa_tc14_get_field(ctrl0, CTRL0_EN);
  cfg->node_id   = oa_tc14_get_field(ctrl1, CTRL1_ID);
  cfg->node_cnt  = oa_tc14_get_field(ctrl1, CTRL1_NCNT);
  cfg->to_tmr    = oa_tc14_get_field(totmr, TOTMR_TOT);
  cfg->burst_cnt = oa_tc14_get_field(burst, BURST_MAXBC);
  cfg->burst_tmr = oa_tc14_get_field(burst, BURST_BTMR);

  printf("PLCA settings for %s\n", cfg->ifname);
  printf("\tEnabled: %s\n", cfg->enable ? "Yes" : "No");
  printf("\tlocal node ID: %d (%s)\n", cfg->node_id,
         cfg->node_id == 0 ? "coordinator" :
         (cfg->node_id == 255 ? "unconfigured" : "follower"));
  printf("\tNode count: %d%s\n", cfg->node_cnt,
         cfg->node_id ? " (ignored)" : "");
  printf("\tTO timer: %d BT\n", cfg->to_tmr);
  printf("\tBurst count: %d (%s)\n", cfg->burst_cnt,
         cfg->burst_cnt > 0 ? "enabled" : "disabled");
  printf("\tBurst timer: %d BT\n", cfg->burst_tmr);

  return OK;
}

static int plcatool_status(FAR struct plcatool_cfg_s *cfg)
{
  uint16_t status;
  bool supported;

  /* Verify the correct address map code */

  if (verify_support(cfg, &supported))
    {
      return EIO;
    }

  if (!supported)
    {
      fprintf(stderr, "Device not supported\n");
      return EINVAL;
    }

  if (read_plca_mmd(cfg->ifname, cfg->phy, OA_TC14_STATUS_ADDR, &status))
    {
      return EIO;
    }

  printf("PLCA status of %s\n", cfg->ifname);
  printf("\tStatus: %s\n",
         oa_tc14_get_field(status, STATUS_PST) ? "on" : "off");

  return OK;
}

static void plcatool_usage(bool err)
{
  FAR FILE *out = err ? stderr : stdout;

  fprintf(out, "Usage:\n");
  fprintf(out, "  plcatool [--phy N] status <ifname>\n");
  fprintf(out, "  plcatool [--phy N] get <ifname>\n");
  fprintf(out, "  plcatool [--phy N] set <ifname> <param> <value> "
               "[<param> <value>] ...\n");
  fprintf(out, "    Accepted <param> <value> pairs:\n");
  fprintf(out, "      enable    on | off\n");
  fprintf(out, "      node-id   N in [%d .. %d]\n",
          NODE_ID_MIN, NODE_ID_MAX);
  fprintf(out, "      node-cnt  N in [%d .. %d]\n",
          NODE_CNT_MIN, NODE_CNT_MAX);
  fprintf(out, "      to-tmr    N in [%d .. %d]\n",
          TO_TMR_MIN, TO_TMR_MAX);
  fprintf(out, "      burst-cnt N in [%d .. %d]\n",
          BURST_CNT_MIN, BURST_CNT_MAX);
  fprintf(out, "      burst-tmr N in [%d .. %d]\n",
          BURST_TMR_MIN, BURST_TMR_MAX);
  fprintf(out, "  plcatool -h\n");
  fprintf(out, "  --phy N defaults to 0\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct plcatool_cfg_s cfg;
  memset(&cfg, 0, sizeof(cfg));

  int err = parse_args(argc, argv, &cfg);
  if (err)
    {
      plcatool_usage(err);
      return EINVAL;
    }

  switch (cfg.cmd)
    {
      case PLCA_CMD_SET:
          return plcatool_set(&cfg);

      case PLCA_CMD_GET:
          return plcatool_get(&cfg);

      case PLCA_CMD_STATUS:
          return plcatool_status(&cfg);

      case PLCA_CMD_HELP:
          plcatool_usage(false);
          break;
    }

  return OK;
}
