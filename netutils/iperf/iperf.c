/****************************************************************************
 * apps/netutils/iperf/iperf.c
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

#include <arpa/inet.h>
#include <assert.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/rpmsg.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include "iperf.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IPERF_TRAFFIC_TASK_NAME      "iperf_traffic"
#define IPERF_TRAFFIC_TASK_PRIORITY  100
#define IPERF_TRAFFIC_TASK_STACK     4096
#define IPERF_REPORT_TASK_NAME       "iperf_report"
#define IPERF_REPORT_TASK_PRIORITY   100
#define IPERF_REPORT_TASK_STACK      4096

#define IPERF_UDP_TX_LEN             (1472)
#define IPERF_UDP_RX_LEN             (16 << 10)
#define IPERF_TCP_TX_LEN             (16 << 10)
#define IPERF_TCP_RX_LEN             (16 << 10)

#define IPERF_MAX_DELAY              64
#define IPERF_SOCKET_RX_TIMEOUT      10

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct iperf_ctrl_t
{
  FAR struct iperf_ctrl_t *flink;
  struct iperf_cfg_t cfg;
  bool finish;
  uintmax_t total_len;
  uint32_t buffer_len;
  FAR uint8_t *buffer;
  uint32_t sockfd;
};

struct iperf_udp_pkt_t
{
  int32_t id;
  uint32_t sec;
  uint32_t usec;
};

typedef CODE int (*iperf_client_func_t)(FAR struct iperf_ctrl_t *ctrl,
                                        FAR struct sockaddr *addr,
                                        socklen_t addrlen);
typedef CODE int (*iperf_server_func_t)(FAR struct iperf_ctrl_t *ctrl,
                                        FAR struct sockaddr *addr,
                                        socklen_t addrlen,
                                        FAR struct sockaddr *remote_addr);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t g_iperf_ctrl_mutex = PTHREAD_MUTEX_INITIALIZER;
static sq_queue_t      g_iperf_ctrl_list;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

inline static bool iperf_is_udp_client(FAR struct iperf_ctrl_t *ctrl);
inline static bool iperf_is_udp_server(FAR struct iperf_ctrl_t *ctrl);
inline static bool iperf_is_tcp_client(FAR struct iperf_ctrl_t *ctrl);
inline static bool iperf_is_tcp_server(FAR struct iperf_ctrl_t *ctrl);
static int iperf_get_socket_error_code(int sockfd);
static int iperf_show_socket_error_reason(FAR const char *str, int sockfd);
static void iperf_report_task(FAR void *arg);
static int iperf_start_report(FAR struct iperf_ctrl_t *ctrl);
static int iperf_run_tcp_server(FAR struct iperf_ctrl_t *ctrl);
static int iperf_run_udp_server(FAR struct iperf_ctrl_t *ctrl);
static int iperf_run_udp_client(FAR struct iperf_ctrl_t *ctrl);
static int iperf_run_tcp_client(FAR struct iperf_ctrl_t *ctrl);
static void iperf_task_traffic(FAR void *arg);
static uint32_t iperf_get_buffer_len(FAR struct iperf_ctrl_t *ctrl);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iperf_is_udp_client
 *
 * Description:
 *   Check if it is a udp client
 *
 ****************************************************************************/

inline static bool iperf_is_udp_client(FAR struct iperf_ctrl_t *ctrl)
{
  return ((ctrl->cfg.flag & IPERF_FLAG_CLIENT)
         && (ctrl->cfg.flag & IPERF_FLAG_UDP));
}

/****************************************************************************
 * Name: iperf_is_udp_server
 *
 * Description:
 *   Check if it is a udp server
 *
 ****************************************************************************/

inline static bool iperf_is_udp_server(FAR struct iperf_ctrl_t *ctrl)
{
  return ((ctrl->cfg.flag & IPERF_FLAG_SERVER)
         && (ctrl->cfg.flag & IPERF_FLAG_UDP));
}

/****************************************************************************
 * Name: iperf_is_tcp_client
 *
 * Description:
 *   Check if it is a tcp client
 *
 ****************************************************************************/

inline static bool iperf_is_tcp_client(FAR struct iperf_ctrl_t *ctrl)
{
  return ((ctrl->cfg.flag & IPERF_FLAG_CLIENT)
         && (ctrl->cfg.flag & IPERF_FLAG_TCP));
}

/****************************************************************************
 * Name: iperf_is_tcp_server
 *
 * Description:
 *   Check if it is a tcp server
 *
 ****************************************************************************/

inline static bool iperf_is_tcp_server(FAR struct iperf_ctrl_t *ctrl)
{
  return ((ctrl->cfg.flag & IPERF_FLAG_SERVER)
         && (ctrl->cfg.flag & IPERF_FLAG_TCP));
}

/****************************************************************************
 * Name: iperf_get_socket_error_code
 *
 * Description:
 *   Get reason of socket error.
 *
 ****************************************************************************/

static int iperf_get_socket_error_code(int sockfd)
{
  return errno;
}

/****************************************************************************
 * Name: iperf_show_socket_error_reason
 *
 * Description:
 *   Show reason of socket error.
 *
 ****************************************************************************/

static int iperf_show_socket_error_reason(FAR const char *str, int sockfd)
{
  int err = errno;
  if (err != 0)
    {
      printf("%s error, error code: %d, reason: %s\n",
             str, err, strerror(err));
    }

  return err;
}

/****************************************************************************
 * Name: iperf_print_addr
 *
 * Description:
 *   Print addr info
 *
 ****************************************************************************/

static void iperf_print_addr(FAR const char *str, FAR struct sockaddr *addr)
{
  switch (addr->sa_family)
    {
      case AF_INET:
        {
          FAR struct sockaddr_in *inaddr = (FAR struct sockaddr_in *)addr;
          printf("%s: %s:%d\n", str,
                 inet_ntoa(inaddr->sin_addr), htons(inaddr->sin_port));
          return;
        }

      case AF_LOCAL:
        {
          FAR struct sockaddr_un *unaddr = (FAR struct sockaddr_un *)addr;
          printf("%s: path=%s\n", str, unaddr->sun_path);
          return;
        }

      case AF_RPMSG:
        {
          FAR struct sockaddr_rpmsg *rpaddr =
                                          (FAR struct sockaddr_rpmsg *)addr;
          printf("%s: cpu=%s,name=%s\n", str,
                 rpaddr->rp_cpu, rpaddr->rp_name);
          return;
        }

      default:
        assert(false); /* shouldn't happen */
    }
}

/****************************************************************************
 * Name: ts_sec
 *
 * Description:
 *    Convert a timespec to a double.
 *
 ****************************************************************************/

static double ts_sec(const struct timespec *ts)
{
  return (double)ts->tv_sec + (double)ts->tv_nsec / 1e9;
}

/****************************************************************************
 * Name: ts_diff
 *
 * Description:
 *   Return the diff of two timespecs in second.
 *
 ****************************************************************************/

static double ts_diff(const struct timespec *a, const struct timespec *b)
{
  return ts_sec(a) - ts_sec(b);
}

/****************************************************************************
 * Name: iperf_report_task
 *
 * Description:
 *   Start iperf report task
 *
 ****************************************************************************/

static void iperf_report_task(FAR void *arg)
{
  FAR struct iperf_ctrl_t *ctrl = arg;
  uint32_t interval = ctrl->cfg.interval;
  uint32_t time = ctrl->cfg.time;
  struct timespec now;
  struct timespec start;
  uintmax_t now_len;
  int ret;

  prctl(PR_SET_NAME, IPERF_REPORT_TASK_NAME);

  now_len = ctrl->total_len;
  ret = clock_gettime(CLOCK_MONOTONIC, &now);
  if (ret != 0)
    {
      fprintf(stderr, "clock_gettime failed\n");
      exit(EXIT_FAILURE);
    }

  start = now;
  printf("\n%19s %16s %18s\n", "Interval", "Transfer", "Bandwidth\n");
  while (!ctrl->finish)
    {
      uintmax_t last_len;
      struct timespec last;

      sleep(interval);
      last_len = now_len;
      last = now;
      now_len = ctrl->total_len;
      ret = clock_gettime(CLOCK_MONOTONIC, &now);
      if (ret != 0)
        {
          fprintf(stderr, "clock_gettime failed\n");
          exit(EXIT_FAILURE);
        }

      printf("%7.2lf-%7.2lf sec %10ju Bytes %7.2f Mbits/sec\n",
             ts_diff(&last, &start),
             ts_diff(&now, &start),
             now_len -last_len,
             (((now_len - last_len) * 8) / 1000000.0) /
             ts_diff(&now, &last)
             );
      if (time != 0 && ts_diff(&now, &start) >= time)
        {
          break;
        }
    }

  if (ts_diff(&now, &start) > 0)
    {
      printf("%7.2lf-%7.2lf sec %10ju Bytes %7.2f Mbits/sec\n",
             ts_diff(&start, &start),
             ts_diff(&now, &start),
             now_len,
             ((now_len * 8) / 1000000.0) /
             ts_diff(&now, &start)
             );
    }

  ctrl->finish = true;

  pthread_exit(NULL);
}

/****************************************************************************
 * Name: iperf_start_report
 *
 * Description:
 *   Start iperf report
 *
 ****************************************************************************/

static int iperf_start_report(FAR struct iperf_ctrl_t *ctrl)
{
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  int ret;

  pthread_attr_init(&attr);
  param.sched_priority = IPERF_REPORT_TASK_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, IPERF_REPORT_TASK_STACK);

  ret = pthread_create(&thread, &attr, (FAR void *)iperf_report_task,
                       ctrl);
  if (ret != 0)
    {
      printf("iperf_thread: pthread_create failed: %d, %s\n",
             ret, IPERF_REPORT_TASK_NAME);
      return -1;
    }

  pthread_detach(thread);

  return 0;
}

/****************************************************************************
 * Name: iperf_run_server
 *
 * Description:
 *   Start a server
 *
 ****************************************************************************/

static int iperf_run_server(FAR struct iperf_ctrl_t *ctrl,
                            iperf_server_func_t server_func)
{
  if (ctrl->cfg.flag & IPERF_FLAG_LOCAL)
    {
      struct sockaddr_un addr;
      struct sockaddr_un remote_addr;

      addr.sun_family = AF_LOCAL;
      strlcpy(addr.sun_path, ctrl->cfg.path, sizeof(addr.sun_path));

      return server_func(ctrl, (FAR struct sockaddr *)&addr, sizeof(addr),
                               (FAR struct sockaddr *)&remote_addr);
    }
  else if (ctrl->cfg.flag & IPERF_FLAG_RPMSG)
    {
      struct sockaddr_rpmsg addr;
      struct sockaddr_rpmsg remote_addr;

      addr.rp_family = AF_RPMSG;
      strlcpy(addr.rp_cpu, ctrl->cfg.host, sizeof(addr.rp_cpu));
      strlcpy(addr.rp_name, ctrl->cfg.path, sizeof(addr.rp_name));

      return server_func(ctrl, (FAR struct sockaddr *)&addr, sizeof(addr),
                               (FAR struct sockaddr *)&remote_addr);
    }
  else
    {
      struct sockaddr_in addr;
      struct sockaddr_in remote_addr;

      addr.sin_family = AF_INET;
      addr.sin_port = htons(ctrl->cfg.sport);
      addr.sin_addr.s_addr = ctrl->cfg.sip;

      return server_func(ctrl, (FAR struct sockaddr *)&addr, sizeof(addr),
                               (FAR struct sockaddr *)&remote_addr);
    }
}

/****************************************************************************
 * Name: iperf_run_client
 *
 * Description:
 *   Start a client
 *
 ****************************************************************************/

static int iperf_run_client(FAR struct iperf_ctrl_t *ctrl,
                            iperf_client_func_t client_func)
{
  if (ctrl->cfg.flag & IPERF_FLAG_LOCAL)
    {
      struct sockaddr_un addr;

      addr.sun_family = AF_LOCAL;
      strlcpy(addr.sun_path, ctrl->cfg.path, sizeof(addr.sun_path));

      return client_func(ctrl, (FAR struct sockaddr *)&addr, sizeof(addr));
    }
  else if (ctrl->cfg.flag & IPERF_FLAG_RPMSG)
    {
      struct sockaddr_rpmsg addr;

      addr.rp_family = AF_RPMSG;
      strlcpy(addr.rp_cpu, ctrl->cfg.host, sizeof(addr.rp_cpu));
      strlcpy(addr.rp_name, ctrl->cfg.path, sizeof(addr.rp_name));

      return client_func(ctrl, (FAR struct sockaddr *)&addr, sizeof(addr));
    }
  else
    {
      struct sockaddr_in addr;

      addr.sin_family = AF_INET;
      addr.sin_port = htons(ctrl->cfg.dport);
      addr.sin_addr.s_addr = ctrl->cfg.dip;

      return client_func(ctrl, (FAR struct sockaddr *)&addr, sizeof(addr));
    }
}

/****************************************************************************
 * Name: iperf_tcp_server
 *
 * Description:
 *   The main tcp server logic
 *
 ****************************************************************************/

static int iperf_tcp_server(FAR struct iperf_ctrl_t *ctrl,
                            FAR struct sockaddr *addr, socklen_t addrlen,
                            FAR struct sockaddr *remote_addr)
{
  int actual_recv = 0;
  int want_recv = 0;
  FAR uint8_t *buffer;
  int listen_socket;
  struct timeval t;
  int sockfd;
  int opt;

  listen_socket = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
  if (listen_socket < 0)
    {
      iperf_show_socket_error_reason("tcp server create", listen_socket);
      return -1;
    }

  setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (bind(listen_socket, addr, addrlen) != 0)
    {
      iperf_show_socket_error_reason("tcp server bind", listen_socket);
      close(listen_socket);
      return -1;
    }

  if (listen(listen_socket, 5) < 0)
    {
      iperf_show_socket_error_reason("tcp server listen", listen_socket);
      close(listen_socket);
      return -1;
    }

  buffer = ctrl->buffer;
  want_recv = ctrl->buffer_len;
  while (!ctrl->finish)
    {
      /* TODO need to change to non-block mode */

      sockfd = accept4(listen_socket, remote_addr, &addrlen, SOCK_CLOEXEC);
      if (sockfd < 0)
        {
          iperf_show_socket_error_reason("tcp server listen", listen_socket);
          close(listen_socket);
          return -1;
        }
      else
        {
          iperf_print_addr("accept", remote_addr);
          iperf_start_report(ctrl);

          t.tv_sec = IPERF_SOCKET_RX_TIMEOUT;
          t.tv_usec = 0;
          setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
        }

      while (!ctrl->finish)
        {
          actual_recv = recv(sockfd, buffer, want_recv, 0);
          if (actual_recv == 0)
            {
              iperf_print_addr("closed by the peer", remote_addr);

              /* Note: unlike the original iperf, this implementation
               * exits after finishing a single connection.
               */

              ctrl->finish = true;
              break;
            }
          else if (actual_recv < 0)
            {
              iperf_show_socket_error_reason("tcp server recv",
                                             listen_socket);
              ctrl->finish = true;
              break;
            }
          else
            {
              ctrl->total_len += actual_recv;
            }
        }

      close(sockfd);
    }

  ctrl->finish = true;
  close(listen_socket);

  return 0;
}

/****************************************************************************
 * Name: iperf_run_tcp_server
 *
 * Description:
 *   Start tcp server
 *
 ****************************************************************************/

static int iperf_run_tcp_server(FAR struct iperf_ctrl_t *ctrl)
{
  return iperf_run_server(ctrl, iperf_tcp_server);
}

/****************************************************************************
 * Name: iperf_udp_server
 *
 * Description:
 *   The main udp server logic
 *
 ****************************************************************************/

static int iperf_udp_server(FAR struct iperf_ctrl_t *ctrl,
                            FAR struct sockaddr *addr, socklen_t addrlen,
                            FAR struct sockaddr *remote_addr)
{
  int actual_recv = 0;
  struct timeval t;
  int want_recv = 0;
  FAR uint8_t *buffer;
  int sockfd;
  int opt;
  bool udp_recv_start = true;

  sockfd = socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0)
    {
      iperf_show_socket_error_reason("udp server create", sockfd);
      return -1;
    }

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (bind(sockfd, addr, addrlen) != 0)
    {
      iperf_show_socket_error_reason("udp server bind", sockfd);
      return -1;
    }

  buffer = ctrl->buffer;
  want_recv = ctrl->buffer_len;
  printf("want recv=%d\n", want_recv);

  t.tv_sec = IPERF_SOCKET_RX_TIMEOUT;
  t.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));

  while (!ctrl->finish)
    {
      actual_recv = recvfrom(sockfd, buffer, want_recv, 0,
                             remote_addr, &addrlen);
      if (actual_recv < 0)
        {
          iperf_show_socket_error_reason("udp server recv", sockfd);
        }
      else
        {
          if (udp_recv_start == true)
            {
              iperf_print_addr("accept", remote_addr);
              iperf_start_report(ctrl);
              udp_recv_start = false;
            }

          ctrl->total_len += actual_recv;
        }
    }

  ctrl->finish = true;
  close(sockfd);

  return 0;
}

/****************************************************************************
 * Name: iperf_run_udp_server
 *
 * Description:
 *   Start udp server
 *
 ****************************************************************************/

static int iperf_run_udp_server(FAR struct iperf_ctrl_t *ctrl)
{
  return iperf_run_server(ctrl, iperf_udp_server);
}

/****************************************************************************
 * Name: iperf_udp_client
 *
 * Description:
 *   The main udp client logic
 *
 ****************************************************************************/

static int iperf_udp_client(FAR struct iperf_ctrl_t *ctrl,
                            FAR struct sockaddr *addr, socklen_t addrlen)
{
  FAR struct iperf_udp_pkt_t *udp;
  int actual_send = 0;
  bool retry = false;
  uint32_t delay = 1;
  int want_send = 0;
  uint8_t *buffer;
  int sockfd;
  int opt;
  int err;
  int id;

  sockfd = socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0)
    {
      iperf_show_socket_error_reason("udp client create", sockfd);
      return -1;
    }

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  iperf_start_report(ctrl);
  buffer = ctrl->buffer;
  udp = (FAR struct iperf_udp_pkt_t *)buffer;
  want_send = ctrl->buffer_len;
  id = 0;

  while (!ctrl->finish)
    {
      if (false == retry)
        {
          id++;
          udp->id = htonl(id);
          delay = 1;
        }

      retry = false;
      actual_send = sendto(sockfd, buffer, want_send, 0, addr, addrlen);

      if (actual_send != want_send)
        {
          err = iperf_get_socket_error_code(sockfd);
          if (err == ENOMEM)
            {
              usleep(delay * 10000);
              if (delay < IPERF_MAX_DELAY)
                {
                  delay <<= 1;
                }

              retry = true;
              continue;
            }
          else
            {
              printf("udp client send abort: err=%d\n", err);
              break;
            }
        }
      else
        {
          ctrl->total_len += actual_send;
        }
    }

  ctrl->finish = true;
  close(sockfd);

  return 0;
}

/****************************************************************************
 * Name: iperf_run_udp_client
 *
 * Description:
 *   Start udp client
 *
 ****************************************************************************/

static int iperf_run_udp_client(FAR struct iperf_ctrl_t *ctrl)
{
  return iperf_run_client(ctrl, iperf_udp_client);
}

/****************************************************************************
 * Name: iperf_tcp_client
 *
 * Description:
 *   The main tcp client logic
 *
 ****************************************************************************/

static int iperf_tcp_client(FAR struct iperf_ctrl_t *ctrl,
                            FAR struct sockaddr *addr, socklen_t addrlen)
{
  FAR uint8_t *buffer;
  int actual_send = 0;
  int want_send = 0;
  int sockfd;

  sockfd = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0)
    {
      iperf_show_socket_error_reason("tcp client create", sockfd);
      return -1;
    }

  if (connect(sockfd, addr, addrlen) < 0)
    {
      iperf_show_socket_error_reason("tcp client connect", sockfd);
      return -1;
    }

  iperf_start_report(ctrl);
  buffer = ctrl->buffer;
  want_send = ctrl->buffer_len;

  while (!ctrl->finish)
    {
      actual_send = send(sockfd, buffer, want_send, 0);
      if (actual_send <= 0)
        {
          iperf_show_socket_error_reason("tcp client send", sockfd);
          break;
        }
      else
        {
          ctrl->total_len += actual_send;
        }
    }

  ctrl->finish = true;
  close(sockfd);

  return 0;
}

/****************************************************************************
 * Name: iperf_run_tcp_client
 *
 * Description:
 *   Start tcp client
 *
 ****************************************************************************/

static int iperf_run_tcp_client(FAR struct iperf_ctrl_t *ctrl)
{
  return iperf_run_client(ctrl, iperf_tcp_client);
}

/****************************************************************************
 * Name: iperf_task_traffic
 *
 * Description:
 *   Select to run tcp or udp.
 *
 ****************************************************************************/

static void iperf_task_traffic(FAR void *arg)
{
  FAR struct iperf_ctrl_t *ctrl = arg;

  prctl(PR_SET_NAME, IPERF_TRAFFIC_TASK_NAME);

  if (iperf_is_udp_client(ctrl))
    {
      iperf_run_udp_client(ctrl);
    }
  else if (iperf_is_udp_server(ctrl))
    {
      iperf_run_udp_server(ctrl);
    }
  else if (iperf_is_tcp_client(ctrl))
    {
      iperf_run_tcp_client(ctrl);
    }
  else if (iperf_is_tcp_server(ctrl))
    {
      iperf_run_tcp_server(ctrl);
    }
  else
    {
      /* shouldn't happen */

      assert(false);
    }

  if (ctrl->buffer)
    {
      free(ctrl->buffer);
      ctrl->buffer = NULL;
    }

  printf("iperf exit\n");

  pthread_exit(NULL);
}

static uint32_t iperf_get_buffer_len(FAR struct iperf_ctrl_t *ctrl)
{
  if (iperf_is_udp_client(ctrl))
    {
      return IPERF_UDP_TX_LEN;
    }
  else if (iperf_is_udp_server(ctrl))
    {
      return IPERF_UDP_RX_LEN;
    }
  else if (iperf_is_tcp_client(ctrl))
    {
      return IPERF_TCP_TX_LEN;
    }
  else
    {
      return IPERF_TCP_RX_LEN;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iperf_start
 *
 * Description:
 *   Start iperf task.
 *
 ****************************************************************************/

int iperf_start(FAR struct iperf_cfg_t *cfg)
{
  struct iperf_ctrl_t ctrl;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  FAR void *retval;
  int ret;

  if (!cfg)
    {
      return -1;
    }

  memset(&ctrl, 0, sizeof(ctrl));
  memcpy(&ctrl.cfg, cfg, sizeof(*cfg));
  ctrl.finish = false;
  ctrl.buffer_len = iperf_get_buffer_len(&ctrl);
  ctrl.buffer = (FAR uint8_t *)malloc(ctrl.buffer_len);
  if (ctrl.buffer == NULL)
    {
      printf("create buffer: not enough memory\n");
      return -1;
    }

  memset(ctrl.buffer, 0, ctrl.buffer_len);
  pthread_attr_init(&attr);
  param.sched_priority = IPERF_TRAFFIC_TASK_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, IPERF_TRAFFIC_TASK_STACK);
  ret = pthread_create(&thread, &attr, (FAR void *)iperf_task_traffic,
                       &ctrl);

  if (ret != 0)
    {
      printf("iperf_task_traffic: create task failed: %d\n", ret);
      free(ctrl.buffer);
      ctrl.buffer = NULL;
      return -1;
    }

  pthread_mutex_lock(&g_iperf_ctrl_mutex);
  sq_addlast((FAR sq_entry_t *)&ctrl, &g_iperf_ctrl_list);
  pthread_mutex_unlock(&g_iperf_ctrl_mutex);

  pthread_join(thread, &retval);

  pthread_mutex_lock(&g_iperf_ctrl_mutex);
  sq_rem((FAR sq_entry_t *)&ctrl, &g_iperf_ctrl_list);
  pthread_mutex_unlock(&g_iperf_ctrl_mutex);

  return 0;
}

/****************************************************************************
 * Name: iperf_stop
 *
 * Description:
 *   Stop iperf task.
 *
 ****************************************************************************/

int iperf_stop(void)
{
  FAR struct iperf_ctrl_t *ctrl;
  FAR sq_entry_t *tmp;
  FAR sq_entry_t *p;

  pthread_mutex_lock(&g_iperf_ctrl_mutex);

  sq_for_every_safe(&g_iperf_ctrl_list, p, tmp)
    {
      ctrl = (FAR struct iperf_ctrl_t *)p;
      ctrl->finish = true;
      sq_rem(p, &g_iperf_ctrl_list);
    }

  pthread_mutex_unlock(&g_iperf_ctrl_mutex);

  return 0;
}
