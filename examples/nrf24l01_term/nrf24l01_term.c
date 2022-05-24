/****************************************************************************
 * apps/examples/nrf24l01_term/nrf24l01_term.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <debug.h>
#include <poll.h>
#include <fcntl.h>

#include <nuttx/wireless/nrf24l01.h>

#include "system/readline.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEV_NAME   "/dev/nrf24l01"

#ifndef STDIN_FILENO
#  define STDIN_FILENO 0
#endif

#define DEFAULT_RADIOFREQ  2450

#define DEFAULT_TXPOWER    -6    /* (0, -6, -12, or -18 dBm) */

#ifdef CONFIG_WL_NRF24L01_RXSUPPORT
#  define N_PFDS  2    /* If RX support is enabled, poll both stdin and the message reception */
#else
#  define N_PFDS  1    /* If RX support is not enabled, we cannot poll the wireless device */
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

int wireless_cfg(int fd);

int wireless_open(void);

int send_pkt(int wl_fd);

int read_pkt(int wl_fd);

void usage(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t defaultaddr[NRF24L01_MAX_ADDR_LEN] =
{
  0x01, 0xca, 0xfe, 0x12, 0x34
};

char buff[NRF24L01_MAX_PAYLOAD_LEN + 1] = "";

/* polled file descr */

static struct pollfd pfds[N_PFDS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int wireless_cfg(int fd)
{
  int error = 0;

  uint32_t rf = DEFAULT_RADIOFREQ;
  int32_t txpow = DEFAULT_TXPOWER;
  nrf24l01_datarate_t datarate = RATE_1Mbps;
  nrf24l01_retrcfg_t retrcfg =
    {
      .count = 5,
      .delay = DELAY_1000us
    };

  uint32_t addrwidth = NRF24L01_MAX_ADDR_LEN;

  uint8_t pipes_en = (1 << 0);  /* Only pipe #0 is enabled */
  nrf24l01_pipecfg_t pipe0cfg;
  nrf24l01_pipecfg_t *pipes_cfg[NRF24L01_PIPE_COUNT] =
    {
      &pipe0cfg, 0, 0, 0, 0, 0
    };

  nrf24l01_state_t primrxstate;

#ifdef CONFIG_WL_NRF24L01_RXSUPPORT
  primrxstate = ST_RX;
#else
  primrxstate = ST_POWER_DOWN;
#endif

  /* Define the pipe #0 parameters (AA enabled and dynamic payload length) */

  pipe0cfg.en_aa = true;
  pipe0cfg.payload_length = NRF24L01_DYN_LENGTH;
  memcpy (pipe0cfg.rx_addr, defaultaddr, NRF24L01_MAX_ADDR_LEN);

  /* Set radio parameters */

  ioctl(fd, WLIOC_SETRADIOFREQ, (unsigned long)((uint32_t *)&rf));
  ioctl(fd, WLIOC_SETTXPOWER, (unsigned long)((int32_t *)&txpow));
  ioctl(fd, NRF24L01IOC_SETDATARATE,
        (unsigned long)((nrf24l01_datarate_t *)&datarate));
  ioctl(fd, NRF24L01IOC_SETRETRCFG,
        (unsigned long)((nrf24l01_retrcfg_t *)&retrcfg));

  ioctl(fd, NRF24L01IOC_SETADDRWIDTH,
        (unsigned long)((uint32_t *)&addrwidth));
  ioctl(fd, NRF24L01IOC_SETTXADDR, (unsigned long)((uint8_t *)&defaultaddr));

  ioctl(fd, NRF24L01IOC_SETPIPESCFG,
        (unsigned long)((nrf24l01_pipecfg_t **)&pipes_cfg));
  ioctl(fd, NRF24L01IOC_SETPIPESENABLED,
        (unsigned long)((uint8_t *)&pipes_en));

  /* Enable receiver */

  ioctl(fd, NRF24L01IOC_SETSTATE,
        (unsigned long)((nrf24l01_state_t *)&primrxstate));

  return error;
}

int wireless_open(void)
{
  int fd;

  fd = open(DEV_NAME, O_RDWR);

  if (fd < 0)
    {
      perror("Cannot open nRF24L01 device");
    }
  else
    {
      wireless_cfg(fd);
    }

  return fd;
}

int send_pkt(int wl_fd)
{
  int ret;
  int len;

  /* Remove carriage return */

  len = strlen(buff);
  if (len > 0 && buff[len - 1] == '\n')
    {
      len--;
      buff[len] = '\0';
    }

  ret = write(wl_fd, buff, len);
  if (ret < 0)
    {
      perror("Error sending packet");
      return ret;
    }
  else
    {
      int retrcount;

      ioctl(wl_fd, NRF24L01IOC_GETLASTXMITCOUNT,
            (unsigned long)((uint32_t *)&retrcount));
      printf("Packet sent successfully !  (%d retransmitted packets)\n",
             retrcount);
    }

  return OK;
}

int read_pkt(int wl_fd)
{
  int ret;
  uint32_t pipeno;

  ret = read(wl_fd, buff, sizeof(buff));
  if (ret < 0)
    {
      perror("Error reading packet\n");
      return ret;
    }

  if (ret == 0)
    {
      /* Should not happen ... */

      printf("Packet payload empty !\n");
      return ERROR;
    }

  /* Get the recipient pipe #
   * (for demo purpose, as here the receiving pipe can only be pipe #0...)
   */

  ioctl(wl_fd, NRF24L01IOC_GETLASTPIPENO,
        (unsigned long)((uint32_t *)&pipeno));

  buff[ret] = '\0';   /* end the string */
  printf("Message received : %s   (on pipe #%" PRId32 ")\n", buff, pipeno);

  return 0;
}

void usage(void)
{
  printf("nRF24L01+ wireless terminal demo.\nUsage:\n");
  printf("- Type in any message ( <= 32 char length)"
         " to send it to communication peer.\n");
  printf("- Ctrl-C to exit.\n\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  bool quit = false;

  int wl_fd;

  wl_fd = wireless_open();
  if (wl_fd < 0)
    {
      return -1;
    }

  usage();

  pfds[0].fd = STDIN_FILENO;
  pfds[0].events = POLLIN;

#ifdef CONFIG_WL_NRF24L01_RXSUPPORT
  pfds[1].fd = wl_fd;
  pfds[1].events = POLLIN;
#endif

  while (!quit)
    {
      ret = poll(pfds, N_PFDS, -1);
      if (ret < 0)
        {
          perror("Error polling console / wireless");
          goto out;
        }

      if (pfds[0].revents & POLLIN)
        {
          char c;
          read(STDIN_FILENO, &c, 1);

          if (c < ' ')
            {
              /* Any non printable char -> exits */

              quit = true;
            }
          else
            {
              printf("Message to send > %c", c);
              fflush(stdout);

              /* Prepend the initial character */

              buff[0] = c;

#ifndef CONFIG_SYSTEM_READLINE
              /* Use fgets if readline utility method is not enabled */

              if (fgets(&buff[1], sizeof(buff) - 1, stdin) == NULL)
                {
                  printf("ERROR: fgets failed: %d\n", errno);
                  goto out;
                }
#else
              ret = readline(&buff[1], sizeof(buff) - 1, stdin, stdout);

              /* Readline normally returns the number of characters read,
               * but will return EOF on end of file or if an error occurs.
               * Either will cause the session to terminate.
               */

              if (ret == EOF)
                {
                  printf("ERROR: readline failed: %d\n", ret);
                  goto out;
                }
#endif

              /* Send content */

              send_pkt(wl_fd);
            }
        }

#ifdef CONFIG_WL_NRF24L01_RXSUPPORT
      if (!quit && (pfds[1].revents & POLLIN))
        {
          read_pkt(wl_fd);
        }
#endif
    }

out:
  close(wl_fd);

  printf ("Bye !\n");
  return 0;
}
