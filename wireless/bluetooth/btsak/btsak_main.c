/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_main.c
 * Bluetooth Swiss Army Knife
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author:  Gregory Nutt <gnutt@nuttx.org>
 *
 * Based loosely on the i8sak IEEE 802.15.4 program by Anthony Merlino and
 * Sebastien Lorquet.  Commands inspired for btshell example in the
 * Intel/Zephyr Arduino 101 package (BSD license).
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

#include <nuttx/wireless/bt_ioctl.h>
#include <nuttx/net/bluetooth.h>

#include "btsak.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if !defined(CONFIG_BTSAK_NINSTANCES) || CONFIG_BTSAK_NINSTANCES <= 0
#  undef CONFIG_BTSAK_NINSTANCES
#  define CONFIG_BTSAK_NINSTANCES 3
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Describes one command */

struct btsak_command_s
{
  FAR const char *name;
  CODE void (*handler)(FAR struct btsak_s *btsak, int argc, FAR char *argv[]);
  FAR const char *help;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

void btsak_cmd_help(FAR struct btsak_s *btsak, int argc, FAR char *argv[]);
void btsak_cmd_gatt(FAR struct btsak_s *btsak, int argc, FAR char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Alphabetical, except for help */

static const struct btsak_command_s g_btsak_commands[] =
{
  {
    "help",
    (CODE void *)btsak_cmd_help,
    NULL
  },
  {"scan",
    (CODE void *)btsak_cmd_scan,
    "[-h] <on|off>"
  },
  {
    "advertise",
    (CODE void *)btsak_cmd_advertise,
    "[-h] <on|off>"
  },
  {
    "security",
    (CODE void *)btsak_cmd_security,
    "[-h] <addr> <addr-type> <level>"
  },
  {
    "gatt",
    (CODE void *)btsak_cmd_gatt,
    "[-h] <cmd> [option [option [option...]]]"
  }
};

#define NCOMMANDS (sizeof(g_btsak_commands) / sizeof(struct btsak_command_s))

static const struct btsak_command_s g_btsak_gatt_commands[] =
{
  {"exchange-mtu",
    (CODE void *)btsak_cmd_gatt_exchange_mtu,
    "[-h] <addr> <addr-type>"
  },
  {
    "discover",
    (CODE void *)btsak_cmd_discover,
    "[-h] <addr> <addr-type> <uuid-type>"
  },
  {
    "characteristic",
    (CODE void *)btsak_cmd_gatt_discover_characteristc,
    "[-h] <addr> <addr-type>"
  },
  {
    "descriptor",
    (CODE void *)btsak_cmd_gat_discover_descriptor,
    "[-h] <addr> <addr-type>"
  },
  {
    "read",
    (CODE void *)btsak_cmd_gatt_read,
    "[-h] <addr> <addr-type> <handle> [<offset>]"
  },
  {
    "read-multiple",
    (CODE void *)btsak_cmd_gatt_read_multiple,
    "[-h] <addr> <addr-type> <handle> <nitems>"
  },
  {
    "write",
    (CODE void *)btsak_cmd_gatt_write,
    "[-h] <addr> <addr-type> <handle> <datum>"
  }
};

#define GATT_NCOMMANDS (sizeof(g_btsak_gatt_commands) / sizeof(struct btsak_command_s))

static const bt_addr_t g_default_epaddr =
{
  BTSAK_DEFAULT_EPADDR
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_help
 *
 * Description:
 *   Handle help command.
 *
 ****************************************************************************/

void btsak_cmd_help(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
  btsak_showusage(btsak->progname, EXIT_SUCCESS);
}

/****************************************************************************
 * Name: btsak_cmd_gatt
 *
 * Description:
 *   Handle gatt command.
 *
 ****************************************************************************/

void btsak_cmd_gatt(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
  FAR const struct btsak_command_s *cmd;
  int argind;
  int i;

  /* Verify that a command was provided */

  argind = 1;
  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing gatt command\n");
      btsak_showusage(btsak->progname, EXIT_FAILURE);
    }

  /* Check for help */

  if (strcmp(argv[argind], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  /* Find the command in the g_btsak_gatt_commands[] list */

  cmd = NULL;
  for (i = 0; i < GATT_NCOMMANDS; i++)
    {
      FAR const struct btsak_command_s *gattcmd = &g_btsak_gatt_commands[i];
      if (strcmp(argv[argind], gattcmd->name) == 0)
        {
          cmd = gattcmd;
          break;
        }
    }

  if (cmd == NULL)
    {
      fprintf(stderr, "ERROR: Unrecognized gatt command: %s\n", argv[argind]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  cmd->handler(btsak, argc - argind, &argv[argind]);
}

/****************************************************************************
 * bt_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int bt_main(int argc, char *argv[])
#endif
{
  FAR const struct btsak_command_s *cmd;
  struct btsak_s btsak;
  int argind;
  int i;

  /* Initialize the default remote endpoint address */

  memset(&btsak, 0, sizeof(struct btsak_s));
  BLUETOOTH_ADDRCOPY(btsak.ep_btaddr.val, g_default_epaddr.val);
  btsak.progname = argv[0];

  /* Check if ifname was included */

  argind = 1;
  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing interface name\n");
      btsak_showusage(btsak.progname, EXIT_FAILURE);
    }

  btsak.ifname = argv[argind];
  argind++;

  if (argc < 3)
    {
      fprintf(stderr, "ERROR: Missing command\n");
      btsak_showusage(btsak.progname, EXIT_FAILURE);
    }

  /* Find the command in the g_btsak_command[] list */

  cmd = NULL;
  for (i = 0; i < NCOMMANDS; i++)
    {
      FAR const struct btsak_command_s *btcmd = &g_btsak_commands[i];
      if (strcmp(argv[argind], btcmd->name) == 0)
        {
          cmd = btcmd;
          break;
        }
    }

  if (cmd == NULL)
    {
      fprintf(stderr, "ERROR: Unrecognized command: %s\n", argv[argind]);
      btsak_showusage(btsak.progname, EXIT_FAILURE);
    }

  cmd->handler(&btsak, argc - argind, &argv[argind]);
  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: btsak_char2nibble
 *
 * Description:
 *   Convert an hexadecimal character to a 4-bit nibble.
 *
 ****************************************************************************/

uint8_t btsak_char2nibble(char ch)
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
 * Name: btsak_str2long
 *
 * Description:
 *   Convert a hex string to an integer value
 *
 ****************************************************************************/

long btsak_str2long(FAR const char *str)
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
 * Name: btsak_str2luint8
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

uint8_t btsak_str2luint8(FAR const char *str)
{
  long value = btsak_str2long(str);
  if (value < 0 || value > UINT8_MAX)
    {
      fprintf(stderr, "ERROR: 8-bit value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (uint8_t)value;
}

/****************************************************************************
 * Name: btsak_str2luint16
 *
 * Description:
 *   Convert a string to an integer value
 *
 ****************************************************************************/

uint16_t btsak_str2luint16(FAR const char *str)
{
  long value = btsak_str2long(str);
  if (value < 0 || value > UINT16_MAX)
    {
      fprintf(stderr, "ERROR: 16-bit value out of range\n");
      exit(EXIT_FAILURE);
    }

  return (uint16_t)value;
}

/****************************************************************************
 * Name: btsak_str2bool
 *
 * Description:
 *   Convert a boolean name to a boolean value.
 *
 ****************************************************************************/

bool btsak_str2bool(FAR const char *str)
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
 * Name : btsak_str2payload
 *
 * Description :
 *   Parse string to get buffer of data. Buf is expected to be of size
 *   BLUETOOTH_SMP_MTU or larger.
 *
 * Returns:
 *   Positive length value of frame payload
 ****************************************************************************/

int btsak_str2payload(FAR const char *str, FAR uint8_t *buf)
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

  if ((str_len & 1) || (ret > BLUETOOTH_SMP_MTU))
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
 * Name: btsak_str2addr
 *
 * Description:
 *   Convert a string 8-byte EADDR array.
 *
 ****************************************************************************/

void btsak_str2addr(FAR const char *str, FAR uint8_t *addr)
{
  FAR const char *src = str;
  uint8_t bvalue;
  char ch;
  int i;

  for (i = 0; i < 8; i++)
    {
      ch = (char)*src++;
      bvalue = btsak_char2nibble(ch) << 4;

      ch = (char)*src++;
      bvalue |= btsak_char2nibble(ch);

      *addr++ = bvalue;

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
 * Name: btsak_socket
 *
 * Description:
 *   Create a socket on the selected device.
 *
 ****************************************************************************/

int btsak_socket(FAR struct btsak_s *btsak)
{
  int sockfd = -1;

  /* Create the socket with the correct addressing */

  BLUETOOTH_ADDRCOPY(btsak->ep_btaddr.val, g_default_epaddr.val);

#if defined(CONFIG_NET_BLUETOOTH)
  btsak->ep_sockaddr.bt_family   = AF_BLUETOOTH;
  btsak->ep_sockaddr.bt_channel  = 0;  /* REVISIT */
  BLUETOOTH_ADDRCOPY(btsak->ep_sockaddr.bt_bdaddr.val, btsak->ep_btaddr.val);

  sockfd = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP);

#elif defined(CONFIG_NET_6LOWPAN)
  btsak->ep_sockaddr.sin6_family = AF_INET6;
  btsak->ep_sockaddr.sin6_port   = HTONS(CONFIG_BTSAK_DEFAULT_PORT);
  btsak_update_ipv6addr(btsak);

  sockfd = socket(PF_INET6, SOCK_DGRAM, 0);

#endif

  if (sockfd < 0)
    {
      fprintf(stderr, "ERROR: failed to create socket, errno=%d\n", errno);
    }

  return sockfd;
}

/****************************************************************************
 * Name: btsak_showusage
 *
 * Description:
 *   Show program usage.
 *
 ****************************************************************************/

void btsak_showusage(FAR const char *progname, int exitcode)
{
  int i;

  fprintf(stderr, "\nUsage:\n\n");
  fprintf(stderr, "\t%s progname <ifname> <cmd> [option [option [option...]]]\n",
          progname);
  fprintf(stderr, "\nWhere <cmd> [option [option [option...]]] is one of:\n\n");

  for (i = 0; i < NCOMMANDS; i++)
    {
      FAR const struct btsak_command_s *cmd = &g_btsak_commands[i];
      if (cmd->help != NULL)
        {
          fprintf(stderr, "\t%s\t%s\n", cmd->name, cmd->help);
        }
      else
        {
          fprintf(stderr, "\t%s\n", cmd->name);
        }
    }

  fprintf(stderr, "\nUse the -h option on any command to get more info.\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: btsak_gatt_showusage
 *
 * Description:
 *   Show gatt command usage.
 *
 ****************************************************************************/

void btsak_gatt_showusage(FAR const char *progname, FAR const char *cmd,
                          int exitcode)
{
  int i;

  fprintf(stderr, "%s:  Generic Attribute (GATT) commands:\n", cmd);
  fprintf(stderr, "Usage:\n\n");
  fprintf(stderr, "  %s <ifname> %s [-h] <cmd> [option [option [option...]]]\n",
          progname, cmd);
  fprintf(stderr, "\nWhere <cmd> [option [option [option...]]] is one of:\n\n");

  for (i = 0; i < GATT_NCOMMANDS; i++)
    {
      FAR const struct btsak_command_s *gattcmd = &g_btsak_gatt_commands[i];
      if (gattcmd->help != NULL)
        {
          fprintf(stderr, "\t%s\t%s\n", gattcmd->name, gattcmd->help);
        }
      else
        {
          fprintf(stderr, "\t%s\n", gattcmd->name);
        }
    }

  fprintf(stderr, "\nUse the -h option on any command to get more info.\n");
  exit(exitcode);
}
