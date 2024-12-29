/****************************************************************************
 * apps/examples/dac/dac_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/param.h>
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

#define DEFAULT_COUNT          1
#define DEFAULT_DELAY          0
#define DEFAULT_CHANNEL        0
#define DEFAULT_TEST_BIT_RES   8
#define DEFAULT_TEST_STEP      8
#define DEFAULT_INITIALIZED    true

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct command
{
  FAR const char *name;
  CODE int (* const cmd)(int argc, FAR const char *argv[]);
  FAR const char *args;
};

struct dac_state_s
{
  FAR char *devpath;
  int count;
  int delay;
  uint8_t channel;
  int test_bit_res;
  int test_step;
  bool initialized;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int cmd_dac_put(int argc, FAR const char *argv[]);
static int cmd_dac_putv(int argc, FAR const char *argv[]);
static int cmd_dac_test(int argc, FAR const char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct command commands[] =
{
  { "put", cmd_dac_put, "DATA [DELAY]" },
  { "putv", cmd_dac_putv, "DATA [DATA...]" },
  { "test", cmd_dac_test, "" },
};

static struct dac_state_s g_dacstate;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dac_devpath
 *
 * Description:
 *   Update the DAC device path in the DAC state structure. This function
 *   manages memory allocation for the device path and frees the previous
 *   path if it exists.
 *
 * Input Parameters:
 *   dac     - Pointer to the DAC state structure.
 *   devpath - New DAC device path to be set.
 *
 * Operation:
 *   - Frees the existing DAC device path if it exists.
 *   - Allocates memory for the new DAC device path.
 *   - Updates the device path in the DAC state structure.
 *
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

/****************************************************************************
 * Name: print_cmds
 *
 * Description:
 *   Print a list of commands with their names, arguments, and a given header
 *   and trailer. This function is used to display available
 *   commands and their usage information.
 *
 * Input Parameters:
 *   header  - Header string to be printed before the command list.
 *   cmds    - Pointer to an array of command structures.
 *   ncmds   - Number of commands in the array.
 *   trailer - Trailer string to be printed after the command list.
 *
 * Operation:
 *   - Prints the header.
 *   - Iterates through the array of command structures and prints
 *     their names, arguments, and a newline character after each command.
 *   - Prints the trailer.
 *
 ****************************************************************************/

static void print_cmds(FAR const char *header,
                       FAR const struct command *cmds,
                       size_t ncmds,
                       FAR const char *trailer)
{
  printf("%s", header);
  while (ncmds--)
    {
      printf("  %s %s %c", cmds->name, cmds->args,
                           (ncmds > 0) ? '\n' : ' ');
      cmds++;
    }

  printf("%s", trailer);
}

/****************************************************************************
 * Name: find_cmd
 *
 * Description:
 *   Find a command structure in an array of commands by comparing command
 *   names.
 *
 * Input Parameters:
 *   name  - Name of the command to be found.
 *   cmds  - Pointer to an array of command structures.
 *   ncmds - Number of commands in the array.
 *
 * Returned Value:
 *   Returns a pointer to the found command structure if successful; NULL
 *   if the command is not found.
 *
 * Operation:
 *   - Iterates through the array of command structures.
 *   - Compares the name of each command with the specified name.
 *   - Returns a pointer to the found command structure or NULL if not found.
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: execute_cmd
 *
 * Description:
 *   Execute a command function based on the command name.
 *   This function finds the corresponding command structure in the
 *   array and invokes the associated function.
 *
 * Input Parameters:
 *   argc  - Number of command-line arguments.
 *   argv  - Array of command-line argument strings.
 *   cmds  - Pointer to an array of command structures.
 *   ncmds - Number of commands in the array.
 *
 * Returned Value:
 *   Returns the result of the executed command function.
 *
 * Operation:
 *   - Finds the command structure for the specified command name.
 *   - Invokes the associated command function with the provided arguments.
 *   - Prints an error message if the command is not found.
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: dac_put
 *
 * Description:
 *   Write DAC messages to the specified DAC device. This function writes
 *   multiple DAC messages, allowing the configuration of DAC channel and
 *   data values. It handles potential write retries in case of non-blocking
 *   write operations and provides debug information about the write process.
 *
 * Input Parameters:
 *   devpath - Path to the DAC device.
 *   msg     - Pointer to an array of DAC messages to be written.
 *   nmsgs   - Number of DAC messages in the array.
 *   delay   - Delay between writes in milliseconds.
 *
 * Returned Value:
 *   Returns the number of successfully written messages on success; a
 *   negated errno value on failure.
 *
 * Operation:
 *   - Opens the specified DAC device for writing.
 *   - Iterates through the array of DAC messages and writes
 *       them to the device.
 *   - Handles non-blocking writes, retries on EAGAIN, and provides
 *       debug output.
 *   - Closes the DAC device after completing the write operations.
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: cmd_dac_put
 *
 * Description:
 *   Execute the 'put' command, writing a specified DAC value to the DAC
 *   device.
 *
 * Input Parameters:
 *   argc - Number of command-line arguments.
 *   argv - Array of command-line argument strings.
 *
 * Returned Value:
 *   Returns OK on success; a negated errno value on failure.
 *
 * Operation:
 *   - Parses the command-line arguments to extract the DAC value and delay.
 *   - Overrides the "sticky" delay option if provided in the arguments.
 *   - Repeats the DAC write operation for the specified number of times.
 *
 * Usage:
 *  `put [value] [delay]`: Write the specified DAC value to the device.
 *  - [value]: DAC value to be written (Default: 100).
 *  - [delay]: Delay between writes in milliseconds.
 *
 ****************************************************************************/

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
      ret = dac_put(g_dacstate.devpath, msgs, nitems(msgs), delay);
      printf("ret=%d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: cmd_dac_putv
 *
 * Description:
 *   Execute the 'putv' command, writing DAC values specified in the command
 *   arguments to the DAC device. The command supports writing
 *   multiple values in a single execution.
 *
 * Input Parameters:
 *   argc - Number of command-line arguments.
 *   argv - Array of command-line argument strings.
 *
 * Returned Value:
 *   Returns OK on success; a negated errno value on failure.
 *
 * Operation:
 *   - Parses the command-line arguments to extract DAC values.
 *   - Constructs DAC messages with the specified channel and data values.
 *   - Repeats the DAC write operation for the specified number of times.
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: cmd_dac_test
 *
 * Description:
 *   Perform DAC testing by generating a sequence of values and writing them
 *   to the DAC device specified in the global DAC state structure.
 *
 * Input Parameters:
 *   argc - Number of arguments in argv.
 *   argv - Array of command-line argument strings.
 *
 * Returned Value:
 *   Total return value, the sum of return values from write calls. Returns
 *   a negative errno on failure.
 *
 * Note: Utilizing the parameters can make testing much easier. If you are
 *   testing with one config test, you can change the constant definition for
 *   default value at the top and simply run `nsh> dat test`
 * If you want setup for multimeter testing, set appropriate delay between
 *   writes (in ms). Example for 8 bit DAC using step in value of 16
 *   nsh> dac -d 5000 -b 8 -s 16 test
 * Example for oscilloscope test for 12 bit DAC:
 *   nsh> dac -d 10 -b 12 -s 4 test
 *
 ****************************************************************************/

static int cmd_dac_test(int argc, FAR const char *argv[])
{
  struct dac_msg_s msg;
  int fd;
  int i;
  bool count_up;
  uint32_t value = 0;
  uint32_t max_value = (1 << g_dacstate.test_bit_res);
  int ret = OK;
  int total_ret = 0;

  fd = open(g_dacstate.devpath, O_WRONLY | O_NONBLOCK);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open() failed: %d\n", errno);
      return -ENODEV;
    }

  for (i = 0; i < g_dacstate.count; i++)
    {
      count_up = true;
      value = 0;
      while (count_up || value)
        {
          msg.am_channel = g_dacstate.channel;
          msg.am_data = value;

          if (count_up)
            {
              value += g_dacstate.test_step;
              if (value >= max_value - 1)
                {
                  value = max_value - 1;
                  count_up = false;
                }
            }
          else
            {
              value -= g_dacstate.test_step;

              /* Check if value is underflow */

              if (value > UINT32_MAX - g_dacstate.test_step)
                {
                  value = 0;
                }
            }

            ret = write(fd, &msg, sizeof(struct dac_msg_s));
            total_ret += ret;
            usleep(1000 * g_dacstate.delay);
        }
    }

  close(fd);

  return total_ret;
}

/****************************************************************************
 * Name: dac_help
 *
 * Description:
 *   Display the help message for the DAC testing application.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void dac_help(void)
{
  printf("Usage: dac [OPTIONS] command [CMD OPTIONS]\n");
  printf("\nGlobal arguments are \"sticky\".  "
         "For example, once the DAC device is\n");
  printf("specified, that device will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-c channel] selects the DAC channel.  "
         "Default: %d Current: %d\n", DEFAULT_CHANNEL, g_dacstate.channel);
  printf("  [-d delay] pause for DELAY ms between writes.  "
         "Default: %d Current: %d\n", DEFAULT_DELAY, g_dacstate.delay);
  printf("  [-n count] repeats device write COUNT times.  "
         "Default: %d Current: %d\n", DEFAULT_COUNT, g_dacstate.count);
  printf("  [-b bit_resolution] Test-specific config - set DAC bit"
         " resolution  Default: %d Current: %d\n",
         DEFAULT_TEST_BIT_RES, g_dacstate.test_bit_res);
  printf("  [-s step] Test-specific config - test loop step  "
         "Default: %d Current: %d\n",
         DEFAULT_TEST_STEP, g_dacstate.test_step);
  printf("  [-p devpath] selects the DAC device.  "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_DAC_DEVPATH,
         g_dacstate.devpath ? g_dacstate.devpath : "NONE");
  printf("  [-h] Print this help and exit.\n");
  print_cmds("\nCommands:\n", commands, nitems(commands), "\n");
}

/****************************************************************************
 * Name: arg_string
 *
 * Description:
 *   Extracts a string argument from a command-line option.
 *
 * Input Parameters:
 *   arg   - Pointer to the current command-line argument being processed.
 *   value - Pointer to store the extracted string argument.
 *
 * Returned Value:
 *   Integer representing the number of arguments consumed
 *   (2 if the option takes an additional argument, 1 otherwise).
 *
 ****************************************************************************/

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

/****************************************************************************
 * Name: arg_decimal
 *
 * Description:
 *   Extracts a decimal value from a command-line option.
 *
 * Input Parameters:
 *   arg   - Pointer to the current command-line argument being processed.
 *   value - Pointer to store the extracted decimal value.
 *
 * Returned Value:
 *   Integer representing the number of arguments consumed
 *   (2 if the option takes an additional argument, 1 otherwise).
 *
 * Note:
 *   Utilizes arg_string internally to extract the string argument
 *   and then converts it to a long integer using strtol.
 *
 ****************************************************************************/

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
  FAR const char *ptr; /* Pointer to the current argument in argv */
  FAR const char *str; /* Pointer to a string argument extracted from argv */
  long value;          /* Numerical argument value */
  int nargs;           /* Number of args consumed by the current option */
  int n;               /* Total number of arguments processed */
  int i;               /* Loop counter */

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

          case 'b':
            nargs = arg_decimal(&argv[i], &value);
            if (value < 0)
              {
                printf("Bit resolution must be non-negative: %ld\n", value);
                exit(1);
              }

            dac->test_bit_res = (uint32_t)value;
            i += nargs;
            n += nargs;
            break;

          case 's':
            nargs = arg_decimal(&argv[i], &value);
            if (value < 0)
              {
                printf("Step must be non-negative: %ld\n", value);
                exit(1);
              }

            dac->test_step = (uint32_t)value;
            i += nargs;
            n += nargs;
            break;

          case 'p':
            nargs = arg_string(&argv[i], &str);
            dac_devpath(dac, str);
            i += nargs;
            n += nargs;
            break;

          case 'h':
            dac_help();
            exit(0);
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

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Entry point for the DAC testing application.
 *   Initializes the DAC hardware and processes command-line
 *   arguments to execute DAC testing commands.
 *
 * Input Parameters:
 *   argc - Number of command-line arguments.
 *   argv - Array of command-line argument strings.
 *
 * Returned Value:
 *   EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 *
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
      g_dacstate.delay         = DEFAULT_DELAY;
      g_dacstate.count         = DEFAULT_COUNT;
      g_dacstate.channel       = DEFAULT_CHANNEL;
      g_dacstate.test_bit_res  = DEFAULT_TEST_BIT_RES;
      g_dacstate.test_step     = DEFAULT_TEST_STEP;
      g_dacstate.initialized   = DEFAULT_INITIALIZED;
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
      ret = execute_cmd(argc, argv, commands, nitems(commands));
    }

  return (ret >= 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
