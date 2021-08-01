/****************************************************************************
 * apps/examples/sendmail/sendmail_main.c
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <nuttx/net/ip.h>
#include "netutils/netlib.h"
#include "netutils/smtp.h"

/****************************************************************************
 * Pre-processor Defintitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_SENDMAIL_RECIPIENT
#  error "You must provide CONFIG_EXAMPLES_SENDMAIL_RECIPIENT"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_IPADDR
#  error "You must provide CONFIG_EXAMPLES_SENDMAIL_IPADDR"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_DRIPADDR
#  error "You must provide CONFIG_EXAMPLES_SENDMAIL_DRIPADDR"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_SERVERADDR
#  error "You must provide CONFIG_EXAMPLES_SENDMAIL_SERVERADDR"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_PORT
#  error "You must provide CONFIG_EXAMPLES_SENDMAIL_PORT"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_NETMASK
#  error "You must provide CONFIG_EXAMPLES_SENDMAIL_NETMASK"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_SENDER
#  define CONFIG_EXAMPLES_SENDMAIL_SENDER "nuttx-testing@example.com"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_SUBJECT
#  define CONFIG_EXAMPLES_SENDMAIL_SUBJECT "Testing SMTP from NuttX"
#endif

#ifndef CONFIG_EXAMPLES_SENDMAIL_BODY
#  define CONFIG_EXAMPLES_SENDMAIL_BODY "Test message sent by NuttX"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_recipient[] = CONFIG_EXAMPLES_SENDMAIL_RECIPIENT;
static const char g_sender[]    = CONFIG_EXAMPLES_SENDMAIL_SENDER;
static const char g_subject[]   = CONFIG_EXAMPLES_SENDMAIL_SUBJECT;
static const char g_msg_body[]  = CONFIG_EXAMPLES_SENDMAIL_BODY "\r\n";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sendmail_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct in_addr addr;
  in_port_t port;
#if defined(CONFIG_EXAMPLES_SENDMAIL_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
  void *handle;

  printf("sendmail: To: %s\n", g_recipient);
  printf("sendmail: From: %s\n", g_sender);
  printf("sendmail: Subject: %s\n", g_subject);
  printf("sendmail: Body: %s\n", g_msg_body);

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_SENDMAIL_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_SENDMAIL_IPADDR);
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_SENDMAIL_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_SENDMAIL_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

  /* Then send the mail */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_SENDMAIL_SERVERADDR);
  port = HTONS(CONFIG_EXAMPLES_SENDMAIL_PORT);
  handle = smtp_open();
  if (handle)
    {
      smtp_configure(handle, CONFIG_LIBC_HOSTNAME, &addr.s_addr, &port);
      smtp_send(handle, g_recipient, NULL, g_sender, g_subject,
                g_msg_body, strlen(g_msg_body));
      smtp_close(handle);
    }

  return 0;
}
