/****************************************************************************
 * apps/examples/nxmbserver/nxmbserver_main.c
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

#include <nxmodbus/nxmodbus.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_UNIT_ID  1
#define DEFAULT_TCP_PORT 502
#define DEFAULT_BAUDRATE 19200

#define NUM_COILS        100
#define NUM_DISCRETE     100
#define NUM_INPUT_REGS   100
#define NUM_HOLDING_REGS 100

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Modbus type supported by example */

enum transport_type_e
{
  TRANSPORT_RTU = 0,
  TRANSPORT_ASCII,
  TRANSPORT_TCP
};

/* Modbus registers */

struct server_data_s
{
  uint8_t  coils[NUM_COILS / 8 + 1];
  uint8_t  discrete[NUM_DISCRETE / 8 + 1];
  uint16_t input_regs[NUM_INPUT_REGS];
  uint16_t holding_regs[NUM_HOLDING_REGS];
};

/* Modbus server configuration */

struct server_config_s
{
  enum transport_type_e  transport;
  enum nxmb_parity_e     parity;
  FAR const char        *device;
  FAR const char        *bindaddr;
  uint32_t               baudrate;
  uint16_t               port;
  uint8_t                unit_id;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct server_data_s g_data;
static volatile bool        g_running = true;

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
  printf("Usage: %s [OPTIONS]\n\n", progname);
  printf("Transport Options:\n");
  printf("  -t TYPE     Transport type: rtu, ascii, tcp (required)\n");
  printf("  -d DEVICE   Serial device path (for RTU/ASCII)\n");
  printf("  -b BAUD     Baud rate (default: %u)\n", DEFAULT_BAUDRATE);
  printf("  -p PARITY   Parity: none, even, odd (default: even)\n");
  printf("  -a ADDR     Bind address (for TCP, default: 0.0.0.0)\n");
  printf("  -P PORT     TCP port (default: %u)\n", DEFAULT_TCP_PORT);
  printf("\nModbus Options:\n");
  printf("  -u UNIT     Unit ID (default: %u)\n", DEFAULT_UNIT_ID);
  printf("\nExample:\n");
  printf("  %s -t rtu -d /dev/ttyS1 -b 19200\n", progname);
  printf("  %s -t tcp -P 502\n", progname);
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
 * Name: coils_callback
 ****************************************************************************/

static int coils_callback(FAR uint8_t *buf, uint16_t addr, uint16_t count,
                          enum nxmb_regmode_e mode, FAR void *priv)
{
  uint16_t byte_idx;
  uint16_t bit_idx;
  uint16_t i;

  if (addr + count > NUM_COILS)
    {
      return -ENOENT;
    }

  if (mode == NXMB_REG_READ)
    {
      for (i = 0; i < count; i++)
        {
          byte_idx = (addr + i) / 8;
          bit_idx  = (addr + i) % 8;

          if (g_data.coils[byte_idx] & (1 << bit_idx))
            {
              buf[i / 8] |= (1 << (i % 8));
            }
          else
            {
              buf[i / 8] &= ~(1 << (i % 8));
            }
        }
    }
  else
    {
      for (i = 0; i < count; i++)
        {
          byte_idx = (addr + i) / 8;
          bit_idx  = (addr + i) % 8;

          if (buf[i / 8] & (1 << (i % 8)))
            {
              g_data.coils[byte_idx] |= (1 << bit_idx);
            }
          else
            {
              g_data.coils[byte_idx] &= ~(1 << bit_idx);
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Name: discrete_callback
 ****************************************************************************/

static int discrete_callback(FAR uint8_t *buf, uint16_t addr,
                             uint16_t count, FAR void *priv)
{
  uint16_t byte_idx;
  uint16_t bit_idx;
  uint16_t i;

  if (addr + count > NUM_DISCRETE)
    {
      return -ENOENT;
    }

  for (i = 0; i < count; i++)
    {
      byte_idx = (addr + i) / 8;
      bit_idx  = (addr + i) % 8;

      if (g_data.discrete[byte_idx] & (1 << bit_idx))
        {
          buf[i / 8] |= (1 << (i % 8));
        }
      else
        {
          buf[i / 8] &= ~(1 << (i % 8));
        }
    }

  return OK;
}

/****************************************************************************
 * Name: input_callback
 ****************************************************************************/

static int input_callback(FAR uint8_t *buf, uint16_t addr, uint16_t count,
                          FAR void *priv)
{
  uint16_t i;

  if (addr + count > NUM_INPUT_REGS)
    {
      return -ENOENT;
    }

  for (i = 0; i < count; i++)
    {
      buf[i * 2]     = (uint8_t)(g_data.input_regs[addr + i] >> 8);
      buf[i * 2 + 1] = (uint8_t)(g_data.input_regs[addr + i] & 0xff);
    }

  return OK;
}

/****************************************************************************
 * Name: holding_callback
 ****************************************************************************/

static int holding_callback(FAR uint8_t *buf, uint16_t addr, uint16_t count,
                            enum nxmb_regmode_e mode, FAR void *priv)
{
  uint16_t i;

  if (addr + count > NUM_HOLDING_REGS)
    {
      return -ENOENT;
    }

  if (mode == NXMB_REG_READ)
    {
      for (i = 0; i < count; i++)
        {
          buf[i * 2]     = (uint8_t)(g_data.holding_regs[addr + i] >> 8);
          buf[i * 2 + 1] = (uint8_t)(g_data.holding_regs[addr + i] & 0xff);
        }
    }
  else
    {
      for (i = 0; i < count; i++)
        {
          g_data.holding_regs[addr + i] =
            (uint16_t)(buf[i * 2] << 8) | (uint16_t)buf[i * 2 + 1];
        }
    }

  return OK;
}

/****************************************************************************
 * Name: init_data
 ****************************************************************************/

static void init_data(void)
{
  uint16_t i;

  memset(&g_data, 0, sizeof(g_data));

  for (i = 0; i < NUM_INPUT_REGS; i++)
    {
      g_data.input_regs[i] = i * 10;
    }

  for (i = 0; i < NUM_HOLDING_REGS; i++)
    {
      g_data.holding_regs[i] = i * 100;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmbserver_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct server_config_s  config;
  struct nxmb_config_s    mb_config;
  struct nxmb_callbacks_s callbacks;
  nxmb_handle_t           handle;
  bool                    transport_set = false;
  int                     option;
  int                     ret;

  /* Default config */

  memset(&config, 0, sizeof(config));
  config.baudrate = DEFAULT_BAUDRATE;
  config.parity   = NXMB_PAR_EVEN;
  config.port     = DEFAULT_TCP_PORT;
  config.unit_id  = DEFAULT_UNIT_ID;

  /* Handle CLI */

  while ((option = getopt(argc, argv, "t:d:b:p:a:P:u:h")) != -1)
    {
      switch (option)
        {
          case 't':
            if (parse_transport(optarg, &config.transport) < 0)
              {
                fprintf(stderr, "Error: invalid transport '%s'\n", optarg);
                return EXIT_FAILURE;
              }

            transport_set = true;
            break;

          case 'd':
            config.device = optarg;
            break;

          case 'b':
            config.baudrate = (uint32_t)strtoul(optarg, NULL, 0);
            break;

          case 'p':
            if (parse_parity(optarg, &config.parity) < 0)
              {
                fprintf(stderr, "Error: invalid parity '%s'\n", optarg);
                return EXIT_FAILURE;
              }

            break;

          case 'a':
            config.bindaddr = optarg;
            break;

          case 'P':
            config.port = (uint16_t)strtoul(optarg, NULL, 0);
            break;

          case 'u':
            config.unit_id = (uint8_t)strtoul(optarg, NULL, 0);
            break;

          case 'h':
          default:
            show_usage(argv[0]);
            return (option == 'h') ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

  /* Validate input */

  if (!transport_set)
    {
      fprintf(stderr, "Error: transport type (-t) is required\n\n");
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if ((config.transport == TRANSPORT_RTU ||
       config.transport == TRANSPORT_ASCII) &&
      config.device == NULL)
    {
      fprintf(stderr, "Error: device path (-d) required for RTU/ASCII\n");
      return EXIT_FAILURE;
    }

  /* Connect signals */

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  /* Init modbus data */

  init_data();

  /* Init modbus stack */

  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.coil_cb     = coils_callback;
  callbacks.discrete_cb = discrete_callback;
  callbacks.input_cb    = input_callback;
  callbacks.holding_cb  = holding_callback;

  memset(&mb_config, 0, sizeof(mb_config));
  mb_config.unit_id   = config.unit_id;
  mb_config.is_client = false;

  switch (config.transport)
    {
      case TRANSPORT_RTU:
        mb_config.mode                      = NXMB_MODE_RTU;
        mb_config.transport.serial.devpath  = config.device;
        mb_config.transport.serial.baudrate = config.baudrate;
        mb_config.transport.serial.parity   = config.parity;
        printf("Starting Modbus RTU server on %s (baud=%" PRId32
               ", unit=%u)\n", config.device, config.baudrate,
               config.unit_id);
        break;

      case TRANSPORT_ASCII:
        mb_config.mode                      = NXMB_MODE_ASCII;
        mb_config.transport.serial.devpath  = config.device;
        mb_config.transport.serial.baudrate = config.baudrate;
        mb_config.transport.serial.parity   = config.parity;
        printf("Starting Modbus ASCII server on %s (baud=%" PRId32
               ", unit=%u)\n", config.device, config.baudrate,
               config.unit_id);
        break;

      case TRANSPORT_TCP:
        mb_config.mode                   = NXMB_MODE_TCP;
        mb_config.transport.tcp.port     = config.port;
        mb_config.transport.tcp.bindaddr = config.bindaddr;
        printf("Starting Modbus TCP server on port %u (unit=%u)\n",
               config.port, config.unit_id);
        break;
    }

  ret = nxmb_create(&handle, &mb_config);
  if (ret < 0)
    {
      fprintf(stderr, "Error: failed to create Modbus context: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = nxmb_set_callbacks(handle, &callbacks);
  if (ret < 0)
    {
      fprintf(stderr, "Error: failed to set callbacks: %d\n", ret);
      nxmb_destroy(handle);
      return EXIT_FAILURE;
    }

  ret = nxmb_enable(handle);
  if (ret < 0)
    {
      fprintf(stderr, "Error: failed to enable context: %d\n", ret);
      nxmb_destroy(handle);
      return EXIT_FAILURE;
    }

  printf("Server running. Press Ctrl+C to stop.\n");
  printf("Register map:\n");
  printf("  Coils:          1-%d (read/write)\n", NUM_COILS);
  printf("  Discrete:       1-%d (read-only)\n", NUM_DISCRETE);
  printf("  Input regs:     1-%d (read-only, value=addr*10)\n",
         NUM_INPUT_REGS);
  printf("  Holding regs:   1-%d (read/write, initial=addr*100)\n",
         NUM_HOLDING_REGS);

  /* Poll loop */

  while (g_running)
    {
      ret = nxmb_poll(handle);
      if (ret < 0 && ret != -EAGAIN)
        {
          fprintf(stderr, "Error: poll failed: %d\n", ret);
          break;
        }
    }

  printf("\nShutting down...\n");
  nxmb_disable(handle);
  nxmb_destroy(handle);

  return EXIT_SUCCESS;
}
