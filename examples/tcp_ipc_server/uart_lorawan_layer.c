/****************************************************************************
 * apps/examples/tcp_ipc_server/uart_lorawan_layer.c
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
#include <inttypes.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "lorawan/uart_lorawan_layer.h"

/* Useful links:
 *
 * LoRaWAN module used here: Radioenge LoRaWAN module
 * Radioenge LoRaWAN module page:
 * https://www.radioenge.com.br/produto/modulo-lorawan/
 * Radioenge LoRaWAN module datasheet:
 * https://www.radioenge.com.br/storage/2021/08/Manual_LoRaWAN_Jun2022.pdf
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_ESP32_UART1
#  error "CONFIG_ESP32_UART1 needs to be defined in order to compile this program."
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void clear_uart_rx_buffer(void);
static int send_uart_lorawan_at_commands(unsigned char * ptr_at_cmd,
                                         int size_at_cmd);
static int read_uart_lorawan_resp(unsigned char * ptr_response_buffer,
                                         int size_response_buffer,
                                         int time_to_wait_ms);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define PATH_TO_UART1              "/dev/ttyS1"
#define FULL_AT_CMD_MAX_SIZE       200
#define TIME_BETWEEN_AT_CMDS       1 //s
#define MAX_ATTEMPTS_TO_SEND       3

/****************************************************************************
 * Public variables
 ****************************************************************************/

static int fd_uart = 0;
static bool is_lorawan_busy = false;

/****************************************************************************
 * Name: clear_uart_rx_buffer
 * Description: clean UART RX buffer
 * Parameters: nothing
 * Return: nothing
 ****************************************************************************/

static void clear_uart_rx_buffer(void)
{
  char rcv_byte_uart = 0x00;

  if (fd_uart <= -1)
    {
      /* invalid UART file descriptor */

      return;
    }
  else
    {
      while (read(fd_uart, &rcv_byte_uart, 1) > 0);
    }
}

/****************************************************************************
 * Name: send_uart_lorawan_at_commands
 * Description: send AT commands to LoRaWAN module via UART
 * Parameters: - pointer to array containing AT command to send
 *             - AT command size
 * Return: number of bytes written to UART
 ****************************************************************************/

static int send_uart_lorawan_at_commands(unsigned char * ptr_at_cmd,
                                         int size_at_cmd)
{
  int bytes_written_uart = 0;

  if (fd_uart <= -1)
    {
      /* invalid UART file descriptor */

      goto END_UART_SEND_AT_CMD;
    }

  /* Send AT command to LoRaWAN module */

  bytes_written_uart = write(fd_uart, ptr_at_cmd, size_at_cmd);

END_UART_SEND_AT_CMD:
  return bytes_written_uart;
}

/****************************************************************************
 * Name: read_uart_LoRaWAN_AT_command_response
 * Description: read AT command response via UART
 * Parameters: - pointer to array containing response buffer
 *             - response buffer size
 *             - time to wait for UART response
 * Return: number of bytes read from UART
 ****************************************************************************/

static int read_uart_lorawan_resp(unsigned char * ptr_response_buffer,
                                         int size_response_buffer,
                                         int time_to_wait_ms)
{
  int total_bytes_read_uart = 0;
  int bytes_read_uart = 0;
  bool keep_reading = true;
  useconds_t time_to_wait_us = 0;
  char full_response[50];
  char *pos_msg_downlink;
  char *pos_msg_at_busy_error;
  int status_read_uart_lorawan_resp = 0;

  memset(full_response, 0x00, sizeof(full_response));

  if (fd_uart <= -1)
    {
      /* invalid UART file descriptor */

      goto END_UART_READ_AT_CMD;
    }

  /* Wait for UART response */

  time_to_wait_us = time_to_wait_ms * 1000;
  usleep(time_to_wait_us);

  do
    {
      bytes_read_uart = read(fd_uart,
                             full_response + total_bytes_read_uart,
                             1);

      if (bytes_read_uart > 0)
        {
          ptr_response_buffer++;
          total_bytes_read_uart++;
        }
      else
        {
          /* No more bytes to read */

          keep_reading = false;
        }

      if (total_bytes_read_uart >= 50)
        {
          keep_reading = false;
        }
    }
  while (keep_reading == true);

  /* Check if response message is AT_BUSY_ERROR */

  pos_msg_at_busy_error = strstr(full_response, "AT_BUSY_ERROR");

  if (pos_msg_at_busy_error != NULL)
    {
      status_read_uart_lorawan_resp = ERR_AT_BUSY_ERROR;
      goto END_UART_READ_AT_CMD;
    }

  /* Check if response is a downlink message */

  pos_msg_downlink = strstr(full_response, "RX:");

  if (pos_msg_downlink == NULL)
    {
      status_read_uart_lorawan_resp = 0;
    }
  else
    {
      snprintf((char *)ptr_response_buffer, size_response_buffer, "%s",
               pos_msg_downlink + 3);
      status_read_uart_lorawan_resp = strlen((char *)ptr_response_buffer);
    }

END_UART_READ_AT_CMD:
  return status_read_uart_lorawan_resp;
}

/****************************************************************************
 * Name: LoRaWAN_Radioenge_init
 * Description: init LoRaWAN module
 * Parameters: LoRaWAN module config structure
 * Return: nothing
 ****************************************************************************/

void lorawan_radioenge_init(config_lorawan_radioenge_t config_lorawan)
{
  int bytes_written_at_cmd = 0;
  unsigned char full_at_cmd[FULL_AT_CMD_MAX_SIZE];

  memset(full_at_cmd, 0x00, sizeof(full_at_cmd));

  /* Open UART communication with LoRaWAN module */

  fd_uart = open(PATH_TO_UART1, O_RDWR | O_NONBLOCK);

  if (fd_uart <= -1)
    {
      perror("Cannot open UART communication with LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  /* Configuration: LoRaWAN channel mask */

  memset(full_at_cmd, 0x00, FULL_AT_CMD_MAX_SIZE);
  snprintf((char *)full_at_cmd, FULL_AT_CMD_MAX_SIZE, "AT+CHMASK=%s\n\r",
           config_lorawan.channel_mask);
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting Channel Mask to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] Channel mask configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: join mode (ABP) */

  memset(full_at_cmd, 0x00, FULL_AT_CMD_MAX_SIZE);
  snprintf((char *)full_at_cmd, FULL_AT_CMD_MAX_SIZE, "AT+NJM=0\n\r");
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting Join Mode to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] NJM configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: device address */

  memset(full_at_cmd, 0x00, FULL_AT_CMD_MAX_SIZE);
  snprintf((char *)full_at_cmd, FULL_AT_CMD_MAX_SIZE, "AT+DADDR=%s\n\r",
           config_lorawan.device_address);
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting device address to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] Device address configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: Application EUI */

  memset(full_at_cmd, 0x00, FULL_AT_CMD_MAX_SIZE);
  snprintf((char *)full_at_cmd, FULL_AT_CMD_MAX_SIZE, "AT+APPEUI=%s\n\r",
           config_lorawan.application_eui);
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting application EUI to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] APP EUI configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: Application Session Key */

  memset(full_at_cmd, 0x00, sizeof(full_at_cmd));
  snprintf((char *)full_at_cmd, sizeof(full_at_cmd), "AT+APPSKEY=%s\n\r",
           config_lorawan.application_session_key);
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting application session key"
             "to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] APPSKEY configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: Network Session Key */

  memset(full_at_cmd, 0x00, sizeof(full_at_cmd));
  snprintf((char *)full_at_cmd, sizeof(full_at_cmd), "AT+NWKSKEY=%s\n\r",
           config_lorawan.network_session_key);
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting network session key"
             "to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] NWSKEY configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: ADR */

  memset(full_at_cmd, 0x00, sizeof(full_at_cmd));
  snprintf((char *)full_at_cmd, sizeof(full_at_cmd), "AT+ADR=0\n\r");
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting ADR configuration to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] ADR configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: DR */

  memset(full_at_cmd, 0x00, sizeof(full_at_cmd));
  snprintf((char *)full_at_cmd, sizeof(full_at_cmd), "AT+DR=0\n\r");
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting DR configuration to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] DR configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: LoRaWAN class */

  memset(full_at_cmd, 0x00, sizeof(full_at_cmd));
  snprintf((char *)full_at_cmd, sizeof(full_at_cmd), "AT+CLASS=A\n\r");
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting LoRaWAN class to LoRaWAN module");
      goto END_UART_LORAWAN_MODULE_INIT;
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] LoRaWAN class A configured\r\n");
  clear_uart_rx_buffer();

  /* Configuration: send confirmation */

  memset(full_at_cmd, 0x00, sizeof(full_at_cmd));
  snprintf((char *)full_at_cmd, sizeof(full_at_cmd), "AT+CFM=1\n\r");
  bytes_written_at_cmd = send_uart_lorawan_at_commands(full_at_cmd,
  strlen((char *)full_at_cmd));

  if (bytes_written_at_cmd <= 0)
    {
      perror("Error when writting send confirmation to LoRaWAN module");
    }

  sleep(TIME_BETWEEN_AT_CMDS);
  printf("\n\r[LORAWAN] CFM configured\r\n");
  clear_uart_rx_buffer();

END_UART_LORAWAN_MODULE_INIT:
}

/****************************************************************************
 * Name: lorawan_radioenge_send_msg
 * Description: send LoRaWAN message with waiting for a downlink message
 * Parameters: - pointer to array containing uplink message (in Hex-String
 *               format)
 *             - uplink message size
 *             - pointer to array containing downlink message
 *             - downlink message array size
 *             - Time to wait for the downlink message (ms)
 * Return: nothing
 ****************************************************************************/

int lorawan_radioenge_send_msg(unsigned char * pt_uplink_hexstring,
                               int size_uplink,
                               unsigned char * pt_downlink_hexstring,
                               int max_size_downlink,
                               int time_to_wait_ms)
{
  int bytes_rcv_downlink_msg = 0;
  int number_of_attempts = 0;
  unsigned char at_cmd_send_message[FULL_AT_CMD_MAX_SIZE];

  memset(at_cmd_send_message, 0x00, sizeof(at_cmd_send_message));

  while (is_lorawan_busy == true);
  is_lorawan_busy = true;

  /* Clear RX UART buffer before sending a new command
   * (to clean all messages received from previous AT commands
   */

  clear_uart_rx_buffer();

  /* Compose AT command */

  snprintf((char *)at_cmd_send_message,
           FULL_AT_CMD_MAX_SIZE,
           "AT+SENDB=5:%s\n\r",
           pt_uplink_hexstring);
  printf("\n\r[LORAWAN] AT CMD: %s\n\r", at_cmd_send_message);

  /* Send uplink message (until success or number of attempts exceeds
   *  what is defined in MAX_ATTEMPTS_TO_SEND)
   */

  while (number_of_attempts < MAX_ATTEMPTS_TO_SEND)
    {
      if (send_uart_lorawan_at_commands(at_cmd_send_message,
          strlen((char *)at_cmd_send_message)) <= 0)
        {
          printf("\n\rError when sending uplink message.\n\r");
          number_of_attempts++;
          continue;
        }

      /* Get downlink message */

      bytes_rcv_downlink_msg = read_uart_lorawan_resp(pt_downlink_hexstring,
                                                          max_size_downlink,
                                                          time_to_wait_ms);

      /* If a AT_BUSY is received as command responde, command must
       * be sent again
       */

      if (bytes_rcv_downlink_msg == ERR_AT_BUSY_ERROR)
        {
          number_of_attempts++;
          continue;
        }

      /* If there isn't AT_BUSY_ERROR response, check for downlink
       * message
       */

      if (bytes_rcv_downlink_msg == 0)
        {
          printf("\n\rNo downlink message has been received.\n\r");
          break;
        }
    }

  if (number_of_attempts == MAX_ATTEMPTS_TO_SEND)
    {
      printf("\n\rError: failed to send command in all %d attempts\n\r",
             MAX_ATTEMPTS_TO_SEND);
    }

  is_lorawan_busy = false;
  return bytes_rcv_downlink_msg;
}
