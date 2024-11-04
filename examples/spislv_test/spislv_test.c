/****************************************************************************
 * apps/examples/spislv_test/spislv_test.c
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
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define SOURCE_FILE "/dev/spislv2"
#define RX_BUFFER_SIZE 32
#define TX_BUFFER_SIZE 16

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * spislv_test
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  char buffer_rx[RX_BUFFER_SIZE];
  char buffer_tx[TX_BUFFER_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,0x9, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  ssize_t bytes_read;
  ssize_t bytes_written;
  ssize_t i;
  size_t current_tx_index = 0; // Índice para escrita circular

  printf("Slave started!!\n");
  fd = open(SOURCE_FILE, O_RDWR);

  if (fd < 0)
    {
      printf("Failed to open %s\n", SOURCE_FILE);
      return 0;
    }

  printf("Slave: Reading from %s\n", SOURCE_FILE);

  while (1)
  {
    /* Read the number from the source file */

    bytes_read = read(fd, buffer_rx, RX_BUFFER_SIZE - 1);

    if (bytes_read < 0)
      {
        printf("Failed to read from %s\n", SOURCE_FILE);
        close(fd);
        return 0;
      }
    else if (bytes_read > 0)
    {
      ssize_t bytes_to_write = 2U;
      ssize_t total_written = 0;

      while (bytes_to_write > 0)
      {
        size_t bytes_available = TX_BUFFER_SIZE - current_tx_index;
        size_t chunk_size = (bytes_to_write < bytes_available) ? bytes_to_write : bytes_available;

        // Escreve o chunk_size de buffer_tx a partir de current_tx_index
        bytes_written = write(fd, &buffer_tx[current_tx_index], chunk_size);
        
        printf("Queued for sending to master: ");
        for (i = 0; i < chunk_size; i++)
        {
          printf("%02x ", buffer_tx[(current_tx_index + i) % TX_BUFFER_SIZE]);
        }
        printf("\n");

        if (bytes_written < 0)
        {
            printf("Falha ao escrever para %s: %s\n", SOURCE_FILE, strerror(errno));
            close(fd);
            return 0;
        }

        total_written += bytes_written;
        bytes_to_write -= bytes_written;
        current_tx_index = (current_tx_index + bytes_written) % TX_BUFFER_SIZE;
      }
    }
  }
}
