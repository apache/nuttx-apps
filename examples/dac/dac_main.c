/****************************************************************************
 * apps/examples/dac/dac_main.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Juha Niskanen <juha.niskanen@haltian.com>
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <nuttx/analog/dac.h>
#include <nuttx/arch.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_DAC
#  error "DAC device support is not enabled (CONFIG_DAC)"
#endif

#ifndef CONFIG_EXAMPLES_DAC_DEVPATH
#  define CONFIG_EXAMPLES_DAC_DEVPATH "/dev/dac0"
#endif

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct command
{
  FAR const char *name;
  CODE int (* const cmd)(int argc, const char *argv[]);
  const char *args;
};

struct dac_state_s
{
  FAR char *devpath;
  int count;
  int delay;
  uint8_t channel;
  bool initialized;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int cmd_dac_put(int argc, FAR const char *argv[]);
static int cmd_dac_putv(int argc, FAR const char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct command commands[] =
{
  { "put", cmd_dac_put, "DATA [DELAY]" },
  { "putv", cmd_dac_putv, "DATA [DATA...]" },
};

static struct dac_state_s g_dacstate;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void dac_devpath(FAR struct dac_state_s *dac,
                        FAR const char *devpath)
{
  if (dac->devpath)
    {
      free(dac->devpath);
    }

  dac->devpath = strdup(devpath);
}

static void print_cmds(FAR const char *header,
                       FAR const struct command *cmds,
                       size_t ncmds,
                       FAR const char *trailer)
{
  printf(header);
  while (ncmds--)
    {
      printf("  %s %s %c", cmds->name, cmds->args,
                           (ncmds > 0) ? '\n' : ' ');
      cmds++;
    }

  printf(trailer);
}

static const struct command *find_cmd(FAR const char *name,
                                      FAR const struct command *cmds,
                                      size_t ncmds)
{
  while (ncmds--)
    {
      if (!strcmp(cmds->name, name))
        {
          return cmds;
        }

      cmds++;
    }

  return NULL;
}

static int execute_cmd(int argc,
                       FAR const char *argv[],
                       FAR const struct command *cmds,
                       size_t ncmds)
{
  FAR const struct command *cmd;

  cmd = find_cmd(argv[0], cmds, ncmds);
  if (!cmd)
    {
      fprintf(stderr, "ERROR: unknown command: %s\n", argv[0]);
      print_cmds("", cmds, ncmds, "\n");
      return -EINVAL;
    }

  return cmd->cmd(argc - 1, argv + 1);
}

static int dac_put(FAR const char *devpath,
                   FAR struct dac_msg_s *msg,
                   size_t nmsgs,
                   int delay)
{
  size_t retries;
  size_t i;
  int fd;
  int ret = OK;

  fd = open(devpath, O_WRONLY | O_NONBLOCK);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open() failed: %d\n", errno);
      return -ENODEV;
    }

  i = retries = 0;
  while (i < nmsgs && retries < 3)
    {
      errno = 0;
      ret = write(fd, &msg[i], sizeof(*msg));
      printf("write: ret=%d, errno=%d\n", ret, errno);
      if (ret != sizeof(*msg))
        {
          if (errno == EAGAIN)
            {
              retries++;
              continue;
            }

          fprintf(stderr, "write failed: ret=%d, errno=%d\n", ret, errno);
          ret = -errno;
          break;
        }
      else
        {
          printf("wrote: chan=%d, data=%" PRId32 "\n", msg[i].am_channel,
                 msg[i].am_data);
          i++;
          retries = 0;
          ret = OK;
          usleep(1000 * delay);
        }
    }

  close(fd);
  return (i > 0) ? i : ret;
}

static int cmd_dac_put(int argc, FAR const char *argv[])
{
  struct dac_msg_s msgs[1];
  int data;
  int delay;
  int i;
  int ret = OK;

  /* This command allows overriding the "sticky" delay option. */

  data  = (argc > 0) ? atoi(argv[0]) : 100;
  delay = (argc > 1) ? atoi(argv[1]) : g_dacstate.delay;

  printf("devpath=%s data=%d delay=%d\n",
        g_dacstate.devpath, data, delay);

  for (i = 0; i < g_dacstate.count; i++)
    {
      msgs[0].am_channel = g_dacstate.channel;
      msgs[0].am_data = data;
      ret = dac_put(g_dacstate.devpath, msgs, ARRAY_SIZE(msgs), delay);
      printf("ret=%d\n", ret);
    }

  return ret;
}

static int cmd_dac_putv(int argc, FAR const char *argv[])
{
  struct dac_msg_s msgs[CONFIG_NSH_MAXARGUMENTS];
  int nmsgs;
  int i;
  int ret = OK;

  for (nmsgs = 0; nmsgs < CONFIG_NSH_MAXARGUMENTS; nmsgs++)
    {
      if (nmsgs >= argc)
        {
          break;
        }

      msgs[nmsgs].am_channel = g_dacstate.channel;
      msgs[nmsgs].am_data = atoi(argv[nmsgs]);
    }

  for (i = 0; i < g_dacstate.count; i++)
    {
      ret = dac_put(g_dacstate.devpath, msgs, nmsgs, g_dacstate.delay);
      printf("ret=%d\n", ret);
    }

  return ret;
}

static void dac_help(void)
{
  printf("Usage: dac [OPTIONS] command [CMD OPTIONS]\n");
  printf("\nGlobal arguments are \"sticky\".  "
         "For example, once the DAC device is\n");
  printf("specified, that device will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-c channel] selects the DAC channel.  "
         "Default: 0 Current: %d\n", g_dacstate.channel);
  printf("  [-d delay] pause for DELAY ms between writes.  "
         "Default: 0 Current: %d\n", g_dacstate.delay);
  printf("  [-n count] repeats device write COUNT times.  "
         "Default: 1 Current: %d\n", g_dacstate.count);
  printf("  [-p devpath] selects the DAC device.  "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_DAC_DEVPATH,
         g_dacstate.devpath ? g_dacstate.devpath : "NONE");
  print_cmds("\nCommands:\n", commands, ARRAY_SIZE(commands), "\n");
}

static int arg_string(FAR const char **arg, FAR const char **value)
{
  FAR const char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}

static int arg_decimal(FAR const char **arg, FAR long *value)
{
  FAR const char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

static int parse_args(FAR struct dac_state_s *dac,
                      int argc,
                      const char *argv[])
{
  FAR const char *ptr;
  FAR const char *str;
  long value;
  int nargs;
  int n;
  int i;

  for (i = n = 1; i < argc; )
    {
      ptr = argv[i];
      if (ptr[0] != '-')
        {
          i++;
          continue;
        }

      switch (ptr[1])
        {
          case 'c':
            nargs = arg_decimal(&argv[i], &value);
            if (value < 0 || value > 255)
              {
                printf("Channel must be in range [0..255]: %ld\n", value);
                exit(1);
              }

            dac->channel = (uint8_t)value;
            i += nargs;
            n += nargs;
            break;

          case 'd':
            nargs = arg_decimal(&argv[i], &value);
            if (value < 0)
              {
                printf("Delay must be non-negative: %ld\n", value);
                exit(1);
              }

            dac->delay = (uint32_t)value;
            i += nargs;
            n += nargs;
            break;

          case 'n':
            nargs = arg_decimal(&argv[i], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            dac->count = (uint32_t)value;
            i += nargs;
            n += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[i], &str);
            dac_devpath(dac, str);
            i += nargs;
            n += nargs;
            break;

          case '?':
          default:
            printf("Unsupported option: %s\n", ptr);
            dac_help();
            exit(1);
        }
    }

  return n;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR const char *argv[])
{
  int ret;
  int nargs = 1;

  if (!g_dacstate.initialized)
    {
      /* Initialization of the DAC hardware must be performed by
       * board-specific logic prior to running this test.
       */

      /* Set the default values */

      dac_devpath(&g_dacstate, CONFIG_EXAMPLES_DAC_DEVPATH);
      g_dacstate.count       = 1;
      g_dacstate.channel     = 0; /* This seems to be ignored by driver. */
      g_dacstate.initialized = true;
    }

  /* Parse the command line */

  nargs = parse_args(&g_dacstate, argc, argv);

  argc -= nargs;
  argv += nargs;

  if (argc < 1)
    {
      dac_help();
      return EXIT_FAILURE;
    }
  else
    {
      ret = execute_cmd(argc, argv, commands, ARRAY_SIZE(commands));
    }

  return (ret >= 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
