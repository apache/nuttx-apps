/****************************************************************************
 * apps/system/nxmbclient/nxmbclient_main.c
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nxmodbus/nxmb_client.h>
#include <nxmodbus/nxmodbus.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_TIMEOUT_MS 1000
#define DEFAULT_UNIT_ID    1
#define DEFAULT_TCP_PORT   502
#define DEFAULT_BAUDRATE   115200

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Modbus client type */

enum transport_type_e
{
  TRANSPORT_RTU = 0,
  TRANSPORT_ASCII,
  TRANSPORT_TCP
};

/* Modbus client configuration */

struct nxmbclient_config_s
{
  enum transport_type_e  transport;
  enum nxmb_parity_e     parity;
  FAR const char        *device;
  FAR const char        *host;
  uint32_t               baudrate;
  uint16_t               port;
  uint8_t                unit_id;
  uint32_t               timeout_ms;
  uint32_t               poll_ms;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_running = true;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: signal_handler
 ****************************************************************************/

static void signal_handler(int signo)
{
  g_running = false;
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("Usage: %s [OPTIONS] COMMAND [ARGS...]\n\n", progname);
  printf("Transport Options:\n");
  printf("  -t TYPE     Transport type: rtu, ascii, tcp (required)\n");
  printf("  -d DEVICE   Serial device path (for RTU/ASCII)\n");
  printf("  -b BAUD     Baud rate (default: %u)\n", DEFAULT_BAUDRATE);
  printf("  -p PARITY   Parity: none, even, odd (default: none)\n");
  printf("  -h HOST     TCP host address (for TCP)\n");
  printf("  -P PORT     TCP port (default: %u)\n", DEFAULT_TCP_PORT);
  printf("\nModbus Options:\n");
  printf("  -u UNIT     Unit ID (default: %u)\n", DEFAULT_UNIT_ID);
  printf("  -T TIMEOUT  Timeout in ms (default: %u)\n", DEFAULT_TIMEOUT_MS);
  printf("  --poll MS   Polling interval in ms (0 = one-shot)\n");
  printf("\nCommands:\n");
  printf("  read-coils ADDR COUNT\n");
  printf("  read-discrete ADDR COUNT\n");
  printf("  read-input ADDR COUNT\n");
  printf("  read-holding ADDR COUNT\n");
  printf("  write-coil ADDR VALUE\n");
  printf("  write-holding ADDR VALUE\n");
  printf("  write-coils ADDR VALUE...\n");
  printf("  write-holdings ADDR VALUE...\n");
}

/****************************************************************************
 * Name: parse_parity
 ****************************************************************************/

static int parse_parity(FAR const char *str, FAR enum nxmb_parity_e *parity)
{
  if (strcmp(str, "none") == 0)
    {
      *parity = NXMB_PAR_NONE;
    }
  else if (strcmp(str, "even") == 0)
    {
      *parity = NXMB_PAR_EVEN;
    }
  else if (strcmp(str, "odd") == 0)
    {
      *parity = NXMB_PAR_ODD;
    }
  else
    {
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Name: parse_transport
 ****************************************************************************/

static int parse_transport(FAR const char *str,
                           FAR enum transport_type_e *transport)
{
  if (strcmp(str, "rtu") == 0)
    {
      *transport = TRANSPORT_RTU;
    }
  else if (strcmp(str, "ascii") == 0)
    {
      *transport = TRANSPORT_ASCII;
    }
  else if (strcmp(str, "tcp") == 0)
    {
      *transport = TRANSPORT_TCP;
    }
  else
    {
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Name: cmd_read_coils
 ****************************************************************************/

static int cmd_read_coils(nxmb_handle_t handle, uint8_t unit_id,
                          int argc, FAR char **argv)
{
  FAR uint8_t *values;
  uint16_t     addr;
  uint16_t     count;
  uint16_t     nbytes;
  int          ret;
  int          i;

  if (argc < 2)
    {
      fprintf(stderr, "Error: read-coils requires ADDR COUNT\n");
      return -EINVAL;
    }

  addr   = (uint16_t)strtoul(argv[0], NULL, 0);
  count  = (uint16_t)strtoul(argv[1], NULL, 0);
  nbytes = (count + 7) / 8;

  values = malloc(nbytes);
  if (values == NULL)
    {
      return -ENOMEM;
    }

  memset(values, 0, nbytes);

  ret = nxmb_read_coils(handle, unit_id, addr, count, values);
  if (ret < 0)
    {
      fprintf(stderr, "Error: read-coils failed: %d\n", ret);
      free(values);
      return ret;
    }

  for (i = 0; i < count; i++)
    {
      printf("%u\t%u\n", addr + i,
             (values[i / 8] >> (i % 8)) & 1);
    }

  free(values);
  return OK;
}

/****************************************************************************
 * Name: cmd_read_discrete
 ****************************************************************************/

static int cmd_read_discrete(nxmb_handle_t handle, uint8_t unit_id,
                             int argc, FAR char **argv)
{
  FAR uint8_t *values;
  uint16_t     addr;
  uint16_t     count;
  uint16_t     nbytes;
  int          ret;
  int          i;

  if (argc < 2)
    {
      fprintf(stderr, "Error: read-discrete requires ADDR COUNT\n");
      return -EINVAL;
    }

  addr   = (uint16_t)strtoul(argv[0], NULL, 0);
  count  = (uint16_t)strtoul(argv[1], NULL, 0);
  nbytes = (count + 7) / 8;

  values = malloc(nbytes);
  if (values == NULL)
    {
      return -ENOMEM;
    }

  memset(values, 0, nbytes);

  ret = nxmb_read_discrete(handle, unit_id, addr, count, values);
  if (ret < 0)
    {
      fprintf(stderr, "Error: read-discrete failed: %d\n", ret);
      free(values);
      return ret;
    }

  for (i = 0; i < count; i++)
    {
      printf("%u\t%u\n", addr + i,
             (values[i / 8] >> (i % 8)) & 1);
    }

  free(values);
  return OK;
}

/****************************************************************************
 * Name: cmd_read_input
 ****************************************************************************/

static int cmd_read_input(nxmb_handle_t handle, uint8_t unit_id,
                          int argc, FAR char **argv)
{
  FAR uint16_t *values;
  uint16_t      addr;
  uint16_t      count;
  int           ret;
  int           i;

  if (argc < 2)
    {
      fprintf(stderr, "Error: read-input requires ADDR COUNT\n");
      return -EINVAL;
    }

  addr  = (uint16_t)strtoul(argv[0], NULL, 0);
  count = (uint16_t)strtoul(argv[1], NULL, 0);

  values = malloc(count * sizeof(uint16_t));
  if (values == NULL)
    {
      return -ENOMEM;
    }

  ret = nxmb_read_input(handle, unit_id, addr, count, values);
  if (ret < 0)
    {
      fprintf(stderr, "Error: read-input failed: %d\n", ret);
      free(values);
      return ret;
    }

  for (i = 0; i < count; i++)
    {
      printf("%u\t%u\n", addr + i, values[i]);
    }

  free(values);
  return OK;
}

/****************************************************************************
 * Name: cmd_read_holding
 ****************************************************************************/

static int cmd_read_holding(nxmb_handle_t handle, uint8_t unit_id,
                            int argc, FAR char **argv)
{
  FAR uint16_t *values;
  uint16_t      addr;
  uint16_t      count;
  int           ret;
  int           i;

  if (argc < 2)
    {
      fprintf(stderr, "Error: read-holding requires ADDR COUNT\n");
      return -EINVAL;
    }

  addr  = (uint16_t)strtoul(argv[0], NULL, 0);
  count = (uint16_t)strtoul(argv[1], NULL, 0);

  values = malloc(count * sizeof(uint16_t));
  if (values == NULL)
    {
      return -ENOMEM;
    }

  ret = nxmb_read_holding(handle, unit_id, addr, count, values);
  if (ret < 0)
    {
      fprintf(stderr, "Error: read-holding failed: %d\n", ret);
      free(values);
      return ret;
    }

  for (i = 0; i < count; i++)
    {
      printf("%u\t%u\n", addr + i, values[i]);
    }

  free(values);
  return OK;
}

/****************************************************************************
 * Name: cmd_write_coil
 ****************************************************************************/

static int cmd_write_coil(nxmb_handle_t handle, uint8_t unit_id,
                          int argc, FAR char **argv)
{
  uint16_t addr;
  uint8_t  value;
  int      ret;

  if (argc < 2)
    {
      fprintf(stderr, "Error: write-coil requires ADDR VALUE\n");
      return -EINVAL;
    }

  addr  = (uint16_t)strtoul(argv[0], NULL, 0);
  value = (uint8_t)strtoul(argv[1], NULL, 0);

  ret = nxmb_write_coil(handle, unit_id, addr, value);
  if (ret < 0)
    {
      fprintf(stderr, "Error: write-coil failed: %d\n", ret);
      return ret;
    }

  printf("OK\n");
  return OK;
}

/****************************************************************************
 * Name: cmd_write_holding
 ****************************************************************************/

static int cmd_write_holding(nxmb_handle_t handle, uint8_t unit_id,
                             int argc, FAR char **argv)
{
  uint16_t addr;
  uint16_t value;
  int      ret;

  if (argc < 2)
    {
      fprintf(stderr, "Error: write-holding requires ADDR VALUE\n");
      return -EINVAL;
    }

  addr  = (uint16_t)strtoul(argv[0], NULL, 0);
  value = (uint16_t)strtoul(argv[1], NULL, 0);

  ret = nxmb_write_holding(handle, unit_id, addr, value);
  if (ret < 0)
    {
      fprintf(stderr, "Error: write-holding failed: %d\n", ret);
      return ret;
    }

  printf("OK\n");
  return OK;
}

/****************************************************************************
 * Name: cmd_write_coils
 ****************************************************************************/

static int cmd_write_coils(nxmb_handle_t handle, uint8_t unit_id,
                           int argc, FAR char **argv)
{
  FAR uint8_t *values;
  uint16_t addr;
  uint16_t count;
  int ret;
  int i;

  if (argc < 2)
    {
      fprintf(stderr, "Error: write-coils requires ADDR VALUE...\n");
      return -EINVAL;
    }

  addr  = (uint16_t)strtoul(argv[0], NULL, 0);
  count = argc - 1;

  values = malloc(count);
  if (values == NULL)
    {
      return -ENOMEM;
    }

  for (i = 0; i < count; i++)
    {
      values[i] = (uint8_t)strtoul(argv[i + 1], NULL, 0);
    }

  ret = nxmb_write_coils(handle, unit_id, addr, count, values);
  if (ret < 0)
    {
      fprintf(stderr, "Error: write-coils failed: %d\n", ret);
      free(values);
      return ret;
    }

  free(values);
  printf("OK\n");
  return OK;
}

/****************************************************************************
 * Name: cmd_write_holdings
 ****************************************************************************/

static int cmd_write_holdings(nxmb_handle_t handle, uint8_t unit_id,
                              int argc, FAR char **argv)
{
  FAR uint16_t *values;
  uint16_t      addr;
  uint16_t      count;
  int           ret;
  int           i;

  if (argc < 2)
    {
      fprintf(stderr, "Error: write-holdings requires ADDR VALUE...\n");
      return -EINVAL;
    }

  addr  = (uint16_t)strtoul(argv[0], NULL, 0);
  count = argc - 1;

  values = malloc(count * sizeof(uint16_t));
  if (values == NULL)
    {
      return -ENOMEM;
    }

  for (i = 0; i < count; i++)
    {
      values[i] = (uint16_t)strtoul(argv[i + 1], NULL, 0);
    }

  ret = nxmb_write_holdings(handle, unit_id, addr, count, values);
  if (ret < 0)
    {
      fprintf(stderr, "Error: write-holdings failed: %d\n", ret);
      free(values);
      return ret;
    }

  free(values);
  printf("OK\n");
  return OK;
}

/****************************************************************************
 * Name: execute_command
 ****************************************************************************/

static int execute_command(nxmb_handle_t handle,
                           uint8_t unit_id,
                           FAR const char *cmd,
                           int argc,
                           FAR char **argv)
{
  if (strcmp(cmd, "read-coils") == 0)
    {
      return cmd_read_coils(handle, unit_id, argc, argv);
    }
  else if (strcmp(cmd, "read-discrete") == 0)
    {
      return cmd_read_discrete(handle, unit_id, argc, argv);
    }
  else if (strcmp(cmd, "read-input") == 0)
    {
      return cmd_read_input(handle, unit_id, argc, argv);
    }
  else if (strcmp(cmd, "read-holding") == 0)
    {
      return cmd_read_holding(handle, unit_id, argc, argv);
    }
  else if (strcmp(cmd, "write-coil") == 0)
    {
      return cmd_write_coil(handle, unit_id, argc, argv);
    }
  else if (strcmp(cmd, "write-holding") == 0)
    {
      return cmd_write_holding(handle, unit_id, argc, argv);
    }
  else if (strcmp(cmd, "write-coils") == 0)
    {
      return cmd_write_coils(handle, unit_id, argc, argv);
    }
  else if (strcmp(cmd, "write-holdings") == 0)
    {
      return cmd_write_holdings(handle, unit_id, argc, argv);
    }
  else
    {
      fprintf(stderr, "Error: unknown command '%s'\n", cmd);
      return -EINVAL;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmbclient_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct nxmbclient_config_s   config;
  struct nxmb_config_s         mb_config;
  nxmb_handle_t                handle = NULL;
  struct sigaction             sa;
  FAR const char              *cmd;
  int                          option;
  int                          ret;
  int                          cmd_argc;
  FAR char                   **cmd_argv;

  /* Initialize config with defaults */

  memset(&config, 0, sizeof(config));
  config.baudrate   = DEFAULT_BAUDRATE;
  config.parity     = NXMB_PAR_NONE;
  config.port       = DEFAULT_TCP_PORT;
  config.unit_id    = DEFAULT_UNIT_ID;
  config.timeout_ms = DEFAULT_TIMEOUT_MS;
  config.poll_ms    = 0;

  /* Parse options */

  while ((option = getopt(argc, argv, "t:d:b:p:h:P:u:T:-:")) != -1)
    {
      switch (option)
        {
          case 't':
            if (parse_transport(optarg, &config.transport) < 0)
              {
                fprintf(stderr, "Error: invalid transport '%s'\n", optarg);
                return EXIT_FAILURE;
              }
            break;

          case 'd':
            config.device = optarg;
            break;

          case 'b':
            config.baudrate = strtoul(optarg, NULL, 0);
            break;

          case 'p':
            if (parse_parity(optarg, &config.parity) < 0)
              {
                fprintf(stderr, "Error: invalid parity '%s'\n", optarg);
                return EXIT_FAILURE;
              }
            break;

          case 'h':
            config.host = optarg;
            break;

          case 'P':
            config.port = (uint16_t)strtoul(optarg, NULL, 0);
            break;

          case 'u':
            config.unit_id = (uint8_t)strtoul(optarg, NULL, 0);
            break;

          case 'T':
            config.timeout_ms = strtoul(optarg, NULL, 0);
            break;

          case '-':
            if (strcmp(optarg, "poll") == 0)
              {
                if (optind < argc)
                  {
                    config.poll_ms = strtoul(argv[optind++], NULL, 0);
                  }
                else
                  {
                    fprintf(stderr, "Error: --poll requires argument\n");
                    return EXIT_FAILURE;
                  }
              }
            else
              {
                fprintf(stderr, "Error: unknown option --%s\n", optarg);
                return EXIT_FAILURE;
              }
            break;

          default:
            show_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

  /* Check for command */

  if (optind >= argc)
    {
      fprintf(stderr, "Error: no command specified\n");
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  cmd       = argv[optind];
  cmd_argc  = argc - optind - 1;
  cmd_argv  = &argv[optind + 1];

  /* Validate transport-specific options */

  if (config.transport == TRANSPORT_RTU ||
      config.transport == TRANSPORT_ASCII)
    {
      if (config.device == NULL)
        {
          fprintf(stderr, "Error: -d DEVICE required for RTU/ASCII\n");
          return EXIT_FAILURE;
        }
    }
  else if (config.transport == TRANSPORT_TCP)
    {
      if (config.host == NULL)
        {
          fprintf(stderr, "Error: -h HOST required for TCP\n");
          return EXIT_FAILURE;
        }
    }

  /* Setup signal handler for clean shutdown */

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  /* Create Modbus context */

  memset(&mb_config, 0, sizeof(mb_config));
  mb_config.unit_id   = config.unit_id;
  mb_config.is_client = true;

  if (config.transport == TRANSPORT_RTU)
    {
      mb_config.mode                      = NXMB_MODE_RTU;
      mb_config.transport.serial.devpath  = config.device;
      mb_config.transport.serial.baudrate = config.baudrate;
      mb_config.transport.serial.parity   = config.parity;
    }
  else if (config.transport == TRANSPORT_ASCII)
    {
      mb_config.mode                      = NXMB_MODE_ASCII;
      mb_config.transport.serial.devpath  = config.device;
      mb_config.transport.serial.baudrate = config.baudrate;
      mb_config.transport.serial.parity   = config.parity;
    }
  else if (config.transport == TRANSPORT_TCP)
    {
      mb_config.mode               = NXMB_MODE_TCP;
      mb_config.transport.tcp.host = config.host;
      mb_config.transport.tcp.port = config.port;
    }

  ret = nxmb_create(&handle, &mb_config);
  if (ret < 0)
    {
      fprintf(stderr, "Error: failed to create Modbus context: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Enable context */

  ret = nxmb_enable(handle);
  if (ret < 0)
    {
      fprintf(stderr, "Error: failed to enable context: %d\n", ret);
      nxmb_destroy(handle);
      return EXIT_FAILURE;
    }

  /* Set timeout */

  ret = nxmb_set_timeout(handle, config.timeout_ms);
  if (ret < 0)
    {
      fprintf(stderr, "Error: failed to set timeout: %d\n", ret);
      nxmb_disable(handle);
      nxmb_destroy(handle);
      return EXIT_FAILURE;
    }

  /* Execute command (with optional polling) */

  if (config.poll_ms > 0)
    {
      /* Polling mode */

      while (g_running)
        {
          ret = execute_command(handle, config.unit_id, cmd, cmd_argc,
                               cmd_argv);
          if (ret < 0)
            {
              break;
            }

          usleep(config.poll_ms * 1000);
        }
    }
  else
    {
      /* One-shot mode */

      ret = execute_command(handle, config.unit_id, cmd, cmd_argc,
                             cmd_argv);
    }

  /* Cleanup */

  nxmb_disable(handle);
  nxmb_destroy(handle);

  return (ret < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
