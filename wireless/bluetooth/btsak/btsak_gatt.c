/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_gatt.c
 * Bluetooth Swiss Army Knife -- GATT commands
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author:  Gregory Nutt <gnutt@nuttx.org>
 *
 * Based loosely on the i8sak IEEE 802.15.4 program by Anthony Merlino and
 * Sebastien Lorquet.  Commands inspired for btshell example in the
 * Intel/Zephyr Arduino 101 package (BSD license).
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

#include <stdbool.h>

#include <nuttx/wireless/bt_core.h>

#include "btsak.h"

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_gatt_exchange_mtu
 *
 * Description:
 *   gatt [-h] exchange_mtu [-h] <addr> <addr-type> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_exchange_mtu(FAR struct btsak_s *btsak, int argc,
                                 FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_discover
 *
 * Description:
 *   gatt [-h] discover [-h] <addr> <addr-type> <uuid-type> command
 *
 ****************************************************************************/

void btsak_cmd_discover(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_gatt_discover_characteristic
 *
 * Description:
 *   gatt [-h] characteristic [-h] <addr> <addr-type> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_discover_characteristic(FAR struct btsak_s *btsak,
                                            int argc, FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_gat_discover_descriptor
 *
 * Description:
 *   gatt [-h] descriptor [-h] <addr> <addr-type> command
 *
 ****************************************************************************/

void btsak_cmd_gat_discover_descriptor(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_gatt_read
 *
 * Description:
 *   gatt [-h] read [-h] <addr> <addr-type> <handle> [<offset>] command
 *
 ****************************************************************************/

void btsak_cmd_gatt_read(FAR struct btsak_s *btsak, int argc,
                         FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_gatt_read_multiple
 *
 * Description:
 *   gatt [-h] read-multiple [-h] <addr> <addr-type> <handle> <nitems> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_read_multiple(FAR struct btsak_s *btsak, int argc,
                                  FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_gatt_write
 *
 * Description:
 *   gatt [-h] write [-h] [-h] <addr> <addr-type> <handle> <datum> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_write(FAR struct btsak_s *btsak, int argc,
                          FAR char *argv[])
{
# warning Missing logic
}
