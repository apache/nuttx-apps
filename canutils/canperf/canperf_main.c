/****************************************************************************
 * apps/canutils/canperf/canperf_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef CONFIG_CAN
#  include <nuttx/can/can.h>
#endif

#ifdef CONFIG_NET_CAN
#  include <net/if.h>
#  include <sys/socket.h>
#  include <nuttx/can.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CANPERF_BATCH     16    /* Chardev messages per write/read */
#define CANPERF_SEQLEN    4     /* Sequence number bytes in the payload */

#ifdef CONFIG_CAN_FD
#  define CANPERF_DEV_MAXLEN  64
#else
#  define CANPERF_DEV_MAXLEN  8
#endif

#ifdef CONFIG_NET_CAN_CANFD
#  define CANPERF_SOCK_MAXLEN 64
#else
#  define CANPERF_SOCK_MAXLEN 8
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum canperf_mode_e
{
  CANPERF_TX = 0,
  CANPERF_RX,
  CANPERF_ECHO
};

struct canperf_s
{
  enum canperf_mode_e mode;
  FAR const char *devpath;      /* Chardev path, or NULL */
  FAR const char *ifname;       /* SocketCAN interface, or NULL */
  uint32_t count;               /* Frames to send or receive */
  uint32_t id;                  /* CAN ID of sent frames */
  uint32_t rate;                /* TX frames/s, 0 = as fast as possible */
  uint32_t timeout;             /* RX idle timeout in ms */
  uint32_t rxdelay;             /* Delay after each RX read in us */
  bool     rxspin;              /* Busy-wait the RX delay */
  uint32_t bitrate;             /* Bit rate to set first, 0 = keep */
  uint8_t  len;                 /* Payload bytes */
  bool     extid;
  bool     fd;
  bool     brs;

  /* Results */

  uint32_t frames;
  uint64_t bytes;
  uint32_t lost;
  uint32_t ooo;                 /* Sequence went backwards */
  uint32_t errframes;           /* Error frames received */
  uint32_t txretry;             /* TX calls that returned EAGAIN/ENOBUFS */
  uint64_t t0;                  /* First frame, us */
  uint64_t t1;                  /* Last frame, us */
  bool     haveseq;
  uint32_t nextseq;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: canperf_now
 *
 * Description:
 *   Get the monotonic time.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Time in microseconds.
 *
 ****************************************************************************/

static uint64_t canperf_now(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/****************************************************************************
 * Name: canperf_putseq
 *
 * Description:
 *   Fill a payload: a little-endian sequence number in the first
 *   CANPERF_SEQLEN bytes, then a pattern.
 *
 * Input Parameters:
 *   data - Payload buffer
 *   len  - Payload length
 *   seq  - Sequence number
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_putseq(FAR uint8_t *data, uint8_t len, uint32_t seq)
{
  uint8_t i;

  for (i = 0; i < len; i++)
    {
      data[i] = i < CANPERF_SEQLEN ? (uint8_t)(seq >> (8 * i)) :
                                     (uint8_t)(seq + i);
    }
}

/****************************************************************************
 * Name: canperf_account
 *
 * Description:
 *   Count one received frame and check its sequence number.
 *
 * Input Parameters:
 *   p    - Test state
 *   data - Payload
 *   len  - Payload length
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_account(FAR struct canperf_s *p,
                            FAR const uint8_t *data, uint8_t len)
{
  uint64_t now = canperf_now();
  uint32_t seq;

  if (p->frames == 0)
    {
      p->t0 = now;
    }

  p->t1 = now;
  p->frames++;
  p->bytes += len;

  if (len < CANPERF_SEQLEN)
    {
      return;
    }

  seq = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);

  if (p->haveseq)
    {
      if (seq > p->nextseq)
        {
          p->lost += seq - p->nextseq;
        }
      else if (seq < p->nextseq)
        {
          p->ooo++;
        }
    }

  p->haveseq = true;
  p->nextseq = seq + 1;
}

/****************************************************************************
 * Name: canperf_pace
 *
 * Description:
 *   Wait until the TX slot of frame n for a fixed frame rate.
 *
 * Input Parameters:
 *   p - Test state
 *   n - Frame number
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_pace(FAR struct canperf_s *p, uint32_t n)
{
  uint64_t due;
  uint64_t now;

  if (p->rate == 0)
    {
      return;
    }

  due = p->t0 + (uint64_t)n * 1000000 / p->rate;
  now = canperf_now();
  if (due > now)
    {
      usleep(due - now);
    }
}

/****************************************************************************
 * Name: canperf_wait
 *
 * Description:
 *   Wait for input.
 *
 * Input Parameters:
 *   p  - Test state
 *   fd - File descriptor
 *
 * Returned Value:
 *   True if input is ready, false on the idle timeout.
 *
 ****************************************************************************/

static bool canperf_wait(FAR struct canperf_s *p, int fd)
{
  struct pollfd pfd;

  pfd.fd      = fd;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  return poll(&pfd, 1, p->timeout) > 0;
}

/****************************************************************************
 * Name: canperf_rxdelay
 *
 * Description:
 *   Slow down the reader: sleep, or spin to load the CPU.
 *
 * Input Parameters:
 *   p - Test state
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_rxdelay(FAR struct canperf_s *p)
{
  uint64_t end;

  if (p->rxdelay == 0)
    {
      return;
    }

  if (!p->rxspin)
    {
      usleep(p->rxdelay);
      return;
    }

  end = canperf_now() + p->rxdelay;
  while (canperf_now() < end)
    {
    }
}

/****************************************************************************
 * Name: canperf_report
 *
 * Description:
 *   Print the results.
 *
 * Input Parameters:
 *   p - Test state
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_report(FAR struct canperf_s *p)
{
  uint64_t us = p->t1 > p->t0 ? p->t1 - p->t0 : 0;
  uint64_t fps = us ? (uint64_t)p->frames * 1000000 / us : 0;
  uint64_t bps = us ? p->bytes * 8 * 1000000 / us : 0;

  printf("canperf: mode=%s frames=%" PRIu32 " bytes=%" PRIu64
         " us=%" PRIu64 " fps=%" PRIu64 " bps=%" PRIu64
         " lost=%" PRIu32 " ooo=%" PRIu32 " errframes=%" PRIu32
         " txretry=%" PRIu32 "\n",
         p->mode == CANPERF_TX ? "tx" :
         p->mode == CANPERF_RX ? "rx" : "echo",
         p->frames, p->bytes, us, fps, bps, p->lost, p->ooo,
         p->errframes, p->txretry);
}

#ifdef CONFIG_CAN
/****************************************************************************
 * Name: canperf_dev_fill
 *
 * Description:
 *   Build a character driver message.
 *
 * Input Parameters:
 *   p   - Test state
 *   msg - Message to fill
 *   seq - Sequence number
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_dev_fill(FAR struct canperf_s *p,
                             FAR struct can_msg_s *msg, uint32_t seq)
{
  memset(&msg->cm_hdr, 0, sizeof(msg->cm_hdr));
  msg->cm_hdr.ch_id    = p->id;
  msg->cm_hdr.ch_extid = p->extid;
  msg->cm_hdr.ch_dlc   = can_bytes2dlc(p->len);
#ifdef CONFIG_CAN_FD
  msg->cm_hdr.ch_edl   = p->fd;
  msg->cm_hdr.ch_brs   = p->brs;
#endif
  canperf_putseq(msg->cm_data, p->len, seq);
}

/****************************************************************************
 * Name: canperf_dev_tx
 *
 * Description:
 *   Send frames through a character driver.
 *
 * Input Parameters:
 *   p  - Test state
 *   fd - Open CAN device
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int canperf_dev_tx(FAR struct canperf_s *p, int fd)
{
  static uint8_t buf[CANPERF_BATCH * sizeof(struct can_msg_s)];
  size_t msglen = CAN_MSGLEN(p->len);
  uint32_t seq = 0;

  p->t0 = canperf_now();
  while (seq < p->count)
    {
      uint32_t n = p->count - seq;
      size_t off = 0;
      ssize_t ret;
      uint32_t i;

      if (p->rate != 0)
        {
          n = 1;
          canperf_pace(p, seq);
        }
      else if (n > CANPERF_BATCH)
        {
          n = CANPERF_BATCH;
        }

      for (i = 0; i < n; i++)
        {
          canperf_dev_fill(p, (FAR struct can_msg_s *)&buf[i * msglen],
                           seq + i);
        }

      /* The driver accepts whole messages; resend the rest */

      while (off < n * msglen)
        {
          ret = write(fd, &buf[off], n * msglen - off);
          if (ret < 0)
            {
              if (errno == EAGAIN || errno == EINTR)
                {
                  p->txretry++;
                  continue;
                }

              fprintf(stderr, "canperf: write failed: %d\n", errno);
              return -errno;
            }

          off += ret;
        }

      seq += n;
      p->frames += n;
      p->bytes += (uint64_t)n * p->len;
    }

  p->t1 = canperf_now();
  return 0;
}

/****************************************************************************
 * Name: canperf_dev_rx
 *
 * Description:
 *   Receive, or echo, frames through a character driver.
 *
 * Input Parameters:
 *   p  - Test state
 *   fd - Open CAN device
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int canperf_dev_rx(FAR struct canperf_s *p, int fd)
{
  static uint8_t buf[CANPERF_BATCH * sizeof(struct can_msg_s)];

  while (p->frames < p->count)
    {
      ssize_t ret;
      size_t off = 0;

      ret = read(fd, buf, sizeof(buf));
      if (ret < 0)
        {
          if (errno != EAGAIN && errno != EINTR)
            {
              fprintf(stderr, "canperf: read failed: %d\n", errno);
              return -errno;
            }

          if (!canperf_wait(p, fd))
            {
              break;
            }

          continue;
        }

      canperf_rxdelay(p);

      while (off + CAN_MSGLEN(0) <= (size_t)ret)
        {
          FAR struct can_msg_s *msg = (FAR struct can_msg_s *)&buf[off];
          uint8_t len = can_dlc2bytes(msg->cm_hdr.ch_dlc);

          off += CAN_MSGLEN(len);
#ifdef CONFIG_CAN_ERRORS
          if (msg->cm_hdr.ch_error)
            {
              p->errframes++;
              continue;
            }
#endif

          canperf_account(p, msg->cm_data, len);
          if (p->mode == CANPERF_ECHO)
            {
              while (write(fd, msg, CAN_MSGLEN(len)) < 0)
                {
                  if (errno != EAGAIN && errno != EINTR)
                    {
                      return -errno;
                    }

                  p->txretry++;
                }
            }
        }
    }

  return 0;
}

/****************************************************************************
 * Name: canperf_dev
 *
 * Description:
 *   Run the test on a CAN character device.
 *
 * Input Parameters:
 *   p - Test state
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int canperf_dev(FAR struct canperf_s *p)
{
  int oflags = p->mode == CANPERF_TX ? O_RDWR : O_RDWR | O_NONBLOCK;
  int fd;
  int ret;

  if (p->len > CANPERF_DEV_MAXLEN)
    {
      fprintf(stderr, "canperf: max length %d\n", CANPERF_DEV_MAXLEN);
      return -EINVAL;
    }

  fd = open(p->devpath, oflags);
  if (fd < 0)
    {
      fprintf(stderr, "canperf: open %s failed: %d\n", p->devpath, errno);
      return -errno;
    }

  if (p->bitrate != 0)
    {
      struct canioc_bittiming_s bt;

      memset(&bt, 0, sizeof(bt));
      if (ioctl(fd, CANIOC_GET_BITTIMING, (unsigned long)&bt) < 0)
        {
          fprintf(stderr, "canperf: get bit timing failed: %d\n", errno);
          close(fd);
          return -errno;
        }

      bt.bt_baud = p->bitrate;
      if (ioctl(fd, CANIOC_SET_BITTIMING, (unsigned long)&bt) < 0 ||
          ioctl(fd, CANIOC_GET_BITTIMING, (unsigned long)&bt) < 0)
        {
          fprintf(stderr, "canperf: set bit timing failed: %d\n", errno);
          close(fd);
          return -errno;
        }

      printf("canperf: bitrate=%" PRIu32 "\n", bt.bt_baud);
    }

  ret = p->mode == CANPERF_TX ? canperf_dev_tx(p, fd) :
                                canperf_dev_rx(p, fd);
  close(fd);
  return ret;
}
#endif /* CONFIG_CAN */

#ifdef CONFIG_NET_CAN
/****************************************************************************
 * Name: canperf_sock_fill
 *
 * Description:
 *   Build a SocketCAN frame.
 *
 * Input Parameters:
 *   p     - Test state
 *   frame - Frame to fill
 *   seq   - Sequence number
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_sock_fill(FAR struct canperf_s *p,
                              FAR struct canfd_frame *frame, uint32_t seq)
{
  memset(frame, 0, sizeof(*frame));
  frame->can_id = p->extid ? (p->id & CAN_EFF_MASK) | CAN_EFF_FLAG :
                             p->id & CAN_SFF_MASK;
  frame->len    = p->len;
  frame->flags  = p->brs ? CANFD_BRS : 0;
  canperf_putseq(frame->data, p->len, seq);
}

/****************************************************************************
 * Name: canperf_sock_send
 *
 * Description:
 *   Send one frame, retry while the interface is busy.
 *
 * Input Parameters:
 *   p     - Test state
 *   sock  - CAN_RAW socket
 *   frame - Frame to send
 *   size  - Frame size
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int canperf_sock_send(FAR struct canperf_s *p, int sock,
                             FAR const struct canfd_frame *frame,
                             size_t size)
{
  while (write(sock, frame, size) < 0)
    {
      if (errno != EAGAIN && errno != ENOBUFS && errno != EINTR)
        {
          fprintf(stderr, "canperf: write failed: %d\n", errno);
          return -errno;
        }

      p->txretry++;
      usleep(100);
    }

  return 0;
}

/****************************************************************************
 * Name: canperf_sock_tx
 *
 * Description:
 *   Send frames through a SocketCAN interface.
 *
 * Input Parameters:
 *   p    - Test state
 *   sock - CAN_RAW socket
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int canperf_sock_tx(FAR struct canperf_s *p, int sock)
{
  struct canfd_frame frame;
  size_t size = p->fd ? CANFD_MTU : CAN_MTU;
  uint32_t seq;
  int ret;

  p->t0 = canperf_now();
  for (seq = 0; seq < p->count; seq++)
    {
      canperf_pace(p, seq);
      canperf_sock_fill(p, &frame, seq);
      ret = canperf_sock_send(p, sock, &frame, size);
      if (ret < 0)
        {
          return ret;
        }

      p->frames++;
      p->bytes += p->len;
    }

  p->t1 = canperf_now();
  return 0;
}

/****************************************************************************
 * Name: canperf_sock_rx
 *
 * Description:
 *   Receive, or echo, frames through a SocketCAN interface.
 *
 * Input Parameters:
 *   p    - Test state
 *   sock - CAN_RAW socket
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int canperf_sock_rx(FAR struct canperf_s *p, int sock)
{
  struct canfd_frame frame;
  ssize_t ret;

  while (p->frames < p->count)
    {
      ret = read(sock, &frame, sizeof(frame));
      if (ret < 0)
        {
          if (errno != EAGAIN && errno != EINTR)
            {
              fprintf(stderr, "canperf: read failed: %d\n", errno);
              return -errno;
            }

          if (!canperf_wait(p, sock))
            {
              break;
            }

          continue;
        }

      canperf_rxdelay(p);

      if ((frame.can_id & CAN_ERR_FLAG) != 0)
        {
          p->errframes++;
          continue;
        }

      canperf_account(p, frame.data, frame.len);
      if (p->mode == CANPERF_ECHO)
        {
          int err = canperf_sock_send(p, sock, &frame, ret);

          if (err < 0)
            {
              return err;
            }
        }
    }

  return 0;
}

/****************************************************************************
 * Name: canperf_sock
 *
 * Description:
 *   Run the test on a SocketCAN interface.
 *
 * Input Parameters:
 *   p - Test state
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int canperf_sock(FAR struct canperf_s *p)
{
  struct sockaddr_can addr;
  struct ifreq ifr;
  int sock;
  int ret;

  if (p->len > (p->fd ? CANPERF_SOCK_MAXLEN : 8))
    {
      fprintf(stderr, "canperf: payload too long\n");
      return -EINVAL;
    }

  sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (sock < 0)
    {
      fprintf(stderr, "canperf: socket failed: %d\n", errno);
      return -errno;
    }

  strlcpy(ifr.ifr_name, p->ifname, IFNAMSIZ);
  if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
    {
      fprintf(stderr, "canperf: no interface %s\n", p->ifname);
      ret = -errno;
      goto out;
    }

#ifdef CONFIG_NETDEV_CAN_IOCTL
  if (p->bitrate != 0)
    {
      struct ifreq req;

      /* The bit rate can only be set while the interface is down */

      memset(&req, 0, sizeof(req));
      strlcpy(req.ifr_name, p->ifname, IFNAMSIZ);
      ioctl(sock, SIOCGIFFLAGS, &req);
      req.ifr_flags &= ~IFF_UP;
      ioctl(sock, SIOCSIFFLAGS, &req);

      req.ifr_ifru.ifru_can_data.arbi_bitrate = p->bitrate;
      req.ifr_ifru.ifru_can_data.data_bitrate = p->bitrate;
      ret = ioctl(sock, SIOCSCANBITRATE, &req);

      memset(&req.ifr_ifru, 0, sizeof(req.ifr_ifru));
      ioctl(sock, SIOCGIFFLAGS, &req);
      req.ifr_flags |= IFF_UP;
      ioctl(sock, SIOCSIFFLAGS, &req);
      if (ret < 0)
        {
          fprintf(stderr, "canperf: set bitrate failed: %d\n", errno);
          ret = -errno;
          goto out;
        }

      memset(&req.ifr_ifru, 0, sizeof(req.ifr_ifru));
      ioctl(sock, SIOCGCANBITRATE, &req);
      printf("canperf: bitrate=%" PRIu32 "\n",
             req.ifr_ifru.ifru_can_data.arbi_bitrate);
    }
#endif

#ifdef CONFIG_NET_CAN_CANFD
  if (p->fd)
    {
      int enable = 1;

      setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable,
                 sizeof(enable));
    }
#endif

#if defined(CONFIG_NET_CAN_ERRORS) && defined(CONFIG_NET_CANPROTO_OPTIONS)
  /* Error frames are filtered out unless the socket asks for them */

  {
    can_err_mask_t errmask = CAN_ERR_MASK;

    setsockopt(sock, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &errmask,
               sizeof(errmask));
  }
#endif

  memset(&addr, 0, sizeof(addr));
  addr.can_family  = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(sock, (FAR struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      fprintf(stderr, "canperf: bind failed: %d\n", errno);
      ret = -errno;
      goto out;
    }

  if (p->mode != CANPERF_TX)
    {
      int flags = fcntl(sock, F_GETFL, 0);

      fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

  ret = p->mode == CANPERF_TX ? canperf_sock_tx(p, sock) :
                                canperf_sock_rx(p, sock);

out:
  close(sock);
  return ret;
}
#endif /* CONFIG_NET_CAN */

/****************************************************************************
 * Name: canperf_usage
 *
 * Description:
 *   Print the usage.
 *
 * Input Parameters:
 *   progname - Program name
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void canperf_usage(FAR const char *progname)
{
  fprintf(stderr,
          "Usage: %s -m tx|rx|echo (-d <dev> | -i <ifname>) [options]\n"
          "  -n <count>   frames to send/receive (default 1000)\n"
          "  -l <len>     payload bytes, >= 4 carries a sequence number\n"
          "  -I <id>      CAN ID of sent frames (default 0x100)\n"
          "  -x           extended ID\n"
          "  -f           CAN FD frames, -b with bit rate switch\n"
          "  -r <fps>     TX frame rate, 0 = as fast as possible\n"
          "  -t <ms>      RX idle timeout (default 2000)\n"
          "  -w <us>      sleep after each RX read (slow reader)\n"
          "  -W <us>      busy-wait after each RX read (CPU load)\n"
          "  -B <bit/s>   set the bit rate first\n",
          progname);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: canperf_main
 *
 * Description:
 *   Entry point of the canperf tool.
 *
 * Input Parameters:
 *   argc - Number of arguments
 *   argv - Arguments
 *
 * Returned Value:
 *   EXIT_SUCCESS or EXIT_FAILURE.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct canperf_s p;
  int option;
  int ret = -EINVAL;

  memset(&p, 0, sizeof(p));
  p.mode    = CANPERF_TX;
  p.count   = 1000;
  p.len     = 8;
  p.id      = 0x100;
  p.timeout = 2000;

  while ((option = getopt(argc, argv, "m:d:i:n:l:I:xfbr:t:w:W:B:h")) !=
         ERROR)
    {
      switch (option)
        {
          case 'm':
            if (strcmp(optarg, "tx") == 0)
              {
                p.mode = CANPERF_TX;
              }
            else if (strcmp(optarg, "rx") == 0)
              {
                p.mode = CANPERF_RX;
              }
            else if (strcmp(optarg, "echo") == 0)
              {
                p.mode = CANPERF_ECHO;
              }
            else
              {
                canperf_usage(argv[0]);
                return EXIT_FAILURE;
              }
            break;

          case 'd':
            p.devpath = optarg;
            break;

          case 'i':
            p.ifname = optarg;
            break;

          case 'n':
            p.count = strtoul(optarg, NULL, 0);
            break;

          case 'l':
            p.len = (uint8_t)strtoul(optarg, NULL, 0);
            break;

          case 'I':
            p.id = strtoul(optarg, NULL, 0);
            break;

          case 'x':
            p.extid = true;
            break;

          case 'f':
            p.fd = true;
            break;

          case 'b':
            p.brs = true;
            break;

          case 'r':
            p.rate = strtoul(optarg, NULL, 0);
            break;

          case 't':
            p.timeout = strtoul(optarg, NULL, 0);
            break;

          case 'w':
          case 'B':
            p.bitrate = strtoul(optarg, NULL, 0);
            break;

          case 'W':
            p.rxdelay = strtoul(optarg, NULL, 0);
            p.rxspin  = option == 'W';
            break;

          default:
            canperf_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

  if ((p.devpath == NULL) == (p.ifname == NULL))
    {
      canperf_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (!p.fd && p.len > 8)
    {
      fprintf(stderr, "canperf: classic frames carry at most 8 bytes\n");
      return EXIT_FAILURE;
    }

  if (p.devpath != NULL)
    {
#ifdef CONFIG_CAN
      ret = canperf_dev(&p);
#else
      fprintf(stderr, "canperf: no CAN character driver support\n");
#endif
    }
  else
    {
#ifdef CONFIG_NET_CAN
      ret = canperf_sock(&p);
#else
      fprintf(stderr, "canperf: no SocketCAN support\n");
#endif
    }

  canperf_report(&p);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
