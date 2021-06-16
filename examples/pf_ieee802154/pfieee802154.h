/****************************************************************************
 * apps/examples/pf_ieee802154/pfieee802154.h
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

#ifndef __EXAMPLES_PFIEEE802154_PFIEEE802154_H
#define __EXAMPLES_PFIEEE802154_PFIEEE802154_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <debug.h>
#include <netpacket/ieee802154.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ASCIISIZE  (0x7f - 0x20)
#define SENDSIZE   (ASCIISIZE+1)

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern struct ieee802154_saddr_s g_server_addr;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void pf_cmdline(int argc, char **argv);
void pf_client(void);
void pf_server(void);

#endif /* __EXAMPLES_PFIEEE802154_IEEE802154_H */
