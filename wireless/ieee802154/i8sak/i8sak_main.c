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
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <queue.h>
#include <sys/ioctl.h>
#include <nuttx/fs/ioctl.h>

#include "i8sak.h"

#include <nuttx/wireless/ieee802154/ieee802154_ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

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

static const struct i8sak_command_s g_i8sak_commands[] =
{
  {"help",        (CODE void *)NULL},
  {"startpan",    (CODE void *)i8sak_startpan_cmd},
  {"acceptassoc", (CODE void *)i8sak_acceptassoc_cmd},
  {"assoc",       (CODE void *)i8sak_assoc_cmd},
  {"scan",        (CODE void *)i8sak_scan_cmd},
  {"tx",          (CODE void *)i8sak_tx_cmd},
  {"poll",        (CODE void *)i8sak_poll_cmd},
  {"sniffer",     (CODE void *)i8sak_sniffer_cmd},
  {"blaster",     (CODE void *)i8sak_blaster_cmd},
  {"chan",        (CODE void *)i8sak_chan_cmd},
  {"coordinfo",   (CODE void *)i8sak_coordinfo_cmd},
  {"reset",       (CODE void *)i8sak_reset_cmd},
  {"regdump",     (CODE void *)i8sak_regdump_cmd},
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

static int i8sak_setup(FAR struct i8sak_s *i8sak, FAR const char *devname);
static int i8sak_daemon(int argc, FAR char *argv[]);
static int i8sak_showusage(FAR const char *progname, int exitcode);
static void i8sak_switch_instance(FAR char *devname);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_tx
 *
 * Description :
 *   Transmit a data frame.
 ****************************************************************************/

int i8sak_tx(FAR struct i8sak_s *i8sak, int fd)
{
  struct mac802154dev_txframe_s tx;
  int ret;

  /* Set an application defined handle */

  tx.meta.handle = i8sak->msdu_handle++;

  /* This is a normal transaction, no special handling */

  tx.meta.flags.ackreq = 1;
  tx.meta.flags.usegts = 0;

  tx.meta.flags.indirect = i8sak->indirect;

  if (i8sak->indirect)
    {
      if (i8sak->verbose)
        {
          printf("i8sak: queuing indirect transaction\n");
          fflush(stdout);
        }
    }
  else
    {
      if (i8sak->verbose)
        {
          printf("i8sak: queuing CSMA transaction\n");
          fflush(stdout);
        }
    }

  tx.meta.ranging = IEEE802154_NON_RANGING;

  tx.meta.srcmode = IEEE802154_ADDRMODE_SHORT;
  memcpy(&tx.meta.destaddr, &i8sak->ep, sizeof(struct ieee802154_addr_s));

  /* Each byte is represented by 2 chars */

  tx.length = i8sak->payload_len;
  tx.payload = &i8sak->payload[0];

  ret = write(fd, &tx, sizeof(struct mac802154dev_txframe_s));
  if (ret != OK)
    {
      printf(" write: errno=%d\n",errno);
    }

  return ret;
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

static void i8sak_switch_instance(FAR char *devname)
{
  FAR struct i8sak_s *i8sak;

  /* Search list of i8sak instances for one associated with the provided
   * device.
   */

  i8sak = (FAR struct i8sak_s *)sq_peek(&g_i8sak_instances);

  while (i8sak != NULL)
    {
      if (strcmp(devname, i8sak->devname) == 0)
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

      if (i8sak_setup(i8sak, devname) < 0)
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

static int i8sak_setup(FAR struct i8sak_s *i8sak, FAR const char *devname)
{
  char daemonname[I8SAK_DAEMONNAME_FMTLEN];
  int i;
  int ret;
  int fd;

  if (i8sak->initialized)
    {
      return OK;
    }

  i8sak->daemon_started = false;
  i8sak->daemon_shutdown = false;

  i8sak->chan = CONFIG_IEEE802154_I8SAK_CHNUM;
  i8sak->chpage = CONFIG_IEEE802154_I8SAK_CHPAGE;

  if (strlen(devname) > I8SAK_MAX_DEVNAME)
    {
      fprintf(stderr, "ERROR: devname too long\n");
      return ERROR;
    }

  strcpy(&i8sak->devname[0], devname);

  /* Initialze default extended address */

  for (i = 0; i < IEEE802154_EADDRSIZE; i++)
   {
     i8sak->addr.eaddr[i] = (uint8_t)((CONFIG_IEEE802154_I8SAK_DEV_EADDR >> (i*8)) & 0xFF);
   }

  /* Initialize the default remote endpoint address */

  i8sak->ep.mode = IEEE802154_ADDRMODE_SHORT;

  for (i = 0; i < IEEE802154_EADDRSIZE; i++)
   {
     i8sak->ep.eaddr[i] = (uint8_t)((CONFIG_IEEE802154_I8SAK_PANCOORD_EADDR >> (i*8)) & 0xFF);
   }

  for (i = 0; i < IEEE802154_SADDRSIZE; i++)
   {
     i8sak->ep.saddr[i] = (uint8_t)((CONFIG_IEEE802154_I8SAK_PANCOORD_SADDR >> (i*8)) & 0xFF);
   }

  for (i = 0; i < IEEE802154_PANIDSIZE; i++)
     {
       i8sak->ep.panid[i] = (uint8_t)((CONFIG_IEEE802154_I8SAK_PANID >> (i*8)) & 0xFF);
     }

  /* Set the next association device to the default device address, so that
   * the first device to request association gets that address.
   */

  for (i = 0; i < IEEE802154_SADDRSIZE; i++)
   {
     i8sak->next_saddr[i] = (uint8_t)((CONFIG_IEEE802154_I8SAK_DEV_SADDR >> (i*8)) & 0xFF);
   }

  fd = open(i8sak->devname, O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", i8sak->devname, errno);
      i8sak_cmd_error(i8sak);
    }

  ieee802154_seteaddr(fd, i8sak->addr.eaddr);

  close(fd);

  i8sak->addrset = false;

  i8sak->blasterperiod = CONFIG_IEEE802154_I8SAK_BLATER_PERIOD;

  sem_init(&i8sak->exclsem, 0, 1);

  sem_init(&i8sak->updatesem, 0, 0);
  sem_setprotocol(&i8sak->updatesem, SEM_PRIO_NONE);

  sem_init(&i8sak->sigsem, 0, 0);
  sem_setprotocol(&i8sak->sigsem, SEM_PRIO_NONE);

  /* Create strings for task based on device. i.e. i8_ieee0 */

  snprintf(daemonname, I8SAK_DAEMONNAME_FMTLEN, I8SAK_DAEMONNAME_FMT, &devname[5]);

  i8sak->daemon_pid = task_create(daemonname, CONFIG_IEEE802154_I8SAK_PRIORITY,
                                  CONFIG_IEEE802154_I8SAK_STACKSIZE, i8sak_daemon,
                                  NULL);
  if (i8sak->daemon_pid < 0)
    {
      fprintf(stderr, "failed to start daemon\n");
      return ERROR;
    }

  /* Use the signal semaphore to wait for daemon to start before returning */

  ret = sem_wait(&i8sak->sigsem);
  if (ret < 0)
    {
      fprintf(stderr, "i8sak:interrupted while daemon starting\n");
      return ERROR;
    }

  i8sak->initialized = true;
  return OK;
}

/****************************************************************************
 * Name : i8sak_daemon
 *
 * Description :
 *   Runs command in seperate task
 ****************************************************************************/

static int i8sak_daemon(int argc, FAR char *argv[])
{
  FAR struct i8sak_s *i8sak = g_activei8sak;
  int ret;

  fprintf(stderr, "i8sak: daemon started\n");
  i8sak->daemon_started = true;

  i8sak->fd = open(i8sak->devname, O_RDWR);
  if (i8sak->fd < 0)
    {
      printf("cannot open %s, errno=%d\n", i8sak->devname, errno);
      i8sak->daemon_started = false;
      ret = errno;
      return ret;
    }

  if (!i8sak->wpanlistener.is_setup)
    {
      wpanlistener_setup(&i8sak->wpanlistener, i8sak->fd);
    }

  wpanlistener_start(&i8sak->wpanlistener);

  /* Signal the calling thread that the daemon is up and running */

  sem_post(&i8sak->sigsem);

  while (!i8sak->daemon_shutdown)
    {
      if (i8sak->blasterenabled)
        {
          usleep(i8sak->blasterperiod*1000);
        }
      else
        {
          ret = sem_wait(&i8sak->updatesem);
          if (ret != OK)
            {
              break;
            }
        }

      if (i8sak->blasterenabled)
        {
          i8sak_tx(i8sak, i8sak->fd);
        }
    }

  wpanlistener_stop(&i8sak->wpanlistener);
  i8sak->daemon_started = false;
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
  fprintf(stderr, "Usage: %s\n"
          "    startpan [-h]\n"
          "    acceptassoc [-h|e <eaddr>]\n"
          "    scan [-h|p|a|e] minch-maxch\n"
          "    assoc [-h] [-w <count>] [<panid>]\n"
          "    tx [-h|d] <hex-payload>\n"
          "    poll [-h]\n"
          "    blaster [-h|q|f <hex payload>|p <period_ms>]\n"
          "    sniffer [-h|q]\n"
          "    chan [-h|g] [<chan>]\n"
          "    coordinfo [-h|a|e|s]\n"
          "    reset [-h]\n"
          "    regdump [-h]\n"
          , progname);
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

  /* Check if devname was included */

  if (argc > 1)
    {
      /* Check if argument starts with /dev/ */

      if (strncmp(argv[argind], "/dev/", 5) == 0)
        {
          i8sak_switch_instance(argv[argind]);
          argind++;

          if (argc == 2)
            {
              /* Close silently to allow user to set devname without any
               * other operation.
               */

              return EXIT_SUCCESS;
            }
        }

      /* Argument must be command */
    }

  /* If devname wasn't included, we need to check if our sticky feature has
   * ever been set.
   */

  else if (!g_activei8sak_set)
    {
      fprintf(stderr, "ERROR: Must include devname the first time you run\n");
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
