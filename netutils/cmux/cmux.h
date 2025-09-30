/****************************************************************************
 * apps/netutils/cmux/cmux.h
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

#ifndef __APPS_NETUTILS_CMUX_H
#define __APPS_NETUTILS_CMUX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/time.h>
#include <stdbool.h>
#include <debug.h>
#include <errno.h>

#define CMUX_BIT0 (0)
#define CMUX_BIT1 (1)
#define CMUX_BIT2 (2)
#define CMUX_BIT3 (3)
#define CMUX_BIT4 (4)
#define CMUX_BIT5 (5)
#define CMUX_BIT6 (6)
#define CMUX_BIT7 (7)

#define CMUX_BUFFER_SZ (1024)
#define CMUX_CHANNEL_NAME_SZ (64)
#define CMUX_FRAME_MAX_SIZE (127)

/**
 * Mux Frame
 *
 * |      Open flag     |
 * |      1 octed       |
 * |       0xF9         |
 * ______________________
 *        Address
 * |      1 octet       |
 * |      ----------    |
 * ______________________
 *        Control
 * |     1 octet        |
 * |    ----------      |
 * ______________________
 *        Length
 * |     1-2 octet      |
 * |      ---------     |
 * ______________________
 *      Information
 * |  Multiples octets  |
 * |  ----------------  |
 * ______________________
 * |      FCS           |
 * |      1 octed       |
 * |    ---------       |
 * ______________________
 * |    Close flag      |
 * |    1 octed         |
 * |    0xF9            |
 *
 */

/* Flag Field - Each frame begins and ends with a flag sequence octet. */

#define CMUX_OPEN_FLAG       (0xF9)
#define CMUX_CLOSE_FLAG (0xF9)

/**
 * Address Field
 *
 * |Bit 1 |Bit 2 |Bit 3 |Bit 4 |Bit 5 |Bit 6 |Bit 7 |Bit 8
 * |ADDR_FIELD_BIT_EA    | C/R  |      |      |      | DLCI |      |
 */

/* EA bit extends the range of the address field.
 * When the EA bit is set to 1 in an octet, it signifies that this octet
 * is the last octet of the length field.
 * When the EA bit is set to 0, it signifies that another
 * octet of the address field follows.
 */

#define CMUX_ADDR_FIELD_BIT_EA (CMUX_BIT1)
#define CMUX_ADDR_FIELD_OPERATOR (0x3F)
#define CMUX_ADDR_FIELD_CHECK    (0xFC)

/**
 * The C/R (command/response) bit identifies the frame as
 * either a command or a response.
 * ______________________________
 * ________| Direction| CR Value
 * Command | T.E -> U.E | 1
 * Command | T.E <- U.E | 0
 * ______________________________
 * Response | T.E -> U.E | 1
 * Response | T.E <- U.E | 0
 */

#define CMUX_ADDR_FIELD_BIT_CR (CMUX_BIT2)

/* Control field
 * |Bit 1 |Bit 2| Bit 3 |Bit 4 |Bit 5 |Bit 6 |Bit 7 |Bit 8
 *    -     -      -      -      PF     -      -      -
 * P/F (Poll/Final)
 * - The Poll bit set to 1 shall be used by one station to solicit poll
 * a response or sequence of responses from the other station.
 * - The final bit set to 1 shall be used by a station to indicate
 * the response frame transmitted as the result of a
 * soliciting (poll) command.
 */

/* Poll/Final */

#define CMUX_CONTROL_FIELD_BIT_PF (0x10)

/* Set Asynchronous Balanced Mode :  establish DLC between T.E and U.E */

#define CMUX_FRAME_TYPE_SABM (0x2F)

/* Unnumbered Acknowledgement:  is a response to SABM or DISC frame */

#define CMUX_FRAME_TYPE_UA (0x63)

/* Disconnected Mode :  frame is used to report a status where the station
 * is logically disconnected from the data link. When in disconnected mode,
 * no commands are accepted until the disconnected mode is terminated by
 * the receipt of a SABM command. If a DISC command is received while
 * in disconnected mode, a DM response is sent
 */

#define CMUX_FRAME_TYPE_DM (0x0F)

/* Disconnect : is a command frame and is used to close down DLC. */

#define CMUX_FRAME_TYPE_DISC (0x43)

/* Unnumbered Information with Header check :
 * command/response sends user data at either station
 */

#define CMUX_FRAME_TYPE_UIH (0xEF)

/* Unnumbered Information */

#define CMUX_FRAME_TYPE_UI (0x03)

#define CMUX_FRAME_TYPE(type, frame) ((frame->control & ~CMUX_CONTROL_FIELD_BIT_PF) == type)

/* | U.E         | <---------   SABM (DLC 1)         -------------  | T.E
 * |             | ----------   U.A (Response)        ------------> |
 * | Multiplexer | <----------  DISC (Close DLC 1)   ------------   |  Recv
 * |             | <----------- UA(Response)         -----------    |
 */

/* Note : Some manufactures doesn't support UI frame. */

/* Length Field
 * |Bit 1 |Bit 2| Bit 3 |Bit 4 |Bit 5 |Bit 6 |Bit 7 |Bit 8
 *    E/A    L1    L2     L3      L4     L5     L6     L7
 * - L1 - L7 : The L1 to L7 bits indicate the length of the
 *   following data field for the information field less than 128 bytes
 * - EA bit = 1 in an octet, it signifies that this octet
 *   is the last octet of the length field.
 * - EA bit = 0, it signifies that a second octet of
 *   the length field follows.
 * The total length of the length field is 15 bits in that case.
 */

#define CMUX_LENGTH_FIELD_MAX_VALUE (0x7F)
#define CMUX_LENGTH_FIELD_OPERATOR  (0xFE)

/* Information Field
 * The information field is the payload of the frame and carries the
 * user data and any convergence layer information.
 * The field is octet structured and only presents in UIH frames.
 */

/* FSC field
 * In the case of the UIH frame, the contents of the information field shall
 * not be included in the FCS calculation. FCS is calculated on the contents
 * of the address, control and length fields only. This means that only the
 * delivery to the correct DLCI is protected, but not the information.
 */

#define CMUX_FCS_MAX_VALUE  (0xFF)
#define CMUX_FCS_OPERATOR   (0xCF)
struct cmux_parse_s
{
  unsigned char address;              /* Reserved to address filed */
  unsigned char control;              /* Reserved to control field */
  int data_length;                    /* Reserved to data length field */
  unsigned char data[CMUX_BUFFER_SZ]; /* Reserved to information field */
};

struct cmux_stream_buffer_s
{
  unsigned char data[CMUX_BUFFER_SZ]; /* Buffer to hold incoming packets. */
  unsigned char *readp;               /* Pointer to read buffer */
  unsigned char *writep;              /* Pointer to write buffer */
  unsigned char *endp;                /* Pointer to end of buffer */
  int flag_found;                     /* Detected open flag */
  unsigned long received_count;       /* Counter to received packets */
  unsigned long dropped_count;        /* Counter to dropped packets */
};

struct cmux_channel_s
{
  int master_fd;                         /* Master pseudo terminal */
  int slave_fd;                          /* Slave pseudo terminal */
  int dlci;                              /* Data Link Connection Identifier */
  char slave_path[CMUX_CHANNEL_NAME_SZ]; /* Path do slave (/dev/pts/X) */
  bool active;                           /* Flag to check if the channel is active */
  time_t last_activity;                  /* Timestamp to last packet sent/received */
};

#endif /* __APPS_NETUTILS_CMUX_H */