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
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <queue.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <nuttx/fs/ioctl.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include <nuttx/wireless/ieee802154/ieee802154_device.h>
#include "wireless/ieee802154.h"

#include "i8sak.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#if !defined(CONFIG_IEEE802154_I8SAK_NINSTANCES) || CONFIG_IEEE802154_I8SAK_NINSTANCES <= 0
#  undef CONFIG_IEEE802154_I8SAK_NINSTANCES
#  define CONFIG_IEEE802154_I8SAK_NINSTANCES 3
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Describes one command */

struct i8sak_command_s
{
  FAR const char *name;
  CODE void (*handler)(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Alphabetical, except for help */

static const struct i8sak_command_s g_i8sak_commands[] =
{
  {"help",        (CODE void *)NULL},
  {"acceptassoc", (CODE void *)i8sak_acceptassoc_cmd},
  {"assoc",       (CODE void *)i8sak_assoc_cmd},
  {"blaster",     (CODE void *)i8sak_blaster_cmd},
  {"get",         (CODE void *)i8sak_get_cmd},
  {"poll",        (CODE void *)i8sak_poll_cmd},
  {"regdump",     (CODE void *)i8sak_regdump_cmd},
  {"reset",       (CODE void *)i8sak_reset_cmd},
  {"scan",        (CODE void *)i8sak_scan_cmd},
  {"set",         (CODE void *)i8sak_set_cmd},
  {"sniffer",     (CODE void *)i8sak_sniffer_cmd},
  {"startpan",    (CODE void *)i8sak_startpan_cmd},
  {"tx",          (CODE void *)i8sak_tx_cmd},
};

#define NCOMMANDS (sizeof(g_i8sak_commands) / sizeof(struct i8sak_command_s))

static sq_queue_t g_i8sak_free;
static sq_queue_t g_i8sak_instances;
static struct i8sak_s g_i8sak_pool[CONFIG_IEEE802154_I8SAK_NINSTANCES];
static bool g_i8sak_initialized = false;
static bool g_activei8sak_set = false;
static FAR struct i8sak_s *g_activei8sak;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int i8sak_setup(FAR struct i8sak_s *i8sak, FAR const char *ifname);
static int i8sak_daemon(int argc, FAR char *argv[]);
static int i8sak_showusage(FAR const char *progname, int exitcode);
static void i8sak_switch_instance(FAR char *ifname);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_requestdaemon
 *
 * Description :
 *
 ****************************************************************************/

int i8sak_requestdaemon(FAR struct i8sak_s *i8sak)
{
  char daemonname[I8SAK_MAX_DAEMONNAME];
  int ret;
  int len;

  ret = sem_wait(&i8sak->daemonlock);
  if (ret < 0)
    {
      DEBUGASSERT(ret == EINTR);
      return ret;
    }

  if (++i8sak->daemonusers == 1)
    {
      /* Create strings for task based on device. i.e. i8_ieee0 */

      if (i8sak->mode == I8SAK_MODE_CHAR)
        {
          len = strlen(i8sak->ifname);
          snprintf(daemonname, I8SAK_DAEMONNAME_PREFIX_LEN + (len - 5),
                   I8SAK_DAEMONNAME_FMT, &i8sak->ifname[5]);
        }
#ifdef CONFIG_NET_6LOWPAN
      else if (i8sak->mode == I8SAK_MODE_NETIF)
        {
          len = strlen(i8sak->ifname);
          snprintf(daemonname, I8SAK_DAEMONNAME_PREFIX_LEN + len,
                   I8SAK_DAEMONNAME_FMT, i8sak->ifname);
        }
#endif

      i8sak->daemon_shutdown = false;
      i8sak->daemon_pid = task_create(daemonname, CONFIG_IEEE802154_I8SAK_PRIORITY,
                                      CONFIG_IEEE802154_I8SAK_STACKSIZE, i8sak_daemon,
                                      NULL);
      if (i8sak->daemon_pid < 0)
        {
          fprintf(stderr, "failed to start daemon\n");
          sem_post(&i8sak->daemonlock);
          return ERROR;
        }

      /* Use the signal semaphore to wait for daemon to start before returning */

      ret = sem_wait(&i8sak->sigsem);
      if (ret < 0)
        {
          fprintf(stderr, "i8sak:interrupted while daemon starting\n");
          sem_post(&i8sak->daemonlock);
          return ret;
        }
    }

  sem_post(&i8sak->daemonlock);
  return 0;
}

/****************************************************************************
 * Name : i8sak_releasedaemon
 *
 * Description :
 *
 ****************************************************************************/

int i8sak_releasedaemon(FAR struct i8sak_s *i8sak)
{
  int ret;

  ret = sem_wait(&i8sak->daemonlock);
  if (ret < 0)
    {
      DEBUGASSERT(ret == EINTR);
      return ret;
    }

  /* If this was the last user, signal the daemon to shutdown */

  if (--i8sak->daemonusers == 0)
    {
      i8sak->daemon_shutdown = true;
      sem_post(&i8sak->updatesem);
    }

  sem_post(&i8sak->daemonlock);
  return 0;
}

/****************************************************************************
 * Name : i8sak_str2payload
 *
 * Description :
 *   Parse string to get buffer of data. Buf is expected to be of size
 *   IEEE802154_MAX_MAC_PAYLOAD_SIZE or larger.
 *
 * Returns:
 *   Positive length value of frame payload
 ****************************************************************************/

int i8sak_str2payload(FAR const char *str, FAR uint8_t *buf)
{
  int str_len;
  int ret;
  int i = 0;

  str_len = strlen(str);

  /* Each byte is represented by 2 chars */

  ret = str_len >> 1;

  /* Check if the number of chars is a multiple of 2 and that the number of
   * bytes does not exceed the max MAC frame payload supported.
   */

  if ((str_len & 1) || (ret > IEEE802154_MAX_MAC_PAYLOAD_SIZE))
    {
      fprintf(stderr, "ERROR: Invalid payload\n");
      exit(EXIT_FAILURE);
    }

  /* Decode hex packet */

  while (str_len > 0)
    {
      int dat;
      if (sscanf(str, "%2x", &dat) == 1)
        {
          buf[i++] = dat;
          str += 2;
          str_len -= 2;
        }
      else
        {
          fprintf(stderr, "ERROR: Invalid payload\n");
          exit(EXIT_FAILURE);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: i8sak_str2long
 *
 * Description:
 *   Convert a hex string to an integer value
 *
 ****************************************************************************/

long i8sak_str2long(FAR const char *str)
{
  FAR char *endptr;
  long value;

  value = strtol(str, &endptr, 0);
  if (*endptr != '\0')
    {
      fprintf(stderr, "ERROR: Garbage after numeric argument\n");
      exit(EXIT_FAILURE);
    }

  if (value > INT_MAX || value < INT_MIN)
    {
      fprintf(stderr, "ERROR: Integer value out of range\n");
      return LONG_MAX;
      exit(EXIT_FAILURE);
    }

  return value;
}

/****************************************************************************
 * Name: i8sak_str2luint8
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

uint8_t i8sak_str2luint8(FAR const char *str)
{
  long value = i8sak_str2long(str);
  if (value < 0 || value > UINT8_MAX)
    {
      fprintf(stderr, "ERROR: 8-bit value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (uint8_t)value;
}

/****************************************************************************
 * Name: i8sak_str2luint16
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

uint16_t i8sak_str2luint16(FAR const char *str)
{
  long value = i8sak_str2long(str);
  if (value < 0 || value > UINT16_MAX)
    {
      fprintf(stderr, "ERROR: 16-bit value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (uint16_t)value;
}

/****************************************************************************
 * Name: i8sak_char2nibble
 *
 * Description:
 *   Convert an hexadecimal character to a 4-bit nibble.
 *
 ****************************************************************************/

uint8_t i8sak_char2nibble(char ch)
{
  if (ch >= '0' && ch <= '9')
    {
      return ch - '0';
    }
  else if (ch >= 'a' && ch <= 'f')
    {
      return ch - 'a' + 10;
    }
  else if (ch >= 'A' && ch <= 'F')
    {
      return ch - 'A' + 10;
    }
  else if (ch == '\0')
    {
      fprintf(stderr, "ERROR: Unexpected end hex\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      fprintf(stderr, "ERROR: Unexpected character in hex value: %02x\n", ch);
      exit(EXIT_FAILURE);
    }
}

/****************************************************************************
 * Name: i8sak_str2eaddr
 *
 * Description:
 *   Convert a string 8-byte EADDR array.
 *
 ****************************************************************************/

void i8sak_str2eaddr(FAR const char *str, FAR uint8_t *eaddr)
{
  FAR const char *src = str;
  uint8_t bvalue;
  char ch;
  int i;

  for (i = 0; i < 8; i++)
    {
      ch = (char)*src++;
      bvalue = i8sak_char2nibble(ch) << 4;

      ch = (char)*src++;
      bvalue |= i8sak_char2nibble(ch);

      *eaddr++ = bvalue;

      if (i < 7)
        {
          ch = (char)*src++;
          if (ch != ':')
            {
              fprintf(stderr, "ERROR: Missing colon separator: %s\n", str);
              fprintf(stderr, "       Expected xx:xx:xx:xx:xx:xx:xx:xx\n");
              exit(EXIT_FAILURE);
            }
        }
    }
}

/****************************************************************************
 * Name: i8sak_str2saddr
 *
 * Description:
 *   Convert a string 2-byte SADDR array.
 *
 ****************************************************************************/

void i8sak_str2saddr(FAR const char *str, FAR uint8_t *saddr)
{
  FAR const char *src = str;
  uint8_t bvalue;
  char ch;
  int i;

  for (i = 0; i < 2; i++)
    {
      ch = (char)*src++;
      bvalue = i8sak_char2nibble(ch) << 4;

      ch = (char)*src++;
      bvalue |= i8sak_char2nibble(ch);

      *saddr++ = bvalue;

      if (i < 1)
        {
          ch = (char)*src++;
          if (ch != ':')
            {
              fprintf(stderr, "ERROR: Missing colon separator: %s\n", str);
              fprintf(stderr, "       Expected xx:xx\n");
              exit(EXIT_FAILURE);
            }
        }
    }
}

/****************************************************************************
 * Name: i8sak_str2panid
 *
 * Description:
 *   Convert a string 2-byte PAN ID array.
 *
 ****************************************************************************/

void i8sak_str2panid(FAR const char *str, FAR uint8_t *panid)
{
  FAR const char *src = str;
  uint8_t bvalue;
  char ch;
  int i;

  for (i = 0; i < 2; i++)
    {
      ch = (char)*src++;
      bvalue = i8sak_char2nibble(ch) << 4;

      ch = (char)*src++;
      bvalue |= i8sak_char2nibble(ch);

      *panid++ = bvalue;

      if (i < 1)
        {
          ch = (char)*src++;
          if (ch != ':')
            {
              fprintf(stderr, "ERROR: Missing colon separator: %s\n", str);
              fprintf(stderr, "       Expected xx:xx\n");
              exit(EXIT_FAILURE);
            }
        }
    }
}

/****************************************************************************
 * Name: i8sak_str2bool
 *
 * Description:
 *   Convert a boolean name to a boolean value.
 *
 ****************************************************************************/

bool i8sak_str2bool(FAR const char *str)
{
  if (strcasecmp(str, "true") == 0)
    {
      return true;
    }
  else if (strcasecmp(str, "false") == 0)
    {
      return false;
    }
  else
    {
      fprintf(stderr, "ERROR: Invalid boolean name: %s\n", str);
      fprintf(stderr, "       Expected true or false\n");
      exit(EXIT_FAILURE);
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void i8sak_switch_instance(FAR char *ifname)
{
  FAR struct i8sak_s *i8sak;

  /* Search list of i8sak instances for one associated with the provided
   * device.
   */

  i8sak = (FAR struct i8sak_s *)sq_peek(&g_i8sak_instances);

  while (i8sak != NULL)
    {
      if (strcmp(ifname, i8sak->ifname) == 0)
        {
          break;
        }

      i8sak = (FAR struct i8sak_s *)sq_next((FAR sq_entry_t *)i8sak);
    }

  /* If there isn't a i8sak instance started for this device, allocate one */

  if (i8sak == NULL)
    {
      i8sak = (FAR struct i8sak_s *)sq_remfirst(&g_i8sak_free);
      if (i8sak == NULL)
        {
          fprintf(stderr, "ERROR: Failed to allocate i8sak instance\n");
          exit(EXIT_FAILURE);
        }

      sq_addlast((FAR sq_entry_t *)i8sak, &g_i8sak_instances);

      /* Update our "sticky" i8sak instance. Must come before call to setup so that
       * the shared active global i8sak is correct.
       */

      g_activei8sak = i8sak;

      if (i8sak_setup(i8sak, ifname) < 0)
        {
          exit(EXIT_FAILURE);
        }
    }
  else
    {
      g_activei8sak = i8sak;
    }

  g_activei8sak_set = true;
}

static int i8sak_setup(FAR struct i8sak_s *i8sak, FAR const char *ifname)
{
  int i;
  int fd = 0;

  if (i8sak->initialized)
    {
      return OK;
    }

  if (strlen(ifname) > I8SAK_MAX_IFNAME)
    {
      fprintf(stderr, "ERROR: ifname too long\n");
      return ERROR;
    }
  strcpy(&i8sak->ifname[0], ifname);

  i8sak->chan = 11;
  i8sak->chpage = 0;
  i8sak->addrmode = IEEE802154_ADDRMODE_SHORT;

  i8sak->ep_addr.mode = IEEE802154_ADDRMODE_SHORT;

  /* Initialize the default remote endpoint address */

  for (i = 0; i < IEEE802154_EADDRSIZE; i++)
   {
     i8sak->ep_addr.eaddr[i] =
       (uint8_t)((CONFIG_IEEE802154_I8SAK_DEFAULT_EP_EADDR >> (i*8)) & 0xFF);
   }

  for (i = 0; i < IEEE802154_SADDRSIZE; i++)
    {
      i8sak->ep_addr.saddr[i] =
        (uint8_t)((CONFIG_IEEE802154_I8SAK_DEFAULT_EP_SADDR >> (i*8)) & 0xFF);
    }

  for (i = 0; i < IEEE802154_PANIDSIZE; i++)
    {
      i8sak->ep_addr.panid[i] =
        (uint8_t)((CONFIG_IEEE802154_I8SAK_DEFAULT_EP_PANID >> (i*8)) & 0xFF);
    }

#ifdef CONFIG_NET_6LOWPAN
  i8sak->ep_in6addr.sin6_family = AF_INET6;
  i8sak->ep_in6addr.sin6_port = HTONS(CONFIG_IEEE802154_I8SAK_DEFAULT_PORT);

  i8sak_update_ep_ip(i8sak);

  i8sak->snifferport = HTONS(CONFIG_IEEE802154_I8SAK_DEFAULT_PORT);
#endif

  /* Set the next association device to the default device address, so that
   * the first device to request association gets that address.
   */

  for (i = 0; i < IEEE802154_SADDRSIZE; i++)
   {
     i8sak->next_saddr[i] =
        (uint8_t)(((CONFIG_IEEE802154_I8SAK_DEFAULT_EP_SADDR + 1) >> (i*8)) & 0xFF);
   }

   /* Check if argument starts with /dev/ */

  if (strncmp(ifname, "/dev/", 5) == 0)
    {
      i8sak->mode = I8SAK_MODE_CHAR;
    }
  else
    {
      i8sak->mode = I8SAK_MODE_NETIF;
    }

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      fd = open(i8sak->ifname, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n", i8sak->ifname, errno);
          i8sak_cmd_error(i8sak);
        }
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      fd = socket(PF_INET6, SOCK_DGRAM, 0);
      if (fd < 0)
        {
          fprintf(stderr, "ERROR: failed to open socket, errno=%d\n", errno);
          i8sak_cmd_error(i8sak);
        }
    }
#endif
  else
    {
      close(fd);
      return ERROR;
    }

  close(fd);

  sem_init(&i8sak->exclsem, 0, 1);

  sem_init(&i8sak->updatesem, 0, 0);
  sem_setprotocol(&i8sak->updatesem, SEM_PRIO_NONE);

  sem_init(&i8sak->sigsem, 0, 0);
  sem_setprotocol(&i8sak->sigsem, SEM_PRIO_NONE);

  sem_init(&i8sak->eventsem, 0, 1);

  sem_init(&i8sak->daemonlock, 0, 1);

  i8sak->daemonusers = 0;

  i8sak->eventlistener_run = false;
  sq_init(&i8sak->eventreceivers);
  sq_init(&i8sak->eventreceivers_free);
  for (i = 0; i < CONFIG_I8SAK_NEVENTRECEIVERS; i++)
    {
      sq_addlast((FAR sq_entry_t *)&i8sak->eventreceiver_pool[i], &i8sak->eventreceivers_free);
    }

  i8sak->blasterperiod = 1000;

  i8sak->initialized = true;
  return OK;
}

/****************************************************************************
 * Name : i8sak_daemon
 *
 * Description :
 *   Runs command in separate task
 ****************************************************************************/

static int i8sak_daemon(int argc, FAR char *argv[])
{
  FAR struct i8sak_s *i8sak = g_activei8sak;
  int ret;

  fprintf(stderr, "i8sak: daemon started\n");

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      i8sak->fd = open(i8sak->ifname, O_RDWR);
      if (i8sak->fd < 0)
        {
          fprintf(stderr, "ERROR: cannot open %s, errno=%d\n", i8sak->ifname, errno);
          ret = errno;
          return ret;
        }
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      i8sak->fd = socket(PF_INET6, SOCK_DGRAM, 0);
      if (i8sak->fd < 0)
        {
          fprintf(stderr, "ERROR: failed to open socket, errno=%d\n", errno);
          ret = errno;
          return ret;
        }
    }
#endif

  i8sak_eventlistener_start(i8sak);

  /* Signal the calling thread that the daemon is up and running */

  sem_post(&i8sak->sigsem);

  while (!i8sak->daemon_shutdown)
    {
      ret = sem_wait(&i8sak->updatesem);
      if (ret != OK)
        {
          continue;
        }

      if (i8sak->startblaster)
        {
          i8sak->blasterenabled = true;
          i8sak->startblaster = false;

          ret = pthread_create(&i8sak->blaster_threadid, NULL, i8sak_blaster_thread,
                               (void *)i8sak);
          if (ret != 0)
            {
              fprintf(stderr, "failed to start blaster thread: %d\n", ret);
              return ret;
            }
        }

      if (i8sak->startsniffer)
        {
          i8sak->snifferenabled = true;
          i8sak->startsniffer = false;

          ret = pthread_create(&i8sak->sniffer_threadid, NULL, i8sak_sniffer_thread,
                               (void *)i8sak);
          if (ret != 0)
            {
              fprintf(stderr, "failed to start sniffer thread: %d\n", ret);
              return ret;
            }
        }
    }

  i8sak_eventlistener_stop(i8sak);
  close(i8sak->fd);
  printf("i8sak: daemon closing\n");
  return OK;
}

/****************************************************************************
 * Name: i8sak_showusage
 *
 * Description:
 *   Show program usage.
 *
 ****************************************************************************/

static int i8sak_showusage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage (Use the -h option on any command to get more info): %s\n"
          "    acceptassoc [-h|e]\n"
          "    assoc [-h|p|e|s|w|r|t]\n"
          "    blaster [-h|q|f|p]\n"
          "    get [-h] parameter\n"
          "    poll [-h]\n"
          "    regdump [-h]\n"
          "    reset [-h]\n"
          "    scan [-h|p|a|e] minch-maxch\n"
          "    set [-h] param val\n"
          "    sniffer [-h|d]\n"
          "    startpan [-h|b|s] xx:xx\n"
          "    tx [-h|d|m] <hex-payload>\n"
          , progname);
  exit(exitcode);
}

/****************************************************************************
 * i8_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const struct i8sak_command_s *i8sakcmd;
  int argind;
  int ret;
  int i;

  if (!g_i8sak_initialized)
    {
      sq_init(&g_i8sak_free);
      sq_init(&g_i8sak_instances);
      for (i = 0; i < CONFIG_IEEE802154_I8SAK_NINSTANCES; i++)
        {
          sq_addlast((FAR sq_entry_t *)&g_i8sak_pool[i], &g_i8sak_free);
          g_i8sak_pool[i].initialized = false;
        }

      g_i8sak_initialized = true;
    }

  argind = 1;

  /* Check if ifname was included */

  if (argc > 1)
    {
      /* Check if argument starts with /dev/ */

      if ((strncmp(argv[argind], "/dev/", 5) == 0) ||
          (strncmp(argv[argind], "wpan", 4) == 0))
        {

          i8sak_switch_instance(argv[argind]);
          argind++;

          if (argc == 2)
            {
              /* Close silently to allow user to set ifname without any
               * other operation.
               */

              return EXIT_SUCCESS;
            }
        }

      /* Argument must be command */
    }

  /* If ifname wasn't included, we need to check if our sticky feature has
   * ever been set.
   */

  if (!g_activei8sak_set)
    {
      fprintf(stderr, "ERROR: Must include ifname the first time you run\n");
      i8sak_showusage(argv[0], EXIT_FAILURE);
    }

  /* Check to make sure the user supplied a command */

  if (argc <= argind)
    {
      fprintf(stderr, "ERROR: Must include a command\n");
      i8sak_showusage(argv[0], EXIT_FAILURE);
    }

  /* Find the command in the g_i8sak_command[] list */

  i8sakcmd = NULL;
  for (i = 0; i < NCOMMANDS; i++)
    {
      FAR const struct i8sak_command_s *cmd = &g_i8sak_commands[i];
      if (strcmp(argv[argind], cmd->name) == 0)
        {
          i8sakcmd = cmd;
          break;
        }
    }

  if (i8sakcmd == NULL)
    {
      i8sak_showusage(argv[0], EXIT_FAILURE);
    }

  /* Special case the help command which has no handler */

  if (i8sakcmd->handler == NULL)
    {
      i8sak_showusage(argv[0], EXIT_SUCCESS);
    }

  ret = sem_wait(&g_activei8sak->exclsem);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to lock i8sak instance\n");
      exit(EXIT_FAILURE);
    }

  i8sakcmd->handler(g_activei8sak, argc - argind, &argv[argind]);

  sem_post(&g_activei8sak->exclsem);
  return EXIT_SUCCESS;
}
