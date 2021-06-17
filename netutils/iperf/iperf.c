/****************************************************************************
 * apps/netutils/iperf/iperf.c
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

#include <sys/socket.h>
#include <sys/time.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdbool.h>
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
#define IPERF_REPORT_TASK_NAME       "iperf_report"

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
  struct iperf_cfg_t cfg;
  bool finish;
  uintmax_t total_len;
  uint32_t buffer_len;
  uint8_t *buffer;
  uint32_t sockfd;
};

struct iperf_udp_pkt_t
{
  int32_t id;
  uint32_t sec;
  uint32_t usec;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool s_iperf_is_running = false;
static struct iperf_ctrl_t s_iperf_ctrl;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

inline static bool iperf_is_udp_client(void);
inline static bool iperf_is_udp_server(void);
inline static bool iperf_is_tcp_client(void);
inline static bool iperf_is_tcp_server(void);
static int iperf_get_socket_error_code(int sockfd);
static int iperf_show_socket_error_reason(const char *str, int sockfd);
static void iperf_report_task(void *arg);
static int iperf_start_report(void);
static int iperf_run_tcp_server(void);
static int iperf_run_udp_server(void);
static int iperf_run_udp_client(void);
static int iperf_run_tcp_client(void);
static void iperf_task_traffic(void *arg);
static uint32_t iperf_get_buffer_len(void);

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

inline static bool iperf_is_udp_client(void)
{
  return ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT)
         && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP));
}

/****************************************************************************
 * Name: iperf_is_udp_server
 *
 * Description:
 *   Check if it is a udp server
 *
 ****************************************************************************/

inline static bool iperf_is_udp_server(void)
{
  return ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER)
         && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP));
}

/****************************************************************************
 * Name: iperf_is_tcp_client
 *
 * Description:
 *   Check if it is a tcp client
 *
 ****************************************************************************/

inline static bool iperf_is_tcp_client(void)
{
  return ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT)
         && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP));
}

/****************************************************************************
 * Name: iperf_is_tcp_server
 *
 * Description:
 *   Check if it is a tcp server
 *
 ****************************************************************************/

inline static bool iperf_is_tcp_server(void)
{
  return ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER)
         && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP));
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

static int iperf_show_socket_error_reason(const char *str, int sockfd)
{
  int err = errno;
  if (err != 0)
    {
      printf("%s error, error code: %d, reason: %s",
             str, err, strerror(err));
    }

  return err;
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

static void iperf_report_task(void *arg)
{
  uint32_t interval = s_iperf_ctrl.cfg.interval;
  uint32_t time = s_iperf_ctrl.cfg.time;
  struct timespec now;
  struct timespec start;
  uintmax_t now_len;
  int ret;
#ifdef CONFIG_CLOCK_MONOTONIC
  const clockid_t clockid = CLOCK_MONOTONIC;
#else
  const clockid_t clockid = CLOCK_REALTIME;
#endif

  now_len = s_iperf_ctrl.total_len;
  ret = clock_gettime(clockid, &now);
  if (ret != 0)
    {
      fprintf(stderr, "clock_gettime failed\n");
      exit(EXIT_FAILURE);
    }

  start = now;
  printf("\n%19s %16s %18s\n", "Interval", "Transfer", "Bandwidth\n");
  while (!s_iperf_ctrl.finish)
    {
      uintmax_t last_len;
      struct timespec last;

      sleep(interval);
      last_len = now_len;
      last = now;
      now_len = s_iperf_ctrl.total_len;
      ret = clock_gettime(clockid, &now);
      if (ret != 0)
        {
          fprintf(stderr, "clock_gettime failed\n");
          exit(EXIT_FAILURE);
        }

      printf("%7.2lf-%7.2lf sec %10ju Bytes %7.2f Mbits/sec\n",
             ts_diff(&last, &start),
             ts_diff(&now, &start),
             now_len,
             (((double)(now_len - last_len) * 8) /
             ts_diff(&now, &last) / 1e6));
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
             (((double)now_len * 8) /
             ts_diff(&now, &start) / 1e6));
    }

  s_iperf_ctrl.finish = true;

  pthread_exit(NULL);
}

/****************************************************************************
 * Name: iperf_start_report
 *
 * Description:
 *   Start iperf report
 *
 ****************************************************************************/

static int iperf_start_report(void)
{
  int ret;
  pthread_t thread;
  struct sched_param param;
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  param.sched_priority = IPERF_REPORT_TASK_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, IPERF_REPORT_TASK_STACK);

  ret = pthread_create(&thread, &attr, (void *)iperf_report_task,
                       IPERF_REPORT_TASK_NAME);
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
 * Name: iperf_run_tcp_server
 *
 * Description:
 *   Start tcp server
 *
 ****************************************************************************/

static int iperf_run_tcp_server(void)
{
  socklen_t addr_len = sizeof(struct sockaddr);
  struct sockaddr_in remote_addr;
  struct sockaddr_in addr;
  int actual_recv = 0;
  int want_recv = 0;
  uint8_t *buffer;
  int listen_socket;
  struct timeval t;
  int sockfd;
  int opt;

  listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_socket < 0)
    {
      iperf_show_socket_error_reason("tcp server create", listen_socket);
      return -1;
    }

  setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(s_iperf_ctrl.cfg.sport);
  addr.sin_addr.s_addr = s_iperf_ctrl.cfg.sip;
  if (bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
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

  buffer = s_iperf_ctrl.buffer;
  want_recv = s_iperf_ctrl.buffer_len;
  while (!s_iperf_ctrl.finish)
    {
      /* TODO need to change to non-block mode */

      sockfd = accept(listen_socket, (struct sockaddr *)&remote_addr,
                      &addr_len);
      if (sockfd < 0)
        {
          iperf_show_socket_error_reason("tcp server listen", listen_socket);
          close(listen_socket);
          return -1;
        }
      else
        {
          printf("accept: %s,%d\n", inet_ntoa(remote_addr.sin_addr),
                 htons(remote_addr.sin_port));
          iperf_start_report();

          t.tv_sec = IPERF_SOCKET_RX_TIMEOUT;
          setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
        }

      while (!s_iperf_ctrl.finish)
        {
          actual_recv = recv(sockfd, buffer, want_recv, 0);
          if (actual_recv == 0)
            {
              printf("closed by the peer: %s,%d\n",
                     inet_ntoa(remote_addr.sin_addr),
                     htons(remote_addr.sin_port));

              /* Note: unlike the original iperf, this implementation
               * exits after finishing a single connection.
               */

              s_iperf_ctrl.finish = true;
              break;
            }
          else if (actual_recv < 0)
            {
              iperf_show_socket_error_reason("tcp server recv",
                                             listen_socket);
              s_iperf_ctrl.finish = true;
              break;
            }
          else
            {
              s_iperf_ctrl.total_len += actual_recv;
            }
        }

      close(sockfd);
    }

  s_iperf_ctrl.finish = true;
  close(listen_socket);

  return 0;
}

/****************************************************************************
 * Name: iperf_run_udp_server
 *
 * Description:
 *   Start udp server
 *
 ****************************************************************************/

static int iperf_run_udp_server(void)
{
  socklen_t addr_len = sizeof(struct sockaddr_in);
  struct sockaddr_in addr;
  int actual_recv = 0;
  struct timeval t;
  int want_recv = 0;
  uint8_t *buffer;
  int sockfd;
  int opt;
  bool udp_recv_start = true;

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0)
    {
      iperf_show_socket_error_reason("udp server create", sockfd);
      return -1;
    }

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(s_iperf_ctrl.cfg.sport);
  addr.sin_addr.s_addr = s_iperf_ctrl.cfg.sip;
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
      iperf_show_socket_error_reason("udp server bind", sockfd);
      return -1;
    }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(s_iperf_ctrl.cfg.sport);
  addr.sin_addr.s_addr = s_iperf_ctrl.cfg.sip;

  buffer = s_iperf_ctrl.buffer;
  want_recv = s_iperf_ctrl.buffer_len;
  printf("want recv=%d", want_recv);

  t.tv_sec = IPERF_SOCKET_RX_TIMEOUT;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));

  while (!s_iperf_ctrl.finish)
    {
      actual_recv = recvfrom(sockfd, buffer, want_recv, 0,
                             (struct sockaddr *)&addr, &addr_len);
      if (actual_recv < 0)
        {
          iperf_show_socket_error_reason("udp server recv", sockfd);
        }
      else
        {
          if (udp_recv_start == true)
            {
              iperf_start_report();
              udp_recv_start = false;
            }

          s_iperf_ctrl.total_len += actual_recv;
        }
    }

  s_iperf_ctrl.finish = true;
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

static int iperf_run_udp_client(void)
{
  struct sockaddr_in addr;
  struct iperf_udp_pkt_t *udp;
  int actual_send = 0;
  bool retry = false;
  uint32_t delay = 1;
  int want_send = 0;
  uint8_t *buffer;
  int sockfd;
  int opt;
  int err;
  int id;

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0)
    {
      iperf_show_socket_error_reason("udp client create", sockfd);
      return -1;
    }

  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(s_iperf_ctrl.cfg.dport);
  addr.sin_addr.s_addr = s_iperf_ctrl.cfg.dip;

  iperf_start_report();
  buffer = s_iperf_ctrl.buffer;
  udp = (struct iperf_udp_pkt_t *)buffer;
  want_send = s_iperf_ctrl.buffer_len;
  id = 0;

  while (!s_iperf_ctrl.finish)
    {
      if (false == retry)
        {
          id++;
          udp->id = htonl(id);
          delay = 1;
        }

      retry = false;
      actual_send = sendto(sockfd, buffer, want_send, 0,
                           (struct sockaddr *)&addr, sizeof(addr));

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
              printf("udp client send abort: err=%d", err);
              break;
            }
        }
      else
        {
          s_iperf_ctrl.total_len += actual_send;
        }
    }

  s_iperf_ctrl.finish = true;
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

static int iperf_run_tcp_client(void)
{
  struct sockaddr_in remote_addr;
  int actual_send = 0;
  int want_send = 0;
  uint8_t *buffer;
  int sockfd;

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0)
    {
      iperf_show_socket_error_reason("tcp client create", sockfd);
      return -1;
    }

  memset(&remote_addr, 0, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(s_iperf_ctrl.cfg.dport);
  remote_addr.sin_addr.s_addr = s_iperf_ctrl.cfg.dip;
  if (connect(sockfd, (struct sockaddr *)&remote_addr,
              sizeof(remote_addr)) < 0)
    {
      iperf_show_socket_error_reason("tcp client connect", sockfd);
      return -1;
    }

  iperf_start_report();
  buffer = s_iperf_ctrl.buffer;
  want_send = s_iperf_ctrl.buffer_len;

  while (!s_iperf_ctrl.finish)
    {
      actual_send = send(sockfd, buffer, want_send, 0);
      if (actual_send <= 0)
        {
          iperf_show_socket_error_reason("tcp client send", sockfd);
          break;
        }
      else
        {
          s_iperf_ctrl.total_len += actual_send;
        }
    }

  s_iperf_ctrl.finish = true;
  close(sockfd);

  return 0;
}

/****************************************************************************
 * Name: iperf_task_traffic
 *
 * Description:
 *   Select to run tcp or udp.
 *
 ****************************************************************************/

static void iperf_task_traffic(void *arg)
{
  if (iperf_is_udp_client())
    {
      iperf_run_udp_client();
    }
  else if (iperf_is_udp_server())
    {
      iperf_run_udp_server();
    }
  else if (iperf_is_tcp_client())
    {
      iperf_run_tcp_client();
    }
  else if (iperf_is_tcp_server())
    {
      iperf_run_tcp_server();
    }
  else
    {
      /* shouldn't happen */

      assert(false);
    }

  if (s_iperf_ctrl.buffer)
    {
      free(s_iperf_ctrl.buffer);
      s_iperf_ctrl.buffer = NULL;
    }

  printf("iperf exit");
  s_iperf_is_running = false;

  pthread_exit(NULL);
}

static uint32_t iperf_get_buffer_len(void)
{
  if (iperf_is_udp_client())
    {
      return IPERF_UDP_TX_LEN;
    }
  else if (iperf_is_udp_server())
    {
      return IPERF_UDP_RX_LEN;
    }
  else if (iperf_is_tcp_client())
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

int iperf_start(struct iperf_cfg_t *cfg)
{
  int ret;
  void *retval;
  struct sched_param param;
  pthread_t thread;
  pthread_attr_t attr;

  if (!cfg)
    {
      return -1;
    }

  if (s_iperf_is_running)
    {
      printf("iperf is running\n");
      return -1;
    }

  memset(&s_iperf_ctrl, 0, sizeof(s_iperf_ctrl));
  memcpy(&s_iperf_ctrl.cfg, cfg, sizeof(*cfg));
  s_iperf_is_running = true;
  s_iperf_ctrl.finish = false;
  s_iperf_ctrl.buffer_len = iperf_get_buffer_len();
  s_iperf_ctrl.buffer = (uint8_t *)malloc(s_iperf_ctrl.buffer_len);
  if (!s_iperf_ctrl.buffer)
    {
      printf("create buffer: not enough memory\n");
      return -1;
    }

  memset(s_iperf_ctrl.buffer, 0, s_iperf_ctrl.buffer_len);
  pthread_attr_init(&attr);
  param.sched_priority = IPERF_TRAFFIC_TASK_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, IPERF_TRAFFIC_TASK_STACK);
  ret = pthread_create(&thread, &attr, (void *)iperf_task_traffic,
                       IPERF_TRAFFIC_TASK_NAME);

  if (ret != 0)
    {
      printf("iperf_task_traffic: create task failed: %d\n", ret);
      free(s_iperf_ctrl.buffer);
      s_iperf_ctrl.buffer = NULL;
      return -1;
    }

  pthread_join(thread, &retval);
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
  if (s_iperf_is_running)
    {
      s_iperf_ctrl.finish = true;
    }

  while (s_iperf_is_running)
    {
      printf("wait current iperf to stop ...\n");
      usleep(300 * 1000);
    }

  return 0;
}
