/****************************************************************************
 * netutils/pppd/pppd.c
 *
 *   Copyright (C) 2015 Max Nekludov. All rights reserved.
 *   Author: Max Nekludov <macscomp@gmail.com>
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

#include <nuttx/config.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <debug.h>

#include <netinet/in.h>
#include <net/if.h>
#include <nuttx/net/tun.h>

#include "ppp.h"
#include "chat.h"

#if PPP_ARCH_HAVE_MODEM_RESET
extern void ppp_arch_modem_reset(const char *tty);
#endif

/*
socat /dev/ttyUSB2,raw,echo=0,b115200,crtscts=0 /dev/ttyUSB7,raw,echo=0,b115200,crtscts=0
*/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This type describes the state of the NTP client daemon.  Only once
 * instance of the NTP daemon is permitted in this implementation.  This
 * limitation is due only to this global data structure.
 */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct chat_script_s connect_script =
{
  .timeout = 30,
  .lines =
  {
    {"AT",                                 "OK"},
    {"AT+CGDCONT = 1,\"IP\",\"internet\"", "OK"},
    {"ATD*99***1#",                        "CONNECT"},
    {0, 0}
  },
};

static struct chat_script_s disconnect_script =
{
  .timeout = 30,
  .lines =
  {
    {"ATZ",                                "OK"},
    {0, 0}
  },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: make_nonblock
 ****************************************************************************/

static int make_nonblock(int fd)
{
  int flags;

  if( (flags = fcntl(fd, F_GETFL, 0)) < 0) 
    {
      return flags;
    }

  if( (flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0 )
    {
      return flags;
    }

  return 0;
}

/****************************************************************************
 * Name: tun_alloc
 ****************************************************************************/

static int tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;

  if( (fd = open("/dev/tun", O_RDWR)) < 0 )
      return fd;

  printf("tun fd:%i\n", fd);

  if ((err = make_nonblock(fd)) < 0)
    {
      close(fd);
      return err;
    }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN;
  if( *dev )
    {
      strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

  if( (err = ioctl(fd, TUNSETIFF, (unsigned long)&ifr)) < 0 ) 
    {
      close(fd);
      return err;
    }

  strcpy(dev, ifr.ifr_name);

  return fd;
}

/****************************************************************************
 * Name: open_tty
 ****************************************************************************/

static int open_tty(char *dev)
{
  int fd;
  int err;

  if( (fd = open(dev, O_RDWR)) < 0 )
      return fd;

  if ((err = make_nonblock(fd)) < 0)
    {
      close(fd);
      return err;
    }

  printf("tty fd:%i\n", fd);

  return fd;
}

/****************************************************************************
 * Name: ppp_check_errors
 ****************************************************************************/

static u8_t ppp_check_errors(struct ppp_context_s *ctx)
{
  u8_t ret = 0;

  /* Check Errors */

  if(ctx->lcp_state & (LCP_TX_TIMEOUT | LCP_RX_TIMEOUT | LCP_TERM_PEER))
    {
      ret = 1;
    }

  if(ctx->pap_state & (PAP_TX_AUTH_FAIL | PAP_RX_AUTH_FAIL | PAP_TX_TIMEOUT | PAP_RX_TIMEOUT))
    {
      ret = 2;
    }

  if(ctx->ipcp_state & (IPCP_TX_TIMEOUT))
    {
      ret = 3;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ppp_reconnect
 ****************************************************************************/

void ppp_reconnect(struct ppp_context_s *ctx)
{
  int ret;
  int retry = PPP_MAX_CONNECT;

  netlib_ifdown((char*)ctx->ifname);

  lcp_disconnect(ctx, ++ctx->ppp_id);
  sleep(1);
  lcp_disconnect(ctx, ++ctx->ppp_id);
  sleep(1);
  write(ctx->tty_fd, "+++", 3);
  sleep(2);
  write(ctx->tty_fd, "ATE1\r\n", 6);

  if (ctx->disconnect_script)
    {
      ret = ppp_chat(ctx->tty_fd, ctx->disconnect_script, 1 /*echo on*/);
      if (ret < 0)
        {
          printf("ppp: disconnect script failed\n");
        }
    }

  if (ctx->connect_script)
    {
      do
        {
          ret = ppp_chat(ctx->tty_fd, ctx->connect_script, 1 /*echo on*/);
          if (ret < 0)
            {
              printf("ppp: connect script failed\n");
              --retry;
              if (retry == 0)
                {
                  retry = PPP_MAX_CONNECT;
#if PPP_ARCH_HAVE_MODEM_RESET
                  ppp_arch_modem_reset((char*)ctx->ttyname);
#endif
                  sleep(45);
                }
              else
                {
                  sleep(10);
                }
            }
        }
      while (ret != 0);
    }

  ppp_init(ctx);
  ppp_connect(ctx);

  ctx->ip_len = 0;
}

/****************************************************************************
 * Name: ppp_arch_clock_seconds
 ****************************************************************************/

time_t ppp_arch_clock_seconds(void)
{
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
    {
      return 0;
    }

  return ts.tv_sec;
}

/****************************************************************************
 * Name: ppp_arch_getchar
 ****************************************************************************/

int ppp_arch_getchar(struct ppp_context_s *ctx, u8_t *c)
{
  int ret;
  ret = read(ctx->tty_fd, c, 1);
  return ret == 1 ? ret : 0;
}

/****************************************************************************
 * Name: ppp_arch_putchar
 ****************************************************************************/

int ppp_arch_putchar(struct ppp_context_s *ctx, u8_t c)
{
  int ret;
  struct pollfd fds;

  ret = write(ctx->tty_fd, &c, 1);
  if (ret < 0 && errno == EAGAIN)
    {
      fds.fd = ctx->tty_fd;
      fds.events = POLLOUT;
      fds.revents = 0;

      ret = poll(&fds, 1, 1000);
      if (ret > 0)
        {
          ret = write(ctx->tty_fd, &c, 1);
        }
    }

  return ret == 1 ? ret : 0;
}

/****************************************************************************
 * Name: pppd_main
 ****************************************************************************/

int pppd_main(int argc, char **argv)
{
  struct pollfd fds[2];
  int ret;
  struct ppp_context_s *ctx;

  ctx = (struct ppp_context_s*)malloc(sizeof(struct ppp_context_s));
  memset(ctx, 0, sizeof(struct ppp_context_s));

  strcpy((char*)ctx->pap_username, PAP_USERNAME);
  strcpy((char*)ctx->pap_password, PAP_PASSWORD);
  strcpy((char*)ctx->ifname, "ppp%d");
  strcpy((char*)ctx->ttyname, "/dev/ttyS2");

  ctx->connect_script = &connect_script;
  ctx->disconnect_script = &disconnect_script;

  ctx->if_fd = tun_alloc((char*)ctx->ifname);
  if (ctx->if_fd < 0)
    {
      free(ctx);
      return 2;
    }

  ctx->tty_fd = open_tty((char*)ctx->ttyname);
  if (ctx->tty_fd < 0)
    {
      close(ctx->tty_fd);
      free(ctx);
      return 2;
    }

  fds[0].fd = ctx->if_fd;
  fds[0].events = POLLIN;

  fds[1].fd = ctx->tty_fd;
  fds[1].events = POLLIN;

  ppp_init(ctx);
  ppp_reconnect(ctx);

  while (1)
    {
      fds[0].revents = fds[1].revents = 0;

      ret = poll(fds, 2, 1000);

      if (ret > 0 && fds[0].revents & POLLIN)
        {
          ret = read(ctx->if_fd, ctx->ip_buf, PPP_RX_BUFFER_SIZE);
          printf("read from tun :%i\n", ret);
          if (ret > 0)
            {
              ctx->ip_len = ret;
              ppp_send(ctx);
              ctx->ip_len = 0;
            }
        }

      ppp_poll(ctx);

      if (ppp_check_errors(ctx))
        {
          ppp_reconnect(ctx);
        }
      else
        {
          if (ctx->ip_len > 0)
            {
              ret = write(ctx->if_fd, ctx->ip_buf, ctx->ip_len);
              //printf("write to tun :%i\n", ret);
              ctx->ip_len = 0;

              ret = read(ctx->if_fd, ctx->ip_buf, PPP_RX_BUFFER_SIZE);
              //printf("read (after write) from tun :%i\n", ret);
              if (ret > 0)
                {
                  ctx->ip_len = ret;
                  ppp_send(ctx);
                  ctx->ip_len = 0;
                }
            }
        }
    }

  return 1;
}
