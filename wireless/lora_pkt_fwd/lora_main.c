/****************************************************************************
 * apps/wireless/lora_pkt_fwd/lora_main.c
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

/* The "lora" command, the equivalent of the AT command set of the vendor
 * gateway firmware:
 *
 *   lora sys     full system configuration        (AT+SYS)
 *   lora ver     firmware version                 (AT+VER)
 *   lora status  concentrator and forwarder state
 *   lora ch      show or change the channel plan  (AT+CH)
 *   lora server  show or change the server        (AT+PKTFWD)
 *   lora ip      network configuration            (AT+IP)
 *   lora mac     Ethernet address and gateway id  (AT+MAC)
 *   lora reset   restart the concentrator         (AT+RESET)
 *   lora tx      transmit one packet, for bring-up
 *   lora start   start the packet forwarder
 *   lora stop    stop the packet forwarder
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <nuttx/version.h>
#include <nuttx/wireless/lpwan/lora_gw.h>

#include "netutils/netlib.h"

#include "lora_pkt_fwd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LORA_SEPARATOR \
  "---------------------------------------------------------------"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lora_usage
 ****************************************************************************/

static void lora_usage(void)
{
  printf("lora - LoRa Gateway commands (concentrator + forwarder)\n");
  printf("Subcommands:\n");
  printf("  sys     : Show full system configuration (like AT+SYS)\n");
  printf("  ver     : Show firmware version (like AT+VER)\n");
  printf("  status  : Show gateway status and statistics\n");
  printf("  ch      : Show/set channel plan (like AT+CH)\n");
  printf("            Usage: lora ch [REGION]\n");
  printf("  server  : Show/set LoRaWAN server (like AT+PKTFWD)\n");
  printf("            Usage: lora server [HOST [PORT_UP [PORT_DOWN]]]\n");
  printf("  ip      : Show network configuration (like AT+IP)\n");
  printf("  mac     : Show MAC address and Gateway ID (like AT+MAC)\n");
  printf("  reset   : Reset concentrator and restart (like AT+RESET)\n");
  printf("  tx      : Transmit one packet, for bring-up\n");
  printf("            Usage: lora tx FREQ_HZ [SF [TEXT]]\n");
  printf("  start   : Start packet forwarder\n");
  printf("  stop    : Stop packet forwarder\n");
}

/****************************************************************************
 * Name: lora_opendev
 ****************************************************************************/

static int lora_opendev(void)
{
  int fd;

  fd = open(CONFIG_LORA_PKT_FWD_DEVPATH, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0)
    {
      fprintf(stderr, "lora: cannot open %s: %d\n",
              CONFIG_LORA_PKT_FWD_DEVPATH, errno);
      return -errno;
    }

  return fd;
}

/****************************************************************************
 * Name: lora_getregion
 ****************************************************************************/

static int lora_getregion(FAR struct lora_gw_regioninfo_s *info, int index)
{
  struct lora_gw_regionreq_s req;
  int fd;
  int ret;

  fd = lora_opendev();
  if (fd < 0)
    {
      return fd;
    }

  req.index = index;
  ret = ioctl(fd, WLIOC_GW_GETREGION, (unsigned long)&req);
  close(fd);

  if (ret < 0)
    {
      return -errno;
    }

  memcpy(info, &req.info, sizeof(*info));
  return OK;
}

/****************************************************************************
 * Name: lora_getstatus
 ****************************************************************************/

static int lora_getstatus(FAR struct lora_gw_status_s *status)
{
  int fd;
  int ret;

  fd = lora_opendev();
  if (fd < 0)
    {
      return fd;
    }

  ret = ioctl(fd, WLIOC_GW_GETSTATUS, (unsigned long)status);
  close(fd);

  return ret < 0 ? -errno : OK;
}

/****************************************************************************
 * Name: lora_printchannels
 ****************************************************************************/

static void lora_printchannels(FAR const struct lora_gw_regioninfo_s *info,
                               FAR const char *indent)
{
  FAR const struct lora_gw_chaninfo_s *ch;
  FAR const char *type;
  char rate[24];
  int i;

  for (i = 0; i < LORA_GW_IF_CHAIN_NB; i++)
    {
      ch = &info->channels[i];

      switch (ch->type)
        {
          case LORA_GW_CHAN_MULTI_SF:
            type = "LORA_MULTI_SF";
            strlcpy(rate, "SF7/SF12, BW125KHz", sizeof(rate));
            break;

          case LORA_GW_CHAN_STD:
            type = "LORA_STANDARD";
            snprintf(rate, sizeof(rate), "SF%u, BW%uKHz", ch->datarate,
                     ch->bandwidth == LORA_GW_BW_500K ? 500 :
                     (ch->bandwidth == LORA_GW_BW_250K ? 250 : 125));
            break;

          case LORA_GW_CHAN_FSK:
            type = "FSK";
            rate[0] = '\0';
            break;

          default:
            type = "OFF";
            rate[0] = '\0';
            break;
        }

      if (!ch->enable)
        {
          printf("%sCHANNEL%d: OFF                                 (%s)\n",
                 indent, i, type);
          continue;
        }

      printf("%sCHANNEL%d: %" PRIu32 ", %c, %s    (%s)\n",
             indent, i, ch->freq_hz, ch->rf_chain == 0 ? 'A' : 'B',
             rate, type);
    }
}

/****************************************************************************
 * Name: lora_cmd_ver
 ****************************************************************************/

static int lora_cmd_ver(void)
{
  printf("Powered by NuttX & LoRa Gateway HAL\n");
  printf("VERSION: %s (NuttX %s)\n", CONFIG_LORA_PKT_FWD_VERSION,
         CONFIG_VERSION_STRING);
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_ip
 ****************************************************************************/

static int lora_cmd_ip(void)
{
  struct in_addr addr;
  uint8_t flags = 0;

  netlib_getifstatus(LORA_FWD_IFNAME, &flags);

  printf("        ETHERNET: %s\n",
#ifdef CONFIG_NETUTILS_DHCPC
         "DHCP"
#else
         "static"
#endif
        );

  printf("            LINK: %s\n",
         (flags & IFF_UP) != 0 ? "UP" : "DOWN");

  if (netlib_get_ipv4addr(LORA_FWD_IFNAME, &addr) == OK)
    {
      printf("              IP: %s\n", inet_ntoa(addr));
    }

  if (netlib_get_ipv4netmask(LORA_FWD_IFNAME, &addr) == OK)
    {
      printf("         NETMASK: %s\n", inet_ntoa(addr));
    }

  if (netlib_get_dripv4addr(LORA_FWD_IFNAME, &addr) == OK)
    {
      printf("         GATEWAY: %s\n", inet_ntoa(addr));
    }

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_mac
 ****************************************************************************/

static int lora_cmd_mac(void)
{
  FAR struct lora_fwd_config_s *config = lora_fwd_config();
  uint8_t mac[IFHWADDRLEN];

  if (netlib_getmacaddr(LORA_FWD_IFNAME, mac) == OK)
    {
      printf("         MACADDR: %02X:%02X:%02X:%02X:%02X:%02X\n",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

  printf("      GATEWAY ID: %02X%02X%02X%02X%02X%02X%02X%02X\n",
         config->eui[0], config->eui[1], config->eui[2], config->eui[3],
         config->eui[4], config->eui[5], config->eui[6], config->eui[7]);

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_server
 ****************************************************************************/

static int lora_cmd_server(int argc, FAR char *argv[])
{
  FAR struct lora_fwd_config_s *config = lora_fwd_config();

  if (argc > 2)
    {
      unsigned long port_up = (argc > 3) ? strtoul(argv[3], NULL, 10) : 0;
      unsigned long port_down = (argc > 4) ? strtoul(argv[4], NULL, 10) : 0;

      lora_fwd_setserver(argv[2], port_up, port_down);

      if (lora_fwd_isrunning())
        {
          printf("lora: restart the forwarder to use the new server\n");
        }
    }

  printf("  LORAWAN SERVER: %s\n", config->server);
  printf(" UPLINK UDP PORT: %u\n", config->port_up);
  printf("DOWNLINK UDP PORT: %u\n", config->port_down);

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_ch
 ****************************************************************************/

static int lora_cmd_ch(int argc, FAR char *argv[])
{
  struct lora_gw_regioninfo_s info;
  int ret;
  int i;

  if (argc > 2)
    {
      int fd = lora_opendev();

      if (fd < 0)
        {
          return EXIT_FAILURE;
        }

      ret = ioctl(fd, WLIOC_GW_SETREGION, (unsigned long)argv[2]);
      if (ret < 0)
        {
          fprintf(stderr, "lora: cannot select %s: %d\n", argv[2], errno);
          close(fd);
          return EXIT_FAILURE;
        }

      /* Restart the concentrator so that the new plan takes effect */

      if (lora_fwd_isrunning())
        {
          ioctl(fd, WLIOC_GW_RESET, 0);
        }

      close(fd);
    }

  ret = lora_getregion(&info, -1);
  if (ret < 0)
    {
      fprintf(stderr, "lora: cannot read the channel plan: %d\n", -ret);
      return EXIT_FAILURE;
    }

  printf("%s Channel Plan (%s):\n", info.name, info.desc);
  printf("  Radio A: %" PRIu32 " Hz\n", info.radio_freq[0]);
  printf("  Radio B: %" PRIu32 " Hz\n", info.radio_freq[1]);
  printf("\n");

  lora_printchannels(&info, "  ");

  printf("\nAvailable regions:");
  for (i = 0; lora_getregion(&info, i) == OK; i++)
    {
      printf(" %s", info.name);
    }

  printf("\n");
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_status
 ****************************************************************************/

static int lora_cmd_status(void)
{
  struct lora_gw_status_s status;
  struct lora_fwd_stats_s stats;

  if (lora_getstatus(&status) < 0)
    {
      return EXIT_FAILURE;
    }

  lora_fwd_getstats(&stats);

  printf("Concentrator: %s\n", status.started ? "RUNNING" : "STOPPED");
  printf("Pkt Forwarder: %s\n",
         lora_fwd_isrunning() ? "RUNNING" : "STOPPED");
  printf("Uptime:  %" PRIu32 " s\n", stats.uptime_sec);
  printf("RX OK:   %" PRIu32 "\n", status.rx_ok);
  printf("RX BAD:  %" PRIu32 "\n", status.rx_bad);
  printf("RX NOCRC:%" PRIu32 "\n", status.rx_nocrc);
  printf("RX ERR:  %" PRIu32 "\n", status.rx_err);
  printf("RX FWD:  %" PRIu32 "\n", stats.rx_fwd);
  printf("TX OK:   %" PRIu32 "\n", status.tx_ok);
  printf("TX ERR:  %" PRIu32 "\n", status.tx_err);
  printf("PUSH:    %" PRIu32 " sent, %" PRIu32 " acked\n",
         stats.push_sent, stats.push_ack);
  printf("PULL:    %" PRIu32 " sent, %" PRIu32 " acked\n",
         stats.pull_sent, stats.pull_ack);

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_sys
 ****************************************************************************/

static int lora_cmd_sys(void)
{
  FAR struct lora_fwd_config_s *config = lora_fwd_config();
  struct lora_gw_regioninfo_s info;
  struct lora_gw_status_s status;

  printf("\n");
  printf("Powered by NuttX & LoRa Gateway HAL\n");
  printf("%s\n", LORA_SEPARATOR);
  printf("         VERSION: %s (NuttX %s)\n", CONFIG_LORA_PKT_FWD_VERSION,
         CONFIG_VERSION_STRING);
  printf("         LORAWAN: Public\n");

  lora_cmd_mac();

  printf("  LORAWAN SERVER: %s\n", config->server);
  printf(" UPLINK UDP PORT: %u\n", config->port_up);
  printf("DOWNLINK UDP PORT: %u\n", config->port_down);

  lora_cmd_ip();

  if (lora_getregion(&info, -1) == OK)
    {
      printf("          REGION: %s (%s)\n", info.name, info.desc);
      lora_printchannels(&info, "        ");
    }

  printf("%s\n", LORA_SEPARATOR);

  if (lora_getstatus(&status) == OK)
    {
      printf("   CONCENTRATOR: %s\n",
             status.started ? "Running" : "Stopped");
    }

  printf("        PKT FWD: %s\n",
         lora_fwd_isrunning() ? "Running" : "Stopped");

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_tx
 *
 * Description:
 *   Transmit a single packet.  This exists to bring a gateway up without a
 *   network server: any LoRa receiver tuned to the same frequency, spreading
 *   factor and bandwidth sees it.  The polarity is the one of an uplink, not
 *   the inverted one of a LoRaWAN downlink, so that a plain end device radio
 *   can pick it up.
 *
 ****************************************************************************/

static int lora_cmd_tx(int argc, FAR char *argv[])
{
  struct lora_gw_txpkt_s txpkt;
  FAR const char *text;
  unsigned long datarate;
  size_t len;
  int fd;
  int ret;

  if (argc < 3)
    {
      fprintf(stderr, "lora: usage: lora tx FREQ_HZ [SF [TEXT]]\n");
      return EXIT_FAILURE;
    }

  memset(&txpkt, 0, sizeof(txpkt));

  datarate = (argc > 3) ? strtoul(argv[3], NULL, 10) : 7;
  text     = (argc > 4) ? argv[4] : "NuttX LoRa gateway";

  if (datarate < 6 || datarate > 12)
    {
      fprintf(stderr, "lora: spreading factor out of range\n");
      return EXIT_FAILURE;
    }

  len = strlen(text);
  if (len > sizeof(txpkt.payload))
    {
      len = sizeof(txpkt.payload);
    }

  txpkt.freq_hz    = strtoul(argv[2], NULL, 10);
  txpkt.datarate   = datarate;

  txpkt.tx_mode    = LORA_GW_TX_IMMEDIATE;
  txpkt.rf_chain   = 0;
  txpkt.rf_power   = CONFIG_LORA_PKT_FWD_TXPOWER;
  txpkt.modulation = LORA_GW_MOD_LORA;
  txpkt.bandwidth  = LORA_GW_BW_125K;
  txpkt.coderate   = WLIOC_LORA_CR_4_5;
  txpkt.preamble   = 8;
  txpkt.size       = len;

  memcpy(txpkt.payload, text, len);

  fd = lora_opendev();
  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  ret = write(fd, &txpkt, sizeof(txpkt));
  close(fd);

  if (ret != sizeof(txpkt))
    {
      fprintf(stderr, "lora: transmit failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  printf("lora: sent %zu bytes at %" PRIu32 " Hz, SF%lu\n",
         len, txpkt.freq_hz, datarate);
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: lora_cmd_reset
 ****************************************************************************/

static int lora_cmd_reset(void)
{
  int fd;
  int ret;

  fd = lora_opendev();
  if (fd < 0)
    {
      return EXIT_FAILURE;
    }

  ret = ioctl(fd, WLIOC_GW_RESET, 0);
  close(fd);

  if (ret < 0)
    {
      fprintf(stderr, "lora: reset failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  printf("lora: concentrator restarted\n");
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  lora_fwd_init();

  if (argc < 2)
    {
      lora_usage();
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "sys") == 0)
    {
      return lora_cmd_sys();
    }
  else if (strcmp(argv[1], "ver") == 0)
    {
      return lora_cmd_ver();
    }
  else if (strcmp(argv[1], "status") == 0)
    {
      return lora_cmd_status();
    }
  else if (strcmp(argv[1], "ch") == 0)
    {
      return lora_cmd_ch(argc, argv);
    }
  else if (strcmp(argv[1], "server") == 0)
    {
      return lora_cmd_server(argc, argv);
    }
  else if (strcmp(argv[1], "ip") == 0)
    {
      return lora_cmd_ip();
    }
  else if (strcmp(argv[1], "mac") == 0)
    {
      return lora_cmd_mac();
    }
  else if (strcmp(argv[1], "reset") == 0)
    {
      return lora_cmd_reset();
    }
  else if (strcmp(argv[1], "tx") == 0)
    {
      return lora_cmd_tx(argc, argv);
    }
  else if (strcmp(argv[1], "start") == 0)
    {
      ret = lora_fwd_start();
      if (ret < 0)
        {
          fprintf(stderr, "lora: cannot start the forwarder: %d\n", -ret);
          return EXIT_FAILURE;
        }

      return EXIT_SUCCESS;
    }
  else if (strcmp(argv[1], "stop") == 0)
    {
      ret = lora_fwd_stop();
      if (ret < 0)
        {
          fprintf(stderr, "lora: cannot stop the forwarder: %d\n", -ret);
          return EXIT_FAILURE;
        }

      return EXIT_SUCCESS;
    }

  fprintf(stderr, "lora: unknown subcommand '%s'\n", argv[1]);
  lora_usage();
  return EXIT_FAILURE;
}
