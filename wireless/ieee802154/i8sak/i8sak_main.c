/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_main.c
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2014-2015, 2017 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2014-2015 Sebastien Lorquet. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * This app can be used to control an IEEE 802.15.4 device from command line.
 * Example valid ieee802154 packets:
 *
 * Beacon request, seq number 01:
 *   xx xx 01
 *
 * Simple data packet
 * from long address xx to long address yy, pan id pp for src and dest
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/fs/ioctl.h>

#include <nuttx/wireless/ieee802154/ieee802154_ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct i8_command_s
{
  FAR const char *name;
  uint8_t noptions;
  CODE void *handler;
};

/* Generic form of a command handler */

typedef void (*cmd0_t)(FAR const char *devname);
typedef void (*cmd1_t)(FAR const char *devname, FAR const char *arg1);
typedef void (*cmd2_t)(FAR const char *devname, FAR const char *arg1,
                       FAR const char *arg2);
typedef void (*cmd3_t)(FAR const char *devname, FAR const char *arg1,
                       FAR const char *arg2, FAR const char *arg3);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int i8_tx(int fd);
static int i8_parse_payload(FAR const char *str);
static pthread_addr_t i8_eventlistener(pthread_addr_t arg);

static void i8_tx_cmd(FAR const char *devname, FAR const char *payload);
static void i8_sniffer_cmd(FAR const char *devname);
static void i8_blaster_cmd(FAR const char *devname, FAR const char *period_ms,
                           FAR const char *payload);

static int i8_sniffer_daemon(int argc, FAR char *argv[]);
static int i8_blaster_daemon(int argc, FAR char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct i8_command_s g_i8_commands[] =
{
  {"help",    0, (CODE void *)NULL},
  {"tx",      1, (CODE void *)i8_tx_cmd},
  {"sniffer", 0, (CODE void *)i8_sniffer_cmd},
  {"blaster", 2, (CODE void *)i8_blaster_cmd},
};

#define NCOMMANDS (sizeof(g_i8_commands) / sizeof(struct i8_command_s))

uint8_t g_handle = 0;
uint8_t g_txframe[IEEE802154_MAX_MAC_PAYLOAD_SIZE];
uint16_t g_txframe_len;

int g_blaster_period = 0;

bool g_sniffer_daemon_started = false;
bool g_blaster_daemon_started = false;
bool g_eventlistener_run = false;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8_sniffer_cmd
 *
 * Description :
 *   Starts a thread to run the sniffer in the background
 ****************************************************************************/

static void i8_sniffer_cmd(FAR const char *devname)
{
  int ret;
  FAR const char *sniffer_argv[2];

  printf("i8sak: Starting sniffer_daemon\n");

  if (g_sniffer_daemon_started)
    {
      printf("i8sak: sniffer_daemon already running\n");
      return;
    }
  
  sniffer_argv[0] = devname;
  sniffer_argv[1] = NULL;
  
  ret = task_create("sniffer_daemon", CONFIG_IEEE802154_I8SAK_PRIORITY,
                    CONFIG_IEEE802154_I8SAK_STACKSIZE, i8_sniffer_daemon,
                    (FAR char * const *)sniffer_argv);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to start sniffer_daemon\n", errno);
      return;
    }

  printf("i8sak: sniffer_daemon started\n");
}
 
/****************************************************************************
 * Name : i8_sniffer_daemon
 *
 * Description :
 *   Sniff for frames (Promiscuous mode)
 ****************************************************************************/

static int i8_sniffer_daemon(int argc, FAR char *argv[])
{
  int ret, fd, i;
  struct mac802154dev_rxframe_s rx;

  fd = open(argv[1], O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", argv[1], errno);
      ret = errno;
      return ret;
    }

  printf("Listening...\n"); 

  g_sniffer_daemon_started = true;

  /* We don't care about any events, so disable them */

  ret = ieee802154_enableevents(fd, false);

  /* Enable promiscuous mode */

  ret = ieee802154_setpromisc(fd, true);

  /* Make sure receiver is always on while idle */

  ret = ieee802154_setrxonidle(fd, true);

  while(1)
    {
      ret = read(fd, &rx, sizeof(struct mac802154dev_rxframe_s));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: read failed: %d\n", errno);
          goto done;
        }

      printf("Frame Received:\n");

      for (i = 0; i < rx.length; i++)
        {
          printf("%02X", rx.payload[i]);
        }

      printf(" \n");

      fflush(stdout);
    }

done:
  /* Turn receiver off when idle */

  ret = ieee802154_setrxonidle(fd, false);

  /* Disable promiscuous mode */

  ret = ieee802154_setpromisc(fd, false);

  ret = ieee802154_enableevents(fd, true);

  printf("sniffer_daemon: closing");
  close(fd);
  g_sniffer_daemon_started = false;
  return OK;
}

/****************************************************************************
 * Name : i8_blaster_cmd
 *
 * Description :
 *   Starts a thread to send continuous packets at a fixed interval
 ****************************************************************************/

static void i8_blaster_cmd(FAR const char *devname, FAR const char *period_ms,
                           FAR const char *payload)
{
  int ret;
  FAR const char *blaster_argv[2];

  printf("i8sak: Starting blaster_daemon\n");

  if (g_blaster_daemon_started)
    {
      printf("i8sak: blaster_daemon already running\n");
      return;
    }
  
  blaster_argv[0] = devname;
  blaster_argv[1] = NULL;
  
  g_blaster_period = atoi(period_ms);

  ret = i8_parse_payload(payload);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:invalid hex payload\n", ret);
      return;
    }
  
  ret = task_create("blaster_daemon", CONFIG_IEEE802154_I8SAK_PRIORITY,
                    CONFIG_IEEE802154_I8SAK_STACKSIZE, i8_blaster_daemon,
                    (FAR char * const *)blaster_argv);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to start blaster_daemon", errno);
      return;
    }

  printf("i8sak: blaster_daemon started\n");
}

/****************************************************************************
 * Name : i8_blaster_daemon
 *
 * Description :
 *   Continuously transmit a packet
 ****************************************************************************/

static int i8_blaster_daemon(int argc, FAR char *argv[])
{
  int ret, fd;
  pthread_t eventthread;

  fd = open(argv[1], O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR:cannot open %s, errno=%d\n", argv[1], errno);
      ret = errno;
      return ret;
    }

  printf("blaster_daemon: starting\n"); 
  g_blaster_daemon_started = true;

  /* Start a thread to handle any events that occur */

  g_eventlistener_run = true;
  ret = pthread_create(&eventthread, NULL, i8_eventlistener, (void *)&fd);
  if (ret != 0)
    {
      printf("blaster_daemon: Failed to create eventlistener thread: %d\n", ret);
      goto done;
    }

  while(1)
    {
      ret = i8_tx(fd);
      if (ret < 0)
        {
          goto done;
        }
      ret = usleep(g_blaster_period*1000L);
      if (ret < 0)
        {
          goto done;
        }
    }

done:
  /* Tell the eventthread to stop */

  printf("blaster_daemon: closing\n");
  g_eventlistener_run = false;
  pthread_join(eventthread, NULL);
  close(fd);
  g_blaster_daemon_started = false;
  return OK;
}

/****************************************************************************
 * Name : i8_tx_cmd
 *
 * Description :
 *   Transmit a data frame.
 ****************************************************************************/

static void i8_tx_cmd(FAR const char *devname, FAR const char *str)
{
  int ret, fd;
  int i = 0;

  ret = i8_parse_payload(str);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:invalid hex payload\n", ret);
      return;
    }

  for (i = 0; i < g_txframe_len; i++)
    {
      printf("%02X", g_txframe[i]);
    }

  fflush(stdout);

  /* Open device */

  fd = open(devname, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR:cannot open %s, errno=%d\n", devname, errno);
      return;
    }

  ret = i8_tx(fd);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:Failed to transmit packet.\n", ret);
    }
  
  close(fd);
}

/****************************************************************************
 * Name : i8_eventlistener
 *
 * Description :
 *   Listen for events from the MAC layer
 ****************************************************************************/

static pthread_addr_t i8_eventlistener(pthread_addr_t arg)
{
  int ret;
  struct ieee802154_notif_s notif;
  int fd = *(int *)arg;

  while (g_eventlistener_run)
    {
      ret = ioctl(fd, MAC802154IOC_GET_EVENT, (unsigned long)((uintptr_t)&notif));
      if (ret < 0)
        {
          fprintf(stderr, "MAC802154IOC_GET_EVENTS failed: %d\n", ret);
        }
      
      switch (notif.notiftype)
        {
          case IEEE802154_NOTIFY_CONF_DATA:
            printf("Data Confirmation Status %d\n", notif.u.dataconf.status);
            break;
          default:
            printf("Unhandled notification: %d\n", notif.notiftype);
            break;
        }
    }

    return NULL;
}

/****************************************************************************
 * Name : i8_parse_payload
 *
 * Description :
 *   Parse string to get payload
 ****************************************************************************/

static int i8_parse_payload(FAR const char *str)
{
  int str_len;
  int i = 0;
  
  str_len = strlen(str);

  /* Each byte is represented by 2 chars */

  g_txframe_len = str_len >> 1;

  /* Check if the number of chars is a multiple of 2 and that the number of 
   * bytes does not exceed the max MAC frame payload supported */

  if ((str_len & 1) || (g_txframe_len > IEEE802154_MAX_MAC_PAYLOAD_SIZE))
    {
      return -EINVAL;
    }

  /* Decode hex packet */

  while (str_len > 0)
    {
      int dat;
      if (sscanf(str, "%2x", &dat) == 1)
        {
          g_txframe[i++] = dat;
          str += 2;
          str_len -= 2;
        }
      else
        {
          return -EINVAL;
        }
    }
  
  return OK;
}

/****************************************************************************
 * Name : i8_tx
 *
 * Description :
 *   Transmit a data frame.
 ****************************************************************************/

static int i8_tx(int fd)
{
  int ret;
  struct mac802154dev_txframe_s tx;

  /* Set an application defined handle */

  tx.meta.msdu_handle = g_handle++;

  /* This is a normal transaction, no special handling */

  tx.meta.msdu_flags.ack_tx = 0;
  tx.meta.msdu_flags.gts_tx = 0;
  tx.meta.msdu_flags.indirect_tx = 0;

  tx.meta.ranging = IEEE802154_NON_RANGING;

  tx.meta.src_addrmode = IEEE802154_ADDRMODE_EXTENDED;
  tx.meta.dest_addr.mode = IEEE802154_ADDRMODE_SHORT;
  tx.meta.dest_addr.saddr = 0xFADE;

  /* Each byte is represented by 2 chars */

  tx.length = g_txframe_len;
  tx.payload = &g_txframe[0];

  ret = write(fd, &tx, sizeof(struct mac802154dev_txframe_s));
  if (ret == OK)
    {
      printf(" Tx OK\n");
    }
  else
    {
      printf(" write: errno=%d\n",errno);
    }
  
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i8_showusage
 *
 * Description:
 *   Show program usage.
 *
 ****************************************************************************/

int i8_showusage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage:\n", progname);
  fprintf(stderr, "\t%s <op> <command> [OPTIONS]\n", progname);
  fprintf(stderr, "\nWhere supported commands and [OPTIONS] appear below\n");
  fprintf(stderr, "\t%s tx <devname> <hex payload>\n", progname);
  fprintf(stderr, "\t%s sniffer <devname>\n", progname);
  fprintf(stderr, "\t%s blaster <devname> <period (ms)> <hex payload>\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * i8_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int i8_main(int argc, char *argv[])
#endif
{
  FAR const char *cmdname;
  FAR const char *devname;
  FAR const struct i8_command_s *i8cmd;
  int i;

  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing command\n");
      i8_showusage(argv[0], EXIT_FAILURE);
    }

  if (argc < 3)
    {
      fprintf(stderr, "ERROR: Missing devname\n");
      i8_showusage(argv[0], EXIT_FAILURE);
    }

  cmdname = argv[1];
  devname = argv[2];

  /* Find the command in the g_i8_command[] list */

  i8cmd = NULL;
  for (i = 0; i < NCOMMANDS; i++)
    {
      FAR const struct i8_command_s *cmd = &g_i8_commands[i];
      if (strcmp(cmdname, cmd->name) == 0)
        {
          i8cmd = cmd;
          break;
        }
    }

  if (i8cmd == NULL)
    {
      fprintf(stderr, "ERROR: Unsupported command: %s\n", cmdname);
      i8_showusage(argv[0], EXIT_FAILURE);
    }

  if (i8cmd->noptions + 1 < argc)
    {
      fprintf(stderr, "ERROR: Garbage at end of command ignored\n");
    }
  else if (i8cmd->noptions + 1 > argc)
    {
      fprintf(stderr, "ERROR: Missing required command options: %s\n",
              cmdname);
      i8_showusage(argv[0], EXIT_FAILURE);
    }

  /* Special case the help command which has no arguments, no handler,
   * and does not need a socket.
   */

  if (i8cmd->handler == NULL)
    {
      i8_showusage(argv[0], EXIT_SUCCESS);
    }

  /* Dispatch the command handling */

  switch (i8cmd->noptions)
    {
      case 0:
        ((cmd0_t)i8cmd->handler)(devname);
        break;

      case 1:
        ((cmd1_t)i8cmd->handler)(devname, argv[3]);
        break;

      case 2:
        ((cmd2_t)i8cmd->handler)(devname, argv[3], argv[4]);
        break;

      case 3:
        ((cmd3_t)i8cmd->handler)(devname, argv[3], argv[4], argv[5]);
        break;

      default:
        fprintf(stderr, "ERROR: Too many arguments: %s\n",
                cmdname);
        i8_showusage(argv[0], EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
