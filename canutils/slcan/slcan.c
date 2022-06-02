/****************************************************************************
 * apps/canutils/slcan/slcan.c
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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <nuttx/clock.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <nuttx/fs/fs.h>
#include <nuttx/arch.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <net/if.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <syslog.h>
#include <sys/uio.h>
#include <net/if.h>
#include <termios.h>
#include <nuttx/can.h>
#include <netpacket/can.h>

#include "slcan.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#define DEFAULT_PRIORITY 100
#define DEFAULT_STACK_SIZE 2048

#ifdef CONFIG_SLCAN_TRACE
#  define DEBUG 1
#else
#  define DEBUG 0
#endif

#define debug_print(fmt, ...) \
  do \
    { \
      if (DEBUG) \
        syslog(LOG_DEBUG,  fmt, ##__VA_ARGS__); \
    } \
  while (0)

/****************************************************************************
 * private data
 ****************************************************************************/

#ifdef CONFIG_SLCAN_TRACE
static char opening[] = "starting slcan\n";
#else
static char opening[] = "";
#endif

static void ok_return(int fd)
{
  write(fd, "\r", 1);
}

static void fail_return(int fd)
{
  write(fd, "\a", 1); /* BELL return for error */
}

static int readlinebuffer(int fd, char *buf, int maxlen)
{
  size_t n;
  int meslen = 0;
  int noeol  = 1;
  uint8_t ch;

  while ((meslen < maxlen) && noeol)
    {
      n = read(fd, &ch, 1);
      if (n > 0)
        {
          /* valid input */

          if (ch == '\r')
            {
              noeol = 0;
              *buf  = '\0';
            }
          else
            {
              *buf++ = ch;
              meslen++;
            }
        }
      else
        {
          return (meslen);
        }
    }
  return (meslen);
}

static int caninit(char *candev, int *s, struct sockaddr_can *addr,
                   char *ctrlmsg, struct canfd_frame *frame,
                   struct msghdr *msg, struct iovec *iov)
{
  struct ifreq ifr;

  debug_print("slcanBus\n");
  if ((*s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
      syslog(LOG_ERR, "Error opening CAN socket\n");
      return -1;
    }
  strncpy(ifr.ifr_name, candev, 4);
  ifr.ifr_name[4] = '\0';
  ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
  if (!ifr.ifr_ifindex)
    {
      syslog(LOG_ERR, "error finding index %s\n", candev);
      return -1;
    }
  memset(addr, 0, sizeof(struct sockaddr));
  addr->can_family  = AF_CAN;
  addr->can_ifindex = ifr.ifr_ifindex;
  setsockopt(*s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

  if (bind(*s, (struct sockaddr *)addr, sizeof(struct sockaddr)) < 0)
    {
      syslog(LOG_ERR, "bind error\n");
      return -1;
    }

  iov->iov_base    = frame;
  msg->msg_name    = addr;
  msg->msg_iov     = iov;
  msg->msg_iovlen  = 1;
  msg->msg_control = ctrlmsg;

  /* CAN interface ready to be used */

  debug_print("CAN socket open\n");

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: slcan_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  /* UART */

  char buf[31];
  int mode = 0;
  size_t n;
  int canspeed = 1000000; /* default to 1MBps */
  int fd;                 /* UART slcan channel */

  /* CAN */

  int s;
  int nbytes;
  int i;
  int ret;
  int reccount;
  struct sockaddr_can addr;
  struct canfd_frame frame;
  struct msghdr msg;
  struct iovec iov;
  fd_set rdfs;
  char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) +
                          3 * sizeof(struct timespec) + sizeof(int))];
  char sbuf[40];
  char *sbp;

  if (argc != 3)
    {
      fprintf(stderr, "Usage: slcan <can device> <uart device>\n");
      fflush(stderr);
      return -1;
    }

  char *chrdev = argv[2];
  char *candev = argv[1];

  debug_print("Starting slcan on NuttX\n");
  fd = open(chrdev, O_RDWR);
  if (fd < 0)
    {
      syslog(LOG_ERR, "Failed to open serial channel %s\n", chrdev);
      return -1;
    }
  else
    {
      /* Create CAN socket */

      if (caninit(candev, &s, &addr, &ctrlmsg[0], &frame, &msg, &iov) < 0)
        {
          syslog(LOG_ERR, "Failed to open CAN socket %s\n", candev);
          close(fd);
          return -1;
        }

      /* serial interface active */

      debug_print("Serial interface open %s\n", chrdev);
      write(fd, opening, (sizeof(opening) - 1));

      while (mode < 100)
        {
          /* Setup ooll */

          FD_ZERO(&rdfs);
          FD_SET(s, &rdfs);  /* CAN Socket */
          FD_SET(fd, &rdfs); /* UART */

          if ((ret = select(s + 1, &rdfs, NULL, NULL, NULL)) <= 0)
            {
              continue;
            }

          if (FD_ISSET(s, &rdfs))
            {
              /* CAN received new message in socketCAN input */

              iov.iov_len        = sizeof(frame);
              msg.msg_namelen    = sizeof(addr);
              msg.msg_controllen = sizeof(ctrlmsg);
              msg.msg_flags      = 0;
              nbytes             = recvmsg(s, &msg, 0);

              if (nbytes == CAN_MTU)
                {
                  reccount++;
                  debug_print("R%d, Id:0x%lX\n", reccount, frame.can_id);
                  if (frame.can_id & CAN_EFF_FLAG)
                    {
                      /* 29 bit address */

                      frame.can_id = frame.can_id & ~CAN_EFF_FLAG;
                      sprintf(sbuf, "T%08lX%d", frame.can_id, frame.len);
                      sbp = &sbuf[10];
                    }
                  else
                    {
                      /* 11 bit address */

                      sprintf(sbuf, "t%03lX%d", frame.can_id, frame.len);
                      sbp = &sbuf[5];
                    }

                  for (i = 0; i < frame.len; i++)
                    {
                      sprintf(sbp, "%02X", frame.data[i]);
                      sbp += 2;
                    }

                  *sbp++ = '\r';
                  *sbp   = '\0';
                  write(fd, sbuf, strlen(sbuf));
                }
            }

          if (FD_ISSET(fd, &rdfs))
            {
              /* UART receive */

              n = readlinebuffer(fd, buf, 30);

              switch (mode)
                {
                case 0: /* CAN channel not open */
                  if (n > 0)
                    {
                      if (buf[0] == 'F')
                        {
                          /* return clear flags */

                          write(fd, "F00\r", 4);
                        }
                      else if (buf[0] == 'O')
                        {
                          /* open CAN interface */

                          mode = 1;
                          debug_print("Open interface\n");
                          ok_return(fd);
                        }
                      else if (buf[0] == 'S')
                        {
                          /* set CAN interface speed */

                          switch (buf[1])
                            {
                            case '0':
                              canspeed = 10000;
                              break;
                            case '1':
                              canspeed = 20000;
                              break;
                            case '2':
                              canspeed = 50000;
                              break;
                            case '3':
                              canspeed = 100000;
                              break;
                            case '4':
                              canspeed = 125000;
                              break;
                            case '5':
                              canspeed = 250000;
                              break;
                            case '6':
                              canspeed = 500000;
                              break;
                            case '7':
                              canspeed = 800000;
                              break;
                            case '8': /* set speed to 1Mbps */
                              canspeed = 1000000;
                              break;
                            default:
                              break;
                            }

                          struct ifreq ifr;

                          /* set the device name */

                          strncpy(ifr.ifr_name, argv[1], IFNAMSIZ - 1);
                          ifr.ifr_name[IFNAMSIZ - 1] = '\0';

                          ifr.ifr_ifru.ifru_can_data.arbi_bitrate =
                            canspeed / 1000; /* Convert bit/s to kbit/s */
                          ifr.ifr_ifru.ifru_can_data.arbi_samplep = 80;

                          if (ioctl(s, SIOCSCANBITRATE, &ifr) < 0)
                            {
                              syslog(LOG_ERR, "set speed %d failed\n",
                                     canspeed);
                              fail_return(fd);
                            }
                          else
                            {
                              debug_print("set speed %d\n", canspeed);
                              ok_return(fd);
                            }
                        }
                      else
                        {
                          /* whatever */

                          ok_return(fd);
                        }
                    }
                  break;
                case 1: /* CAN task running open interface */
                  if (n > 0)
                    {
                      if (buf[0] == 'C')
                        {
                          /* close interface */

                          mode = 0;
                          debug_print("Close interface\n");
                          ok_return(fd);
                        }
                      else if (buf[0] == 'T')
                        {
                          /* Transmit an extended 29 bit CAN frame */

                          char tbuf[9];
                          int idval;
                          int val;

                          /* get 29bit CAN ID */

                          strncpy(tbuf, &buf[1], 8);
                          tbuf[8] = '\0';
                          sscanf(tbuf, "%x", &idval);

                          frame.len = buf[9] - '0'; /* get byte count */

                          /* get canmessage */

                          for (i = 0; i < frame.len; i++)
                            {
                              tbuf[0] = buf[10 + (2 * i)];
                              tbuf[1] = buf[11 + (2 * i)];
                              tbuf[2] = '\0';
                              sscanf(tbuf, "%x", &val);
                              frame.data[i] = val & 0xff;
                            }

                          debug_print("Transmitt: 0x%X ", idval);
                          for (i = 0; i < frame.len; i++)
                            {
                              debug_print("0x%02X ", frame.data[i]);
                            }

                          debug_print("\n");

                          frame.can_id = idval | CAN_EFF_FLAG; /* 29 bit */

                          if (write(s, &frame, CAN_MTU) != CAN_MTU)
                            {
                              syslog(LOG_ERR, "transmitt error\n");

                              /* TODO update error flags */
                            }

                          ok_return(fd);
                        }
                      else if (buf[0] == 't')
                        {
                          /* Transmit an 11 bit CAN frame */

                          char tbuf[9];
                          int idval;
                          int val;

                          /* get 11bit CAN ID */

                          strncpy(tbuf, &buf[1], 3);
                          tbuf[3] = '\0';
                          sscanf(tbuf, "%x", &idval);

                          frame.len = buf[4] - '0'; /* get byte count */

                          /* get canmessage */

                          for (i = 0; i < frame.len; i++)
                            {
                              tbuf[0] = buf[5 + (2 * i)];
                              tbuf[1] = buf[6 + (2 * i)];
                              tbuf[2] = '\0';
                              sscanf(tbuf, "%x", &val);
                              frame.data[i] = val & 0xff;
                            }

                          debug_print("Transmitt: 0x%X ", idval);
                          for (i = 0; i < frame.len; i++)
                            {
                              debug_print("0x%02X ", frame.data[i]);
                            }

                          debug_print("\n");

                          frame.can_id = idval; /* 11 bit address command */

                          if (write(s, &frame, CAN_MTU) != CAN_MTU)
                            {
                              syslog(LOG_ERR, "transmitt error\n");

                              /* TODO update error flags */
                            }

                          ok_return(fd);
                        }
                      else
                        {
                          /* whatever */

                          ok_return(fd);
                        }
                    }
                  break;
                default: /* should not happen */
                  mode = 100;
                  break;
                }
            }
        }

      close(fd);
      close(s);
    }

  return 0;
}
