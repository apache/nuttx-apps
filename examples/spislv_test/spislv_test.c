/****************************************************************************
 * apps/examples/spislv_test/spislv_test.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Define buffer sizes */
#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 64
#define SOURCE_FILE "dev/spislv2" /* SPI device path */

/* Enumeration for operation modes */

typedef enum
{
  MODE_WRITE,
  MODE_LISTEN,
  MODE_ECHO,
  MODE_INVALID
} operation_mode_t;

/****************************************************************************
 * Structure to hold program configurations
 ****************************************************************************/

typedef struct
{
  operation_mode_t mode;
  int num_bytes;            /* Applicable for write mode */
  int timeout_seconds;      /* Applicable for all modes */
  unsigned char tx_buffer[TX_BUFFER_SIZE];
} program_config_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * @brief Converts a single hexadecimal character to its byte value.
 *
 * @param c The hexadecimal character.
 * @return The byte value of the hexadecimal character, or -1 if invalid.
 */

static int hexchar_to_byte(char c)
{
  if (c >= '0' && c <= '9')
    {
      return c - '0';
    }

  c = tolower(c);

  if (c >= 'a' && c <= 'f')
    {
      return c - 'a' + 10;
    }

  return -1;
}

/**
 * @brief Converts a hexadecimal string to a byte array.
 *
 * @param hexstr The input hexadecimal string.
 * @param bytes The output byte array.
 * @param max_bytes The maximum number of bytes to convert.
 * @return The number of bytes converted, or -1 on error.
 */

static int hexstr_to_bytes(const char *hexstr, unsigned char *bytes,
                           size_t max_bytes)
{
  int len;
  int i;

  len = strlen(hexstr);
  if (len % 2 != 0 || len / 2 > max_bytes)
    {
      return -1;
    }

  for (i = 0; i < len / 2; i++)
    {
      int high = hexchar_to_byte(hexstr[2 * i]);
      int low  = hexchar_to_byte(hexstr[2 * i + 1]);

      if (high == -1 || low == -1)
        {
          return -1;
        }

      bytes[i] = (high << 4) | low;
    }

  return len / 2;
}

/**
 * @brief Parses and validates command-line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param config Pointer to the program configuration structure.
 * @return 0 on success, -1 on failure.
 */

static int parse_arguments(int argc, char *argv[],
                           program_config_t *config)
{
  int opt;

  /* Set default configurations */

  config->mode           = MODE_INVALID;
  config->num_bytes      = 0;
  config->timeout_seconds = 10; /* Default timeout */

  /* Parse command-line options */

  while ((opt = getopt(argc, argv, "x:t:le")) != -1)
    {
      switch (opt)
        {
          case 'x':
            if (config->mode != MODE_INVALID)
              {
                fprintf(stderr,
                        "Error: Multiple operation modes specified.\n");
                return -1;
              }

            config->mode = MODE_WRITE;
            config->num_bytes = atoi(optarg);
            if (config->num_bytes <= 0 ||
                config->num_bytes > TX_BUFFER_SIZE)
              {
                fprintf(stderr,
                        "Error: Invalid number of bytes for write mode.\n");
                return -1;
              }

            break;

          case 't':
            config->timeout_seconds = atoi(optarg);
            if (config->timeout_seconds <= 0)
              {
                fprintf(stderr,
                        "Error: Timeout must be a positive integer.\n");
                return -1;
              }
            break;

          case 'l':
            if (config->mode != MODE_INVALID)
              {
                fprintf(stderr,
                        "Error: Multiple operation modes specified.\n");
                return -1;
              }

            config->mode = MODE_LISTEN;
            break;

          case 'e':
            if (config->mode != MODE_INVALID)
              {
                fprintf(stderr,
                        "Error: Multiple operation modes specified.\n");
                return -1;
              }

            config->mode = MODE_ECHO;
            break;

          default:
            fprintf(stderr, "Usage:\n");
            fprintf(stderr,
        "  %s -x <num_bytes> [-t <timeout_seconds>] <hex_bytes>\n",
                    argv[0]);
            fprintf(stderr,
                    "  %s -l [-t <timeout_seconds>]\n", argv[0]);
            fprintf(stderr,
                    "  %s -e [-t <timeout_seconds>]\n", argv[0]);
            printf("Examples:\n");
            printf("  spislv -x 2 abba\n");
            printf("  spislv -l -t 5\n");
            printf("  spislv -e -t 10\n\n");
            return -1;
        }
    }

  /* Validate mutual exclusivity and required arguments */

  if (config->mode == MODE_WRITE)
    {
      if (optind >= argc)
        {
          fprintf(stderr,
                  "Error: Missing hexadecimal bytes to send.\n");
          fprintf(stderr,
        "Usage: %s -x <num_bytes> [-t <timeout_seconds>] <hex_bytes>\n",
                  argv[0]);
          return -1;
        }

      char *hex_input = argv[optind];

      /* Verify the hexadecimal string length */

      if (strlen(hex_input) != (size_t)(config->num_bytes * 2))
        {
          fprintf(stderr,
                  "Error: Hex string length must be %d characters\n"
                  "for %d bytes.\n",
                  config->num_bytes * 2, config->num_bytes);
          return -1;
        }

      /* Convert hexadecimal string to byte array */

      int converted = hexstr_to_bytes(hex_input, config->tx_buffer,
                                      TX_BUFFER_SIZE);
      if (converted != config->num_bytes)
        {
          fprintf(stderr, "Error: Invalid hexadecimal string.\n");
          return -1;
        }
    }

  else if (config->mode == MODE_INVALID)
    {
      fprintf(stderr, "Error: No operation mode specified.\n");
      fprintf(stderr, "Usage:\n");
      fprintf(stderr,
        "  %s -x <num_bytes> [-t <timeout_seconds>] <hex_bytes>\n",
              argv[0]);
      fprintf(stderr,
              "  %s -l [-t <timeout_seconds>]\n", argv[0]);
      fprintf(stderr,
              "  %s -e [-t <timeout_seconds>]\n", argv[0]);
      printf("Examples:\n");
      printf("  spislv -x 2 abba\n");
      printf("  spislv -l -t 5\n");
      printf("  spislv -e -t 10\n");
      return -1;
    }

  return 0;
}

/**
 * @brief Executes the write mode: sends specified bytes to the master.
 *
 * @param config Pointer to the program configuration structure.
 * @param fd File descriptor for the SPI device.
 * @return 0 on success, -1 on failure.
 */

static int write_mode(program_config_t *config, int fd)
{
  ssize_t bytes_written;
  char data_str[3 * TX_BUFFER_SIZE + 1]; /* Buffer for debug string */
  char *ptr = data_str;
  int len;
  int i;

  for (i = 0; i < config->num_bytes; i++)
    {
      len = snprintf(ptr, sizeof(data_str) - (ptr - data_str),
                         "%02X ", config->tx_buffer[i]);
      if (len < 0 || len >= (int)(sizeof(data_str) - (ptr - data_str)))
        {
          break;
        }

      ptr += len;
    }

  *ptr = '\0';

  printf("Slave: Queuing %d bytes for sending to master: %s\n",
         config->num_bytes, data_str);

  bytes_written = write(fd, config->tx_buffer, config->num_bytes);
  if (bytes_written < 0)
    {
      fprintf(stderr, "Error: Failed to write to %s: %s\n",
              SOURCE_FILE, strerror(errno));
      return -1;
    }

  else if (bytes_written != config->num_bytes)
    {
      fprintf(stderr, "Error: Incomplete write. Expected %d, got %zd\n",
              config->num_bytes, bytes_written);
      return -1;
    }

  return 0;
}

/**
 * @brief Executes the listen-only mode: waits for data from the master.
 *
 * @param config Pointer to the program configuration structure.
 * @param fd File descriptor for the SPI device.
 * @return 0 on success, -1 on failure.
 */

static int listen_mode(program_config_t *config, int fd)
{
  printf("Slave: Listen-only mode activated. Waiting for data\n"
         "       from master.\n");

  return 0;
}

/**
 * @brief Executes the echo mode: continuously echoes received data to
 * the master.
 *
 * @param config Pointer to the program configuration structure.
 * @param fd File descriptor for the SPI device.
 * @return 0 on success, -1 on failure.
 */

static int echo_mode_func(program_config_t *config, int fd)
{
  printf("Slave: Echo mode activated. Will echo received data until\n"
         "       timeout.\n");

  return 0;
}

/**
 * @brief Reads data from the SPI device with a specified timeout.
 *        Depending on the mode, it either exits after the first read or
 *        continues (echo mode).
 *
 * @param config Pointer to the program configuration structure.
 * @param fd File descriptor for the SPI device.
 * @return 0 on success, -1 on failure or timeout.
 */

static int read_with_timeout(program_config_t *config, int fd)
{
  unsigned char buffer_rx[RX_BUFFER_SIZE];
  ssize_t bytes_read;
  ssize_t bytes_written;
  int select_ret;

  while (1)
    {
      fd_set read_fds;
      struct timeval timeout;

      FD_ZERO(&read_fds);
      FD_SET(fd, &read_fds);

      timeout.tv_sec  = config->timeout_seconds;
      timeout.tv_usec = 0;

      select_ret = select(fd + 1, &read_fds,
                          NULL, NULL, &timeout);
      if (select_ret == -1)
        {
          fprintf(stderr, "Error: Select failed: %s\n",
                  strerror(errno));
          return -1;
        }
      else if (select_ret == 0)
        {
          if (config->mode == MODE_ECHO)
            {
              printf("Communication timeout after %d seconds. No more\n"
                     "data received from master.\n",
                     config->timeout_seconds);
            }
          else
            {
              printf("Communication timeout after %d seconds. No data\n"
                     "received from master.\n",
                     config->timeout_seconds);
            }

          return -1;
        }
      else
        {
          bytes_read = read(fd, buffer_rx, RX_BUFFER_SIZE);
          if (bytes_read < 0)
            {
              fprintf(stderr, "Error: Failed to read from %s: %s\n",
                      SOURCE_FILE, strerror(errno));
              return -1;
            }

          else if (bytes_read == 0)
            {
              printf("No data received from master.\n");
            }

          else
            {
              printf("Data received from master (%zd bytes): ",
                     bytes_read);
              for (int i = 0; i < bytes_read; i++)
                {
                  printf("%02X ", buffer_rx[i]);
                }

              printf("\n");
              if (config->mode == MODE_ECHO)
                {
                  bytes_written = write(fd, buffer_rx,
                                        bytes_read);
                  if (bytes_written < 0)
                    {
                      fprintf(stderr,
                              "Error: Failed to write to %s: %s\n",
                              SOURCE_FILE, strerror(errno));
                      return -1;
                    }
                  else if (bytes_written != bytes_read)
                    {
                      printf("Error: Incomplete write during echo. ");
                      fprintf(stderr, "Expected %zd, got %zd\n",
                        bytes_read, bytes_written);
                      return -1;
                    }

                  printf("Echoed back %zd bytes to master.\n",
                         bytes_written);
                }

              if (config->mode != MODE_ECHO)
                {
                  break;
                }
            }
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief Main function orchestrating the SPI slave operations based on
 * user input.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, -1 on failure.
 */

int main(int argc, char *argv[])
{
  program_config_t config;
  int fd;
  int ret = 0;

  if (parse_arguments(argc, argv, &config) != 0)
    {
      return -1;
    }

  fd = open(SOURCE_FILE, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "Error: Failed to open %s: %s\n",
              SOURCE_FILE, strerror(errno));
      return -1;
    }

  /* Ensure the file descriptor is in blocking mode */

  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    {
      fprintf(stderr, "Error: Failed to get file flags: %s\n",
              strerror(errno));
      close(fd);
      return -1;
    }

  flags &= ~O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1)
    {
      fprintf(stderr, "Error: Failed to set blocking mode: %s\n",
              strerror(errno));
      close(fd);
      return -1;
    }

  /* Execute the selected mode */

  switch (config.mode)
    {
      case MODE_WRITE:
        ret = write_mode(&config, fd);
        if (ret != 0)
          {
            close(fd);
            return -1;
          }
        break;

      case MODE_LISTEN:
        ret = listen_mode(&config, fd);
        if (ret != 0)
          {
            close(fd);
            return -1;
          }
        break;

      case MODE_ECHO:
        ret = echo_mode_func(&config, fd);
        if (ret != 0)
          {
            close(fd);
            return -1;
          }
        break;

      default:
        fprintf(stderr, "Error: Invalid operation mode.\n");
        close(fd);
        return -1;
    }

  ret = read_with_timeout(&config, fd);
  if (ret != 0)
    {
      close(fd);
      return -1;
    }

  printf("\n");
  close(fd);
  return 0;
}
