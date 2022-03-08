/****************************************************************************
 * apps/examples/netpkt/netpkt_main.c
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
#include <sys/socket.h>
#include <unistd.h>
#include <netpacket/packet.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: psock_create
 ****************************************************************************/

static int psock_create(void)
{
  int sd;
  struct sockaddr_ll addr;
  int addrlen = sizeof(addr);

  sd = socket(AF_PACKET, SOCK_RAW, 0);
  if (sd < 0)
    {
      perror("ERROR: failed to create packet socket");
      return -1;
    }

  /* Prepare sockaddr struct */

  addr.sll_family = AF_PACKET;
  addr.sll_ifindex = 0;
  if (bind(sd, (const struct sockaddr *)&addr, addrlen) < 0)
    {
      perror("ERROR: binding socket failed");
      close(sd);
      return -1;
    }

  return sd;
}

/****************************************************************************
 * Name: print_buf
 ****************************************************************************/

static void print_buf(const uint8_t *buf, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      printf("%02X", buf[i]);

      if ((i + 1) % 16 == 0 || (i + 1) == len)
        {
          printf("\n");
        }
      else if ((i + 1) % 8 == 0)
        {
          printf("  ");
        }
      else
        {
          printf(" ");
        }
    }
}

/****************************************************************************
 * Name: netpkt_usage
 ****************************************************************************/

static void netpkt_usage(void)
{
  printf("usage: netpkt options\n");
  printf("\n");
  printf(" -a     transmit and receive\n");
  printf(" -r     receive\n");
  printf(" -t     transmit\n");
  printf(" -v     verbose\n");
  printf("\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netpkt_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int sd;
  int i;
  int txc;
  int rxc;
  uint8_t *buf;
  const int buflen = 128;
  const char da[6] =
    {
      0xf0, 0xde, 0xf1, 0x02, 0x43, 0x01
    };

  const char sa[6] =
    {
      0x00, 0xe0, 0xde, 0xad, 0xbe, 0xef
    };

  int opt;
  int verbose = 0;
  int do_rx = 0;
  int do_rxtimes = 3;
  int do_tx = 0;
  int do_txtimes = 3;

  if (argc == 1)
    {
      netpkt_usage();
      return -1;
    }

  /* Parse arguments */

  while ((opt = getopt(argc, argv, "artv")) != -1)
    {
      switch (opt)
        {
          case 'a':
            do_rx = 1;
            do_tx = 1;
            break;

          case 'r':
            do_rx = 1;
            break;

          case 't':
            do_tx = 1;
            break;

          case 'v':
            verbose = 1;
            break;

          default:
            netpkt_usage();
            return -1;
      }
  }

  sd = psock_create();

  if (do_tx)
    {
      if (verbose)
        {
          printf("Testing write() (%d times)\n", do_txtimes);
        }

      buf = malloc(buflen);
      memset(buf, 0, buflen);
      memcpy(buf, da, 6);
      memcpy(buf + 6, sa, 6);
      for (i = 0; i < do_txtimes; i++)
        {
          if ((txc = write(sd, buf, buflen)) < 0)
            {
              perror("ERROR: write failed");
              free(buf);
              close(sd);
              return -1;
            }
          else
            {
              if (verbose)
                {
                  printf("transmitted %d octets\n", txc);
                  print_buf(buf, txc);
                }
            }
        }

      free(buf);
    }

  if (do_rx)
    {
      if (verbose)
        {
          printf("Testing read() (%d times)\n", do_rxtimes);
        }

      buf = malloc(buflen);
      memset(buf, 0, buflen);
      for (i = 0; i < do_rxtimes;  i++)
        {
          rxc = read(sd, buf, buflen);
          if (rxc < 0)
            {
              perror("ERROR: read failed");
              free(buf);
              close(sd);
              return -1;
            }
          else if (rxc == 0)
            {
              /* ignore silently */
            }
          else
            {
              if (verbose)
                {
                  printf("received %d octets\n", rxc);
                  print_buf(buf, rxc);
                }
            }
        }

      free(buf);
    }

  close(sd);
  return 0;
}
