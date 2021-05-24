/****************************************************************************
 * apps/system/ubloxmodem/ubloxmodem_main.c
 *
 *   Copyright (C) 2016 Vladimir Komendantskiy. All rights reserved.
 *   Author: Vladimir Komendantskiy <vladimir@moixaenergy.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nuttx/modem/u-blox.h>

#include "ubloxmodem.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_MODEM_U_BLOX_DEBUG
#  define m_err    _err
#  define m_warn   _llwarn
#  define m_info   _info
#else
#  define m_err(x...)
#  define m_warn(x...)
#  define m_info(x...)
#endif

#define UBLOXMODEM_MAX_REGISTERS 16

#if !defined(CONFIG_SYSTEM_UBLOXMODEM_TTY_DEVNODE)
/* Use /dev/ttyS1 by default */
#  define CONFIG_SYSTEM_UBLOXMODEM_TTY_DEVNODE  "/dev/ttyS1"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int ubloxmodem_help  (FAR struct ubloxmodem_cxt *cxt);
static int ubloxmodem_on    (FAR struct ubloxmodem_cxt *cxt);
static int ubloxmodem_off   (FAR struct ubloxmodem_cxt *cxt);
static int ubloxmodem_reset (FAR struct ubloxmodem_cxt *cxt);
static int ubloxmodem_status(FAR struct ubloxmodem_cxt *cxt);
static int ubloxmodem_at    (FAR struct ubloxmodem_cxt *cxt);

/* Mapping of command indices (@ubloxmodem_cmd@ implicit from the position in
 * the list) to tuples containing the command handler and descriptive
 * information.
 */

static const struct cmdinfo cmdmap[] =
{
  {ubloxmodem_help,   "help",   "Show help",   NULL},
  {ubloxmodem_on,     "on",     "Power ON",    NULL},
  {ubloxmodem_off,    "off",    "Power OFF",   NULL},
  {ubloxmodem_reset,  "reset",  "Reset",       NULL},
  {ubloxmodem_status, "status", "Show status", NULL},
  {ubloxmodem_at,     "at",     "AT test",     "<AT cmd> <response>"},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int make_nonblock(int fd)
{
  int flags;

  if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
    {
      return flags;
    }

  if ((flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0)
    {
      return flags;
    }

  return 0;
}

static int ubloxmodem_open_tty(void)
{
  int fd;
  int ret;

  fd = open(CONFIG_SYSTEM_UBLOXMODEM_TTY_DEVNODE, O_RDWR);
  if (fd < 0)
    {
      m_info("failed to open TTY\n");
      return fd;
    }

  ret = make_nonblock(fd);
  if (ret < 0)
    {
      m_info("make_nonblock failed\n");
      close(fd);
      return ret;
    }

  return fd;
}

static int chat_readb(int fd, FAR char *dst, int timeout_ms)
{
  struct pollfd fds;
  int ret;

  fds.fd = fd;
  fds.events = POLLIN;
  fds.revents = 0;

  ret = poll(&fds, 1, timeout_ms);
  if (ret <= 0)
    {
      m_info("poll timed out\n");
      return -ETIMEDOUT;
    }

  ret = read(fd, dst, 1);
  if (ret != 1)
    {
      m_info("read failed\n");
      return -EPERM;
    }

  return 0;
}

static int chat_match_response(int fd, FAR char *response, int timeout_ms)
{
  char c;
  int ret;
  int delim_countdown = 10;

  while (*response && delim_countdown >= 0)
    {
      ret = chat_readb(fd, &c, timeout_ms);
      if (ret < 0)
        {
          return ret;
        }

      if (c == *response)
        {
          response++;
        }
      else if (delim_countdown > 0 && (c == '\r' || c == '\n'))
        {
          delim_countdown--;
        }
      else
        {
          m_info("expected %c (0x%02X), got %c (0x%02X)\n",
                 *response, *response, c, c);
          return -EILSEQ;
        }
    }

  return 0;
}

static int chat_single(int fd, FAR char *cmd, FAR char *resp)
{
  int ret;

  /* Write the command */

  ret = write(fd, cmd, strlen(cmd));
  if (ret < 0)
    {
      return ret;
    }

  /* Terminate the command with <CR>, hence sending it to the modem */

  ret = write(fd, "\r", 1);
  if (ret < 0)
    {
      return ret;
    }

  /* Match the command echo */

  ret = chat_match_response(fd, cmd,  5 * 1000);
  if (ret < 0)
    {
      m_info("invalid echo\n");
      return ret;
    }

  /* Match the modem response to the command */

  ret = chat_match_response(fd, resp, 5 * 1000);
  return ret;
}

static int ubloxmodem_help(FAR struct ubloxmodem_cxt *cxt)
{
  int i;

  printf("Usage: ubloxmodem <cmd> [arguments]\n"
         "  where <cmd> is one of\n");
  for (i = 0;
       i < sizeof(cmdmap) / sizeof(struct cmdinfo);
       i++)
    {
      printf("%s\n  %s\n  %s\n",
             cmdmap[i].name,
             cmdmap[i].desc,
             (!cmdmap[i].args ? "No arguments" : cmdmap[i].args));
    }

  return 0;
}

static int ubloxmodem_on(FAR struct ubloxmodem_cxt *cxt)
{
  int ret;

  ret = ioctl(cxt->fd, MODEM_IOC_POWERON, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl failed: %d\n", errno);
      return -EPERM;
    }

  return ret;
}

static int ubloxmodem_off(FAR struct ubloxmodem_cxt *cxt)
{
  int ret;

  ret = ioctl(cxt->fd, MODEM_IOC_POWEROFF, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl failed: %d\n", errno);
      return -EPERM;
    }

  return ret;
}

static int ubloxmodem_reset(FAR struct ubloxmodem_cxt *cxt)
{
  int ret;

  ret = ioctl(cxt->fd, MODEM_IOC_RESET, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl failed: %d\n", errno);
      return -EPERM;
    }

  return ret;
}

static int ubloxmodem_status(FAR struct ubloxmodem_cxt *cxt)
{
  int ret, i;
  struct ubxmdm_status status;

  /* Allocate name-value pairs */

  FAR struct ubxmdm_regval register_values[UBLOXMODEM_MAX_REGISTERS];
  char regname[4];   /* Null-terminated string buffer */

  regname[3] = '\0'; /* Set the null string terminator */

  /* Set the maximum value, to be updated by driver */

  status.register_values_size = UBLOXMODEM_MAX_REGISTERS;
  status.register_values      = register_values;

  ret = ioctl(cxt->fd, MODEM_IOC_GETSTATUS, (unsigned long) &status);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  printf("Modem is %s\n", status.on ? "ON" : "OFF");
  for (i = 0;
       i < status.register_values_size && i < UBLOXMODEM_MAX_REGISTERS;
       i++)
    {
      strncpy(regname, status.register_values[i].name, 3);
      printf("%s=%d ",
             regname,
             (int) status.register_values[i].val);
    }

  printf("\n");
  return ret;
}

static int ubloxmodem_at(FAR struct ubloxmodem_cxt *cxt)
{
  int fd, ret;
  FAR char *atcmd;
  FAR char *resp;

  atcmd = cxt->argv[2];
  resp  = cxt->argv[3];

  if (cxt->argc < 4 || atcmd == NULL || resp == NULL)
    {
      fprintf(stderr, "ERROR: missing arguments\n");
      return -EINVAL;
    }

  fd = ubloxmodem_open_tty();
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: cannot open TTY device: %d\n", errno);
      return fd;
    }

  ret = chat_single(fd, atcmd, resp);

  m_info("test result: %d\n", ret);

  close(fd);
  return ret;
}

static int ubloxmodem_parse(FAR struct ubloxmodem_cxt *cxt)
{
  int i;

  for (i = 0;
       i < sizeof(cmdmap) / sizeof(struct cmdinfo) &&
         cxt->cmd == UBLOXMODEM_CMD_UNKNOWN;
       i++)
    {
      if (!strcmp(cxt->argv[1], cmdmap[i].name))
        cxt->cmd = i;
    }

  if (cxt->cmd == UBLOXMODEM_CMD_UNKNOWN)
    {
      cxt->cmd = UBLOXMODEM_CMD_HELP;
    }

  return 0;
}

static int ubloxmodem_exec(FAR struct ubloxmodem_cxt *cxt)
{
  return (cmdmap[cxt->cmd].handler)(cxt);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ubloxmodem_main
 *
 * Description:
 *   Main entry point for the u-blox modem tool.
 *
 ****************************************************************************/

int main(int argc, FAR char** argv)
{
  struct ubloxmodem_cxt cxt;
  int ret;

  cxt.argc = argc;
  cxt.argv = argv;
  cxt.cmd  = UBLOXMODEM_CMD_UNKNOWN;

  ubloxmodem_parse(&cxt);

  cxt.fd = open(CONFIG_SYSTEM_UBLOXMODEM_DEVNODE, O_RDWR);
  if (cxt.fd < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_SYSTEM_UBLOXMODEM_DEVNODE, errno);
      return EXIT_FAILURE;
    }

  ret = ubloxmodem_exec(&cxt);
  printf("Command result: %s (%d)\n", ret ? "FAIL" : "OK", ret);

  fflush(stdout);
  close(cxt.fd);

  return 0;
}
