/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_main.c
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2014-2015, 2017 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2014-2015 Sebastien Lorquet. All rights reserved.
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
 *   Author: Anthony Merlino <anthony@vergeaero.com>
 *   Author: Gregory Nuttx <gnutt@nuttx.org>
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
 * Private Function Prototypes
 ****************************************************************************/

static int i8_tx(int fd, bool indirect);
static int i8_parse_payload(FAR const char *str);
static pthread_addr_t i8_eventlistener(pthread_addr_t arg);

static int i8_tx_cmd(int fd);
static int i8_sniffer(int fd);
static int i8_blaster(int fd);
static int i8_startpan(int fd);
static int i8_quit(void);

static int i8_test_indirect(int fd);
static int i8_test_poll(int fd);
static int i8_test_assoc(int fd);

static int i8_daemon(int argc, FAR char *argv[]);

/****************************************************************************
 * Macros
 ****************************************************************************/

#define I8_DEFAULT_PANID 0xFADE
#define I8_DEFAULT_COORD_SADDR 0x000A
#define I8_DEFAULT_COORD_EADDR 0xDEADBEEF00FADE0A
#define I8_DEFAULT_DEV_SADDR 0x000B
#define I8_DEFAULT_DEV_EADDR 0xDEADBEEF00FADE0B
#define I8_DEFAULT_CHNUM 0x0B
#define I8_DEFAULT_CHPAGE 0x00

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum i8_cmd_e
{
  I8_CMD_NONE = 0x00,
  I8_CMD_TX,
  I8_CMD_SNIFFER,
  I8_CMD_BLASTER,
  I8_CMD_PANCOORD,
  I8_CMD_TEST_INDIRECT = 0x40,
  I8_CMD_TEST_POLL,
  I8_CMD_TEST_ASSOC,
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

FAR const char *g_devname;

uint8_t g_handle = 0;
uint8_t g_txframe[IEEE802154_MAX_MAC_PAYLOAD_SIZE];
uint16_t g_txframe_len;

enum i8_cmd_e g_cmd = I8_CMD_NONE;
sem_t g_cmdsem;
CODE int (*g_cmdfunc)(int fd);

int g_blaster_period = 0;

struct ieee802154_addr_s g_dev;
struct ieee802154_addr_s g_coord;

pid_t g_daemon_pid;
bool g_daemon_started = false;
bool g_eventlistener_run = false;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8_start_daemon
 *
 * Description :
 *   Starts a thread to run command on
 ****************************************************************************/

static void i8_start_command(enum i8_cmd_e cmd)
{
  if (g_daemon_started)
    {
      printf("i8sak: command already running\n");
      return;
    }
  
  g_cmd = cmd; 
  
  switch (g_cmd)
    {
      case I8_CMD_BLASTER:
        g_cmdfunc = i8_blaster;
        break;
      case I8_CMD_SNIFFER:
        g_cmdfunc = i8_sniffer;
        break;
      case I8_CMD_TX:
        g_cmdfunc = i8_tx_cmd;
        break;
      case I8_CMD_PANCOORD:
        g_cmdfunc = i8_startpan;
        break;
      case I8_CMD_TEST_ASSOC:
        g_cmdfunc = i8_test_assoc;
        break;
      case I8_CMD_TEST_INDIRECT:
        g_cmdfunc = i8_test_indirect;
        break;
      case I8_CMD_TEST_POLL:
        g_cmdfunc = i8_test_poll;
        break;
      default:
        fprintf(stderr, "invalid command\n");
        return;
    }
  
  g_daemon_pid = task_create("i8_daemon", CONFIG_IEEE802154_I8SAK_PRIORITY,
                             CONFIG_IEEE802154_I8SAK_STACKSIZE, i8_daemon,
                             NULL);
                    
  if (g_daemon_pid < 0)
    {
      fprintf(stderr, "failed to start daemon\n", errno);
      return;
    }

  g_daemon_started = true;
  printf("i8sak: daemon started\n");
}

/****************************************************************************
 * Name : i8_daemon
 *
 * Description :
 *   Runs command in seperate task
 ****************************************************************************/

static int i8_daemon(int argc, FAR char *argv[])
{
  int fd, ret;
  pthread_t eventthread;

  fd = open(g_devname, O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", g_devname, errno);
      ret = errno;
      return ret;
    }

  /* Start a thread to handle any events that occur */

  g_eventlistener_run = true;
  ret = pthread_create(&eventthread, NULL, i8_eventlistener, (void *)&fd);
  if (ret != 0)
    {
      printf("i8_daemon: failed to create eventlistener thread: %d\n", ret);
      goto done;
    }
  
  g_cmdfunc(fd);

  g_eventlistener_run = false;
  pthread_kill(eventthread, 9);
  pthread_join(eventthread, NULL);
done:
  close(fd);
  g_cmd = I8_CMD_NONE;
  g_daemon_started = false;
  printf("i8sak: cmd finished\n");
  return OK;
}
 
/****************************************************************************
 * Name : i8_sniffer
 *
 * Description :
 *   Sniff for frames (Promiscuous mode)
 ****************************************************************************/

static int i8_sniffer(int fd)
{
  int ret, i;
  struct mac802154dev_rxframe_s rx;

  printf("Listening...\n"); 

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
          break;
        }

      printf("Frame Received:\n");

      for (i = 0; i < rx.length; i++)
        {
          printf("%02X", rx.payload[i]);
        }

      printf(" \n");

      fflush(stdout);
    }

  /* Turn receiver off when idle */

  ret = ieee802154_setrxonidle(fd, false);

  /* Disable promiscuous mode */

  ret = ieee802154_setpromisc(fd, false);

  ret = ieee802154_enableevents(fd, true);

  printf("sniffer closing\n");
  return OK;
}

/****************************************************************************
 * Name : i8_blaster
 *
 * Description :
 *   Continuously transmit a packet
 ****************************************************************************/

static int i8_blaster(int fd)
{
  int ret;

  printf("blaster starting\n"); 

  while(1)
    {
      ret = i8_tx(fd, false);
      if (ret < 0)
        {
          break;
        }
      ret = usleep(g_blaster_period*1000L);
      if (ret < 0)
        {
          break;
        }
    }

  printf("blaster closing\n");
  return OK;
}

/****************************************************************************
 * Name : i8_tx_cmd
 *
 * Description :
 *   Transmit a data frame.
 ****************************************************************************/

static int i8_tx_cmd(int fd)
{
  int ret;

  ret = i8_tx(fd, false);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:Failed to transmit packet.\n", ret);
    }
  
  close(fd);
  return ret;
}

/****************************************************************************
 * Name : i8_test_coord_indirect
 *
 * Description :
 *   Try and send an indirect transaction to a device
 ****************************************************************************/

static int i8_test_indirect(int fd)
{
  int ret;

  /* Always listen */

  printf("i8sak: enabling receiver\n");
  ieee802154_setrxonidle(fd, true);

  printf("i8sak: queuing indirect transaction\n");
  fflush(stdout);

  ret = i8_tx(fd, true);
  if (ret < 0)
    {
      fprintf(stderr, "i8sak: failed to transmit packet\n", ret);
    }
  
  /* Wait here, the event listener will notify us if the correct event occurs */

  ret = sem_wait(&g_cmdsem);
  if (ret != OK)
    {
      printf("i8sak: test cancelled\n");
      return -EINTR;
    }

  printf("i8sak: test success\n");
  return OK;
}

/****************************************************************************
 * Name : i8_test_poll
 *
 * Description :
 *   Try and extract data from the coordinator
 ****************************************************************************/

static int i8_test_poll(int fd)
{
  struct ieee802154_poll_req_s pollreq;
  int ret;
  
  printf("i8sak: Polling coordinator. PAN ID: 0x%04X SADDR: 0x%04X\n",
         g_coord.panid,
         g_coord.saddr);

  pollreq.coordaddr.mode = IEEE802154_ADDRMODE_SHORT;
  pollreq.coordaddr.saddr = g_coord.saddr;
  pollreq.coordaddr.panid = g_coord.panid;

  ieee802154_poll_req(fd, &pollreq);

  /* Wait here, the event listener will notify us if the correct event occurs */

  ret = sem_wait(&g_cmdsem);
  if (ret != OK)
    {
      printf("i8sak: test cancelled\n");
      return -EINTR;
    }

  printf("i8sak: test finished\n");
  return OK;
}

/****************************************************************************
 * Name : i8_startpan
 *
 * Description :
 *   Start PAN and accept association requests
 ****************************************************************************/

static int i8_startpan(int fd)
{
  int ret;
  struct ieee802154_reset_req_s resetreq;
  struct ieee802154_start_req_s startreq;

  /* Reset the MAC layer */

  printf("\ni8sak: resetting MAC layer\n");
  resetreq.rst_pibattr = true;
  ieee802154_reset_req(fd, &resetreq);

  /* Make sure receiver is always on */

  ret = ieee802154_setrxonidle(fd, true);

  /* Set EADDR and SADDR */

  ieee802154_seteaddr(fd, &g_coord.eaddr[0]);
  ieee802154_setsaddr(fd, g_coord.saddr);

  /* Tell the MAC to start acting as a coordinator */

  printf("i8sak: starting PAN\n");

  startreq.panid = g_coord.panid;
  startreq.chnum = I8_DEFAULT_CHNUM;
  startreq.chpage = I8_DEFAULT_CHPAGE;
  startreq.beaconorder = 15;
  startreq.pancoord = true;
  startreq.coordrealign = false;

  ieee802154_start_req(fd, &startreq);

  /* Wait here, the event listener will notify us if the correct event occurs */

  ret = sem_wait(&g_cmdsem);
  if (ret != OK)
    {
      printf("i8sak: test cancelled\n");
      return -EINTR;
    }

  return OK;
}

/****************************************************************************
 * Name : i8_test_assoc
 *
 * Description :
 *   Request association with the Coordinator
 ****************************************************************************/

static int i8_test_assoc(int fd)
{
  int ret;
  struct ieee802154_assoc_req_s assocreq;

  /* Set the extended address of the device */

  ieee802154_seteaddr(fd, &g_dev.eaddr[0]);

  printf("i8sak: issuing ASSOCIATION.request primitive\n");

  assocreq.chnum = I8_DEFAULT_CHNUM;
  assocreq.chpage = I8_DEFAULT_CHPAGE;
  assocreq.coordaddr.panid = I8_DEFAULT_PANID;
  assocreq.coordaddr.mode = IEEE802154_ADDRMODE_SHORT;
  assocreq.coordaddr.saddr = g_coord.saddr;

  assocreq.capabilities.devtype = 0;
  assocreq.capabilities.powersource = 1;
  assocreq.capabilities.rxonidle = 1;
  assocreq.capabilities.security = 0;
  assocreq.capabilities.allocaddr = 1;

  ieee802154_assoc_req(fd, &assocreq);

  /* Wait here, the event listener will notify us if the correct event occurs */

  ret = sem_wait(&g_cmdsem);
  if (ret != OK)
    {
      printf("i8sak: test cancelled\n");
      return -EINTR;
    }

  return OK;
}

/****************************************************************************
 * Name : i8_quit
 *
 * Description :
 *    Quit a running command
 *
 ****************************************************************************/

static int i8_quit(void)
{
  if (g_daemon_started)
    {
      kill(g_daemon_pid, 9);
    }
  else
    {
      fprintf(stderr, "no command running\n");
      return ERROR;
    }

  return OK;
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
  struct ieee802154_assoc_resp_s assocresp;
  struct mac802154dev_rxframe_s rx;
  int i;
  int fd = *(int *)arg;

  while (g_eventlistener_run)
    {
      ret = ioctl(fd, MAC802154IOC_GET_EVENT, (unsigned long)((uintptr_t)&notif));
      if (ret != OK)
        {
          return NULL;
        }
      
      switch (notif.notiftype)
        {
          case IEEE802154_NOTIFY_CONF_DATA:
            if (notif.u.dataconf.status == IEEE802154_STATUS_SUCCESS)
              {
                printf("i8sak: frame successfully transmitted\n");
              }
            else
              {
                printf("i8sak: frame failed to send:  %d\n", notif.u.dataconf.status);
              }
            
            if (g_cmd == I8_CMD_TEST_INDIRECT)
              {
                sem_post(&g_cmdsem);
              }
            break;
          case IEEE802154_NOTIFY_CONF_POLL:
            if (notif.u.pollconf.status == IEEE802154_STATUS_SUCCESS)
              {
                printf("i8sak: POLL.request succeeded\n");

                ret = read(fd, &rx, sizeof(struct mac802154dev_rxframe_s));
                if (ret < 0)
                  {
                    fprintf(stderr, "i8sak: read failed: %d\n", errno);
                    break;
                  }

                printf("i8sak: frame received:\n");

                for (i = 0; i < rx.length; i++)
                  {
                    printf("%02X", rx.payload[i]);
                  }

                printf(" \n");

                fflush(stdout);
              }
            else
              {
                printf("i8sak: POLL.request failed:  %d\n", notif.u.pollconf.status);
              }

            if (g_cmd == I8_CMD_TEST_POLL)
              {
                sem_post(&g_cmdsem);
              }
            break;
          case IEEE802154_NOTIFY_CONF_ASSOC:
            if (notif.u.assocconf.status == IEEE802154_STATUS_SUCCESS)
              {
                printf("i8sak: ASSOC.request succeeded\n");
              }
            else
              {
                printf("i8sak: ASSOC.request failed:  %d\n", notif.u.assocconf.status);
              }

            if (g_cmd == I8_CMD_TEST_ASSOC)
              {
                sem_post(&g_cmdsem);
              }
            break;
          case IEEE802154_NOTIFY_IND_ASSOC:
            /* When the next higher layer of a coordinator receives the
             * MLME-ASSOCIATE.indication primitive, the coordinator determines
             * whether to accept or reject the unassociated device using an
             * algorithm outside the scope of this standard.
             */

            if (g_cmd == I8_CMD_PANCOORD)
              {
                printf("i8sak: a device is trying to associate\n");

                /* If the address matches our device, accept the association.
                 * Otherwise, reject the assocation.
                 */

                if (memcmp(&notif.u.assocind.dev_addr[0], &g_dev.eaddr[0],
                           IEEE802154_EADDR_LEN) == 0)
                  {
                    /* Send a ASSOC.resp primtive to the MAC. Copy the association
                     * indication address into the association response primitive
                     */  

                    memcpy(&assocresp.dev_addr[0], &notif.u.assocind.dev_addr[0],
                           IEEE802154_EADDR_LEN);
                          
                    assocresp.assoc_saddr = I8_DEFAULT_DEV_SADDR;

                    assocresp.status = IEEE802154_STATUS_SUCCESS;

                    printf("i8sak: accepting association request\n");
                  }
                else
                  {
                    /* Send a ASSOC.resp primtive to the MAC. Copy the association
                     * indication address into the association response primitive
                     */ 

                    memcpy(&assocresp.dev_addr[0], &notif.u.assocind.dev_addr[0],
                           IEEE802154_EADDR_LEN);
                          
                    assocresp.status = IEEE802154_STATUS_DENIED;

                    printf("i8sak: rejecting association request\n");
                  }

                ieee802154_assoc_resp(fd, &assocresp);
              }
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

static int i8_tx(int fd, bool indirect)
{
  int ret;
  struct mac802154dev_txframe_s tx;

  /* Set an application defined handle */

  tx.meta.msdu_handle = g_handle++;

  /* This is a normal transaction, no special handling */

  tx.meta.msdu_flags.ack_tx = 0;
  tx.meta.msdu_flags.gts_tx = 0;
  tx.meta.msdu_flags.indirect_tx = indirect;

  tx.meta.ranging = IEEE802154_NON_RANGING;

  tx.meta.src_addrmode = IEEE802154_ADDRMODE_SHORT;
  tx.meta.dest_addr.mode = IEEE802154_ADDRMODE_SHORT;
  tx.meta.dest_addr.saddr = I8_DEFAULT_DEV_SADDR;
  tx.meta.dest_addr.panid = I8_DEFAULT_PANID;

  /* Each byte is represented by 2 chars */

  tx.length = g_txframe_len;
  tx.payload = &g_txframe[0];

  ret = write(fd, &tx, sizeof(struct mac802154dev_txframe_s));
  if (ret != OK)
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
  fprintf(stderr, "Usage: %s",
          "[-t <hex-payload>]"
          "[-b <period_ms> <hex payload>]"
          "[-s]"
          "[-r <test-name>]"
          "[-p]"
          "[-q]"
          ,progname);
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "    -t <hex-payload>: Transmit a frame with payload <hex-payload>\n");
  fprintf(stderr, "    -b <hex-payload> <period_ms>: Transmit data frame with payload"
                  " <hex-payload> every <period_ms> milliseconds.\n");
  fprintf(stderr, "    -s: Run sniffer\n");
  fprintf(stderr, "    -r: Run test <test-name>\n");
  fprintf(stderr, "    -p: Start PAN Coordinator");
  fprintf(stderr, "    -q: Quit any running command\n");
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
  static bool initialized = false;
  int ret, i;

  if (!initialized)
    {
      for (i = 0; i < IEEE802154_EADDR_LEN; i++)
        {
          g_coord.eaddr[i] = (uint8_t)((I8_DEFAULT_COORD_EADDR >> (i*8)) & 0xFF);
        }
      
      g_coord.saddr = I8_DEFAULT_COORD_SADDR;
      g_coord.panid = I8_DEFAULT_PANID;

      for (i = 0; i < IEEE802154_EADDR_LEN; i++)
        {
          g_dev.eaddr[i] = (uint8_t)((I8_DEFAULT_DEV_EADDR >> (i*8)) & 0xFF);
        }

      g_dev.saddr = IEEE802154_SADDR_UNSPEC;
      g_dev.panid = IEEE802154_PAN_UNSPEC;

      initialized = true;
    }

  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing devname\n");
      i8_showusage(argv[0], EXIT_FAILURE);
    }

  if (argc < 3)
    {
      fprintf(stderr, "ERROR: Missing command\n");
      i8_showusage(argv[0], EXIT_FAILURE);
    }

  g_devname = argv[1];

  if (strcmp(argv[2], "-t") == 0)
    {
      if (argc < 4)
        {
          fprintf(stderr, "i8sak: Missing payload\n");
          i8_showusage(argv[0], EXIT_FAILURE);
        }
      else
        {
          ret = i8_parse_payload(argv[3]);
          if (ret < 0)
            {
              fprintf(stderr, "ERROR:invalid hex payload\n", ret);
              i8_showusage(argv[0], EXIT_FAILURE);
            }
          i8_start_command(I8_CMD_TX);
        }
    }
  else if (strcmp(argv[2], "-b") == 0)
    {
      if (argc != 5)
        {
          fprintf(stderr, "i8sak: Invalid argument count\n");
          i8_showusage(argv[0], EXIT_FAILURE);
        }
      else
        {
          g_blaster_period = atoi(argv[3]);

          ret = i8_parse_payload(argv[4]);
          if (ret < 0)
            {
              fprintf(stderr, "ERROR:invalid hex payload\n", ret);
              i8_showusage(argv[0], EXIT_FAILURE);
            }

           i8_start_command(I8_CMD_BLASTER);
        }
    }
  else if (strcmp(argv[2], "-s") == 0) 
    {
      i8_start_command(I8_CMD_SNIFFER);
    }
  else if (strcmp(argv[2], "-r") == 0)
    {
      if (argc < 4)
        {
          fprintf(stderr, "i8sak: not enough arguments.", errno);
          i8_showusage(argv[0], EXIT_FAILURE);
        }

      if (strcmp(argv[3], "indirect") == 0)
        {
          i8_parse_payload(argv[4]);
          i8_start_command(I8_CMD_TEST_INDIRECT);
        }
      else if (strcmp(argv[3], "poll") == 0)
        {
          i8_start_command(I8_CMD_TEST_POLL);
        }
      else if (strcmp(argv[3], "assoc") == 0)
        {
          i8_start_command(I8_CMD_TEST_ASSOC);
        }
      else
        {
          fprintf(stderr, "i8sak: unknown test", errno);
          i8_showusage(argv[0], EXIT_FAILURE);
        }
    }
  else if (strcmp(argv[2], "-p") == 0)
    {
      i8_start_command(I8_CMD_PANCOORD);
    }
  else if (strcmp(argv[2], "-q") == 0)
    {
      i8_quit();
    }
  else
    {
      fprintf(stderr, "i8sak: invalid command\n");
      i8_showusage(argv[0], EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}
