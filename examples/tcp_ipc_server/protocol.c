/****************************************************************************
 * apps/examples/tcp_ipc_server/protocol.c
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
#include "lorawan/uart_lorawan_layer.h"
#include "protocol.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define LORAWAN_DOWNLINK_TIME_MS       10000
#define MAX_MESSAGE_SIZE               12
#define SEND_MESSAGE                   'U'
#define DOWNLINK_RESPONSE              'D'

/****************************************************************************
 * Name: send_msg_to_lpwan
 * Description: function that parses received message from a client and
 *              sends a message to UART LoRaWAN layer based on parsed
 *              data.
 * Parameters: - pointer to char array containing received message from
 *               client
 *             - pointer to a protocol structure variable, which will contain
 *               the response to be sent back to client
 * Return: nothing
 ****************************************************************************/

void send_msg_to_lpwan(unsigned char *msg, protocolo_ipc *pt_protocol)
{
  protocolo_ipc tprotocol;
  unsigned char buf_recv_downlink[12];
  int bytes_recv = 0;

  memcpy((unsigned char *)&tprotocol, msg, sizeof(protocolo_ipc));

  /* Parse message accordingly to received opcode */

  switch (tprotocol.opcode)
  {
    case SEND_MESSAGE:
      /* A client wants to send a LoRaWAN message. Here, the message is
       * forwarded to LoRaWAN transceiver
       */

      memset(buf_recv_downlink, 0x00, sizeof(buf_recv_downlink));
      bytes_recv = lorawan_radioenge_send_msg(tprotocol.msg,
                                              tprotocol.msg_size,
                                              buf_recv_downlink,
                                              sizeof(buf_recv_downlink),
                                              LORAWAN_DOWNLINK_TIME_MS);
      printf("Number of Downlink bytes received: %d\n\n", bytes_recv);

      if (bytes_recv < MAX_MESSAGE_SIZE)
        {
          pt_protocol->opcode =  DOWNLINK_RESPONSE;
          pt_protocol->msg_size = (unsigned char) bytes_recv;
          snprintf((char *)pt_protocol->msg, MAX_MESSAGE_SIZE, "%s",
                   buf_recv_downlink);
        }
      break;

    default:
      break;
  }
}
