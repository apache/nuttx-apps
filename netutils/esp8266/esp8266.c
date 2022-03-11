/****************************************************************************
 * apps/netutils/esp8266/esp8266.c
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

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <poll.h>
#include <termios.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <netdb.h>
#include <arpa/inet.h>

#include "netutils/esp8266.h"

#ifdef CONFIG_NETUTILS_ESP8266

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NETAPP_IPCONFIG_MAC_OFFSET     (20)

#ifndef CONFIG_NETUTILS_ESP8266_MAXTXLEN
#   define CONFIG_NETUTILS_ESP8266_MAXTXLEN  256
#endif

#ifndef CONFIG_NETUTILS_ESP8266_MAXRXLEN
#   define CONFIG_NETUTILS_ESP8266_MAXRXLEN  256
#endif

#if (CONFIG_NETUTILS_ESP8266_MAXRXLEN < CONFIG_NETUTILS_ESP8266_WORKER_BUF_LEN)
#   error "CONFIG_NETUTILS_ESP8266_WORKER_BUF_LEN would be bigger than CONFIG_NETUTILS_ESP8266_MAXRXLEN"
#endif

#define BUF_CMD_LEN     CONFIG_NETUTILS_ESP8266_MAXTXLEN
#define BUF_ANS_LEN     CONFIG_NETUTILS_ESP8266_MAXRXLEN
#define BUF_WORKER_LEN  CONFIG_NETUTILS_ESP8266_WORKER_BUF_LEN

#define CON_NBR 4

#define ESP8266_ACCESS_POINT_NBR_MAX   32

#define LESP_WAITING_OK_POLLING_MS     250
#define LESP_TIMEOUT_FLUSH_MS          100
#define LESP_TIMEOUT_MS                1000
#define LESP_TIMEOUT_MS_SEND           1000
#define LESP_TIMEOUT_MS_CONNECTION     30000
#define LESP_TIMEOUT_MS_LISP_AP        5000
#define LESP_TIMEOUT_FLOODING_OFFSET_S 3
#define LESP_TIMEOUT_MS_RECV_S         60

#define LESP_CON_USED_MASK(idx)        (1<<(idx))
#define LESP_POLLING_TIME_MS           1000

/* Must be a power of 2 */

#define SOCKET_FIFO_SIZE              2048
#define SOCKET_NBR                    4

#define FLAGS_SOCK_USED               (1 << 0)
#define FLAGS_SOCK_CONNECTED          (1 << 1)

#define FLAGS_SOCK_TYPE_MASK          (3 << 2)
#define FLAGS_SOCK_TYPE_TCP           (0 << 2)
#define FLAGS_SOCK_TYPE_UDP           (1 << 2)
#define FLAGS_SOCK_TYPE_SSL           (2 << 2) /* non standard but useful */

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  LESP_ERR  = -1,
  LESP_NONE =  0,
  LESP_OK   =  1
}lesp_ans_t;

typedef struct
{
  sem_t          *sem;
  uint8_t         flags;
  uint16_t        inndx;
  uint16_t        outndx;
  struct timespec rcv_timeo;
  uint8_t         rxbuf[SOCKET_FIFO_SIZE];
} lesp_socket_t;

typedef struct
{
  bool            running;
  pthread_t       thread;

  char            rxbuf[BUF_WORKER_LEN];

  sem_t           sem;              /* Inform that something is received */
  char            buf[BUF_ANS_LEN]; /* Last complete line received */
  lesp_ans_t      and;              /* Last and received (OK,FAIL or ERROR) */
  pthread_mutex_t mutex;
} lesp_worker_t;

typedef struct
{
  pthread_mutex_t mutex;
  bool            is_initialized;
  int             fd;
  lesp_worker_t   worker;
  lesp_socket_t   sockets[SOCKET_NBR];
  lesp_ans_t      and;
  char            bufans[BUF_ANS_LEN];
  char            bufcmd[BUF_CMD_LEN];
  struct hostent  hostent;

  /* ESP Got only One ip + 1 for NULL that indicate end of list */

  in_addr_t *     h_addr_list_buf[2];

  in_addr_t       in_addr;
} lesp_state_t;

/****************************************************************************
 *  Private Functions prototypes
 ****************************************************************************/

static int lesp_low_level_read(uint8_t *buf, int size);
static inline int lesp_read_ipd(int sockfd, int len);
static lesp_socket_t *get_sock(int sockfd);

/****************************************************************************
 * Private Data
 ****************************************************************************/

lesp_state_t g_lesp_state =
{
  .mutex          = PTHREAD_MUTEX_INITIALIZER,
  .is_initialized = false,
  .fd             = -1,
  .worker.running = false,
  .worker.and     = LESP_NONE,
  .worker.mutex   = PTHREAD_MUTEX_INITIALIZER,
  .and            = LESP_NONE,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lesp_clear_read_buffer
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline void lesp_clear_read_buffer(void)
{
  g_lesp_state.bufans[0] = '\0';
}

/****************************************************************************
 * Name: lesp_clear_read_ans
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline void lesp_clear_read_ans(void)
{
  g_lesp_state.and = LESP_NONE;
}

/****************************************************************************
 * Name: lesp_str_to_unsigned
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *   unsigned value
 *
 ****************************************************************************/

static inline int lesp_str_to_unsigned(char **p_ptr, char end)
{
  int nbr = 0;
  char *ptr = *p_ptr;

  while (*ptr != end)
    {
      char c = *ptr++ - '0';
      if ((c < 0) || (c >= 10))
          return -1;
      nbr *= 10;
      nbr += c;
    }

  *p_ptr = ptr + 1; /* Pass the end char */
  return nbr;
}

/****************************************************************************
 * Name: lesp_set_baudrate
 *
 * Description:
 *   Set com port to baudrate.
 *
 * Input Parameters:
 *   sockfd : int baudrate
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

#ifdef CONFIG_SERIAL_TERMIOS
static int lesp_set_baudrate(int baudrate)
{
  struct termios term;

  if (ioctl(g_lesp_state.fd, TCGETS, (unsigned long)&term) < 0)
    {
      nerr("ERROR: TCGETS failed.\n");
      return -1;
    }

  if ((cfsetispeed(&term, baudrate) < 0) ||
      (cfsetospeed(&term, baudrate) < 0))
    {
      nerr("ERROR: Cannot set baudrate %0x08X\n", baudrate);
      return -1;
    }

  if (ioctl(g_lesp_state.fd, TCSETS, (unsigned long)&term) < 0)
    {
      nerr("ERROR: TCSETS failed.\n");
      return -1;
    }

  return 0;
}
#endif

/****************************************************************************
 * Name: get_sock
 *
 * Description:
 *   Get socket structure pointer of sockfd.
 *
 * Input Parameters:
 *   sockfd : socket id
 *
 * Returned Value:
 *   socket id pointer, NULL on error.
 *
 ****************************************************************************/

static lesp_socket_t *get_sock(int sockfd)
{
  DEBUGASSERT(sockfd >= 0);

  if (!g_lesp_state.is_initialized)
    {
      errno = ENETDOWN;
      ninfo("Esp8266 not initialized; can't list access points\n");
      return NULL;
    }

  if (((unsigned int)sockfd) >= SOCKET_NBR)
    {
      errno = EINVAL;
      ninfo("Esp8266 invalid sockfd\n", sockfd);
      return NULL;
    }

  if ((g_lesp_state.sockets[sockfd].flags & FLAGS_SOCK_USED) == 0)
    {
      errno = EPERM;
      nerr("ERROR: Connection id %d not Created!\n", sockfd);
      return NULL;
    }

  return &g_lesp_state.sockets[sockfd];
}

/****************************************************************************
 * Name: get_sock_protected
 *
 * Description:
 *   Get socket structure pointer of sockfd with worker.mutex portection.
 *
 * Input Parameters:
 *   sockfd : socket id
 *
 * Returned Value:
 *   socket id pointer, NULL on error.
 *
 ****************************************************************************/

static lesp_socket_t *get_sock_protected(int sockfd)
{
  lesp_socket_t *ret;

  pthread_mutex_lock(&g_lesp_state.worker.mutex);
  ret = get_sock(sockfd);
  pthread_mutex_unlock(&g_lesp_state.worker.mutex);
  return ret;
}

/****************************************************************************
 * Name: set_sock_closed
 *
 * Description:
 *   Set socket sockfd mark as closed without check and don't send message to
 *   esp8266.
 *
 * Input Parameters:
 *   sockfd : socket id
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void set_sock_closed(int sockfd)
{
  sem_t *sem;
  lesp_socket_t *sock;

  DEBUGASSERT(((unsigned int)sockfd) < SOCKET_NBR);

  sock         = &g_lesp_state.sockets[sockfd];
  sem          = sock->sem;
  sock->sem    = NULL;
  sock->flags  = 0;
  sock->inndx  = 0;
  sock->outndx = 0;

  ninfo("Socket %d closed\n", sockfd);

  if (sem != NULL)
    {
      sem_post(sem);
    }
}

/****************************************************************************
 * Name: lesp_low_level_read
 *
 * Description:
 *   put size data from esp8266 into buf.
 *
 * Input Parameters:
 *   buf  : buffer to store read data
 *   size : size of data to read
 *
 * Returned Value:
 *    number of byte read, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_low_level_read(uint8_t *buf, int size)
{
  int ret = 0;

  struct pollfd fds[1];

  memset(fds, 0, sizeof(struct pollfd));
  fds[0].fd       = g_lesp_state.fd;
  fds[0].events   = POLLIN;

  /* poll return 1=>even occur 0=>timeout or -1=>error */

  ret = poll(fds, 1, LESP_POLLING_TIME_MS);
  if (ret < 0)
    {
      int errcode = errno;
      nerr("ERROR: worker read Error %d (errno %d)\n", ret, errcode);
      UNUSED(errcode);
    }
  else if ((fds[0].revents & POLLERR) && (fds[0].revents & POLLHUP))
    {
      nerr("ERROR: worker poll read Error %d\n", ret);
      ret = -1;
    }
  else if (fds[0].revents & POLLIN)
    {
      ret = read(g_lesp_state.fd, buf, size);
    }

  if (ret < 0)
    {
      return -1;
    }

  return ret;
}

/****************************************************************************
 * Name: lesp_read_ipd
 *
 * Description:
 *   Try to treat an '+IPD' command in worker buffer.  Worker buffer should
 *   already contain '+IPD,<id>,<len>:'
 *
 * Note:
 *  g_lesp_state.worker.mutex should be locked.
 *
 * Input Parameters:
 *   sockfd : socker number to put received data.
 *   len : data size to read on serial port.
 *
 * Returned Value:
 *   1 was an IPD, 0 is not an IPD, -1 on error.
 *
 ****************************************************************************/

static inline int lesp_read_ipd(int sockfd, int len)
{
  lesp_socket_t *sock;

  sock = get_sock(sockfd);

  ninfo("Read %d bytes for socket %d\n", len, sockfd);

  if (sock == NULL)
    {
      nwarn("socket not opened: drop all data.\n");
    }

  while (len)
    {
      int size;
      uint8_t *buf = (uint8_t *)g_lesp_state.worker.rxbuf;

      size = len;
      if (size >= BUF_WORKER_LEN)
        {
          size = BUF_WORKER_LEN;
        }

      size = lesp_low_level_read(buf, size);
      if (size <= 0)
        {
          return -1;
        }

      len -= size;

      if (sock != NULL)
        {
          while (size--)
            {
              int next;
              uint8_t b;

              /* Read the next byte from the buffer */

              b = *buf++;

              /* Pre-calculate the next 'inndx'.  We do this so that we can
               * check if the FIFO is full.
               */

              next = sock->inndx + 1;
              if (next >= SOCKET_FIFO_SIZE)
                {
                  next -= SOCKET_FIFO_SIZE;
                }

              /* Is there space in the circular buffer for another byte?  If
               * the next 'inndx' would be equal to the 'outndx', then the
               * circular buffer is full.
               */

              if (next == sock->outndx)
                {
                  pthread_mutex_unlock(&g_lesp_state.worker.mutex);
                  usleep(100); /* leave time of aplicative to read buffer */
                  pthread_mutex_lock(&g_lesp_state.worker.mutex);
                }

              if (next != sock->outndx)
                {
                  /* Yes.. add the byte to the circular buffer */

                  sock->rxbuf[sock->inndx] = b;
                  sock->inndx = next;
                }
              else
                {
                  /* No.. the we have lost data */

                  nwarn("overflow socket 0x%02X\n", b);
                }
            }

          if (sock->sem)
            {
              ninfo("post %p\n", sock->sem);
              sem_post(sock->sem);
            }
        }
    }

  return 1;
}

/****************************************************************************
 * Name: lesp_vsend_cmd
 *
 * Description:
 *   Send cmd with format and argument like sprintf.
 *
 * Input Parameters:
 *   format : null terminated format string to send.
 *   ap     : format values.
 *
 * Returned Value:
 *   len of cmd on success, -1 on error.
 *
 ****************************************************************************/

int lesp_vsend_cmd(FAR const IPTR char *format, va_list ap)
{
  int ret = 0;

  ret = vsnprintf(g_lesp_state.bufcmd, BUF_CMD_LEN, format, ap);
  if (ret >= BUF_CMD_LEN)
    {
      g_lesp_state.bufcmd[BUF_CMD_LEN - 1] = '\0';
      ninfo("Buffer too small for '%s'...\n", g_lesp_state.bufcmd);
      ret = -1;
    }

  ninfo("Write:%s\n", g_lesp_state.bufcmd);

  ret = write(g_lesp_state.fd, g_lesp_state.bufcmd, ret);
  if (ret < 0)
    {
      ret = -1;
    }

  return ret;
}

/****************************************************************************
 * Name: lesp_send_cmd
 *
 * Description:
 *   Send cmd with format and argument like sprintf.
 *
 * Input Parameters:
 *   format : null terminated format string to send.
 *   ...    : format values.
 *
 * Note it clear AT Rx buffer (not socket buffer)
 *
 * Returned Value:
 *   len of cmd on success, -1 on error.
 *
 ****************************************************************************/

static int lesp_send_cmd(FAR const IPTR char *format, ...)
{
  int ret = 0;
  va_list ap;

  lesp_clear_read_buffer();
  lesp_clear_read_ans();

  /* Let lesp_vsend_cmd do the real work */

  va_start(ap, format);
  ret = lesp_vsend_cmd(format, ap);
  va_end(ap);

  return ret;
}

/****************************************************************************
 * Name: lesp_read
 *
 * Description:
 *   Read a answer line with timeout.
 *
 * Input Parameters:
 *   timeout_ms : timeout in millisecond.
 *
 * Returned Value:
 *   len of line without line return on success, -1 on error.
 *
 ****************************************************************************/

static int lesp_read(int timeout_ms)
{
  int ret = 0;

  struct timespec ts;

  if (! g_lesp_state.is_initialized)
    {
      ninfo("Esp8266 not initialized; can't list access points\n");
      return -1;
    }

  if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
    {
      return -1;
    }

  ts.tv_nsec += (timeout_ms % 1000) * 1000000;
  if (ts.tv_nsec >= 1000000000)
    {
      ts.tv_nsec -= 1000000000;
      ts.tv_sec  += 1;
    }

  ts.tv_sec  += (timeout_ms / 1000);

  do
    {
      if (sem_timedwait(&g_lesp_state.worker.sem, &ts) < 0)
        {
          return -1;
        }

      pthread_mutex_lock(&g_lesp_state.worker.mutex);

      if (g_lesp_state.worker.and != LESP_NONE)
        {
          g_lesp_state.and = g_lesp_state.worker.and;
          g_lesp_state.worker.and = LESP_NONE;
        }

      ret = strlen(g_lesp_state.worker.buf);
      if (ret > 0)
        {
          /* +1 to copy null */

          memcpy(g_lesp_state.bufans, g_lesp_state.worker.buf, ret + 1);
        }

      g_lesp_state.worker.buf[0] = '\0'; /* buffer is read */
      pthread_mutex_unlock(&g_lesp_state.worker.mutex);
    }
  while ((ret <= 0) && (g_lesp_state.and == LESP_NONE));

  ninfo("lesp_read %d=>%s and and = %d\n", ret, g_lesp_state.bufans,
        g_lesp_state.and);

  return ret;
}

/****************************************************************************
 * Name: lesp_flush
 *
 * Description:
 *   Read and discard all waiting data in rx buffer.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void lesp_flush(void)
{
  do
    {
      lesp_clear_read_buffer();
      lesp_clear_read_ans();
    }
  while (lesp_read(LESP_TIMEOUT_FLUSH_MS) >= 0);
}

/****************************************************************************
 * Name: lesp_read_ans_ok
 *
 * Description:
 *   Read up to read OK, ERROR, or FAIL.
 *
 * Input Parameters:
 *   timeout_ms : timeout in millisecond.
 *
 * Returned Value:
 *   0 on OK, -1 on error.
 *
 ****************************************************************************/

int lesp_read_ans_ok(int timeout_ms)
{
  int ret = 0;
  time_t end;

  end = time(NULL) + (timeout_ms / 1000) +
        LESP_TIMEOUT_FLOODING_OFFSET_S;

  while (g_lesp_state.and != LESP_OK)
    {
      ret = lesp_read(timeout_ms);

      if ((ret < 0) || (g_lesp_state.and == LESP_ERR) ||
          (time(NULL) > end))
        {
          ret = -1;
          break;
        }

      ninfo("Got:%s\n", g_lesp_state.bufans);
    }

  lesp_clear_read_ans();
  lesp_clear_read_buffer();

  return ret;
}

/****************************************************************************
 * Name: lesp_ask_ans_ok
 *
 * Description:
 *   Ask and ignore line start with '+' and except a "OK" answer.
 *
 * Input Parameters:
 *   cmd        : command sent
 *   timeout_ms : timeout in millisecond
 *
 * Returned Value:
 *   len of line without line return on success, -1 on error.
 *
 ****************************************************************************/

static int lesp_ask_ans_ok(int timeout_ms, FAR const IPTR char *format, ...)
{
  int ret = 0;
  va_list ap;

  va_start(ap, format);
  ret = lesp_vsend_cmd(format, ap);
  va_end(ap);

  if (ret >= 0)
    {
      ret = lesp_read_ans_ok(timeout_ms);
    }

  return ret;
}

/****************************************************************************
 * Name: lesp_check
 *
 * Description:
 *   check if esp is ready (initialized and AT return OK)
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   0 : all is ok, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_check(void)
{
  if (! g_lesp_state.is_initialized)
    {
      nerr("ERROR: ESP8266 not initialized\n");
      return -1;
    }

  lesp_flush();

  if (lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT\r\n") < 0)
    {
      nerr("ERROR: ESP8266 not answer at AT command\n");
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_parse_cipdomain_ans_line
 *
 * Description:
 *   Try to decode line start with:
 *      +CIPDOMAIN:
 *   see in:
 *   https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/
 *   or
 *   http://www.espressif.com/sites/default/files/documentation/
 *                                     4a-esp8266_at_instruction_set_en.pdf
 *
 *                net ip
 *    +CIPDOMAIN:"192.168.1.1"
 *
 *   Note:
 *      - Content of ptr is modified and string in ap point into ptr string.
 *      - in current version, only one ip is returned by ESP8266.
 *
 * Input Parameters:
 *   ptr    : +CIPDOMAIN line null terminated string pointer.
 *   ip     : ip of hostname
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_parse_cwdomain_ans_line(const char *ptr, in_addr_t *ip)
{
  int field_idx;
  char *ptr_next;

  for (field_idx = 0; field_idx <= 1; field_idx++)
    {
      if (field_idx == 0)
        {
          ptr_next = strchr(ptr, ':');
        }
      else if (field_idx == 1)
        {
          ptr_next = strchr(ptr, '\0');
        }

      if (ptr_next == NULL)
        {
          return -1;
        }

      *ptr_next = '\0';

      switch (field_idx)
        {
          case 0:
              if (strncmp(ptr, "+CIP", 4) != 0)
                {
                  return -1;
                }

              break;

          case 1:

              /* No '"' for this command ! */

              if (inet_pton(AF_INET, ptr, ip) < 0)
                {
                  return -1;
                }
              break;
        }

      ptr = ptr_next + 1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_parse_cipxxx_ans_line
 *
 * Description:
 *   Try to decode line start with:
 *      +CIPAP?
 *      +CIPAP_XXX?
 *      +CIPSTA?
 *      +CIPSTA_XXX?
 *   see in:
 *   https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/
 *   or
 *   http://www.espressif.com/sites/default/files/documentation/
 *                                   4a-esp8266_at_instruction_set_en.pdf
 *
 *                net ip       net mask
 *    +CIPxxxx:ip:"192.168.1.1","255.255.255.0","192.168.1.1",
 *
 *   Note:
 *      - Content of ptr is modified and string in ap point into ptr string.
 *      - in current version, only net ip is returned by ESP8266.
 *
 * Input Parameters:
 *   ptr   : +CWLAP line null terminated string pointer.
 *   ap    : ap result of parsing.
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_parse_cipxxx_ans_line(const char *ptr, in_addr_t *ip)
{
  int field_idx;
  char *ptr_next;

  for (field_idx = 0; field_idx <= 2; field_idx++)
    {
      if (field_idx <= 1)
        {
          ptr_next = strchr(ptr, ':');
        }
      else if (field_idx == 2)
        {
          ptr_next = strchr(ptr, '\0');
        }
      else
        {
          ptr_next = strchr(ptr, ',');
        }

      if (ptr_next == NULL)
        {
          return -1;
        }

      *ptr_next = '\0';

      switch (field_idx)
        {
          case 0:
              if (strncmp(ptr, "+CIP", 4) != 0)
                {
                  return -1;
                }

              break;

          case 1:

              /* ip label */

              break;
          case 2:
              ptr++; /* Remove first '"' */
              *(ptr_next - 1) = '\0';
              if (inet_pton(AF_INET, ptr, ip) < 0)
                {
                  return -1;
                }
              break;
        }

      ptr = ptr_next + 1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_parse_cwjap_ans_line
 *
 * Description:
 *   Try to decode @b +CWJAP? or +CWJAP_XXX? line.
 *   see in:
 *   https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/
 *   or
 *   http://www.espressif.com/sites/default/files/documentation/
 *                                4a-esp8266_at_instruction_set_en.pdf
 *
 *             SSID        BSSID            ch  RSSI
 *    +CWLAP:"FreeWifi","00:07:cb:07:b6:00", 1, -90
 *    or
 *    +CWJAP_CUR:"WLACTAP","f4:f2:6d:e8:5e:74",6,-59
 *
 *   Note: Content of ptr is modified and string in ap point into ptr string.
 *
 * Input Parameters:
 *   ptr   : +CWLAP line null terminated string pointer.
 *   ap    : ap result of parsing.
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_parse_cwjap_ans_line(char *ptr, lesp_ap_t *ap)
{
  int field_idx;
  char *ptr_next;

  for (field_idx = 0; field_idx <= 4; field_idx++)
    {
      if (field_idx == 0)
        {
          ptr_next = strchr(ptr, ':');
        }
      else if (field_idx == 4)
        {
          ptr_next = strchr(ptr, '\0');
        }
      else
        {
          ptr_next = strchr(ptr, ',');
        }

      if (ptr_next == NULL)
        {
          return -1;
        }

      *ptr_next = '\0';

      switch (field_idx)
        {
          case 0:
              if (strncmp(ptr, "+CWJAP", 6) != 0)
                {
                  return -1;
                }
              break;

          case 1:
              ptr++; /* Remove first '"' */
              *(ptr_next - 1) = '\0';
              strncpy(ap->ssid, ptr, LESP_SSID_SIZE);
              ap->ssid[LESP_SSID_SIZE] = '\0';
              break;

          case 2:
                {
                  int i;

                  ptr++; /* Remove first '"' */
                  *(ptr_next - 1) = '\0';

                  for (i = 0; i < LESP_BSSID_SIZE ; i++)
                    {
                      ap->bssid[i] = strtol(ptr, &ptr, 16);
                      if (*ptr == ':')
                        {
                          ptr++;
                        }
                    }
                }
              break;

          case 3:
                {
                  int i = atoi(ptr);
                  ap->channel = i;
                }
              break;

          case 4:
                {
                  int i = atoi(ptr);

                  if (i > 0)
                    {
                      i = -i;
                    }

                  ap->rssi = i;
                }
              break;
        }

      ptr = ptr_next + 1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_parse_cwlap_ans_line
 *
 * Description:
 *   Try to decode @b +CWLAP line.
 *   see in:
 *   https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/
 *
 *    +CWLAP:(0,"FreeWifi",-90,"00:07:cb:07:b6:00",1)
 *    0 => security
 *    "FreeWifi" => ssid
 *    -90 => rssi
 *    "00:07:cb:07:b6:00" => mac
 *
 *   Note: Content of ptr is modified and string in ap point into ptr string.
 *
 * Input Parameters:
 *   ptr   : +CWLAP line null terminated string pointer.
 *   ap    : ap result of parsing.
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_parse_cwlap_ans_line(char *ptr, lesp_ap_t *ap)
{
  int field_idx;
  char *ptr_next;

  for (field_idx = 0; field_idx <= 5; field_idx++)
    {
      if (field_idx == 0)
        {
          ptr_next = strchr(ptr, '(');
        }
      else if (field_idx == 5)
        {
          ptr_next = strchr(ptr, ')');
        }
      else
        {
          ptr_next = strchr(ptr, ',');
        }

      if (ptr_next == NULL)
        {
          return -1;
        }

      *ptr_next = '\0';

      switch (field_idx)
        {
          case 0:
              if (strcmp(ptr, "+CWLAP:") != 0)
                {
                  return -1;
                }
              break;

          case 1:
                {
                  int i = *ptr - '0';

                  if ((i < 0) || (i >= LESP_SECURITY_NBR))
                    {
                      return -1;
                    }

                  ap->security = i;
                }
              break;

          case 2:
              ptr++; /* Remove first '"' */
              *(ptr_next - 1) = '\0';
              strncpy(ap->ssid, ptr, LESP_SSID_SIZE);
              ap->ssid[LESP_SSID_SIZE] = '\0';
              break;

          case 3:
                {
                  int i = atoi(ptr);

                  if (i > 0)
                    {
                      i = -i;
                    }

                  ap->rssi = i;
                }
              break;

          case 4:
                {
                  int i;

                  ptr++; /* Remove first '"' */
                  *(ptr_next - 1) = '\0';

                  for (i = 0; i < LESP_BSSID_SIZE ; i++)
                    {
                      ap->bssid[i] = strtol(ptr, &ptr, 16);
                      if (*ptr == ':')
                        {
                          ptr++;
                        }
                    }
                }
              break;
        }

      ptr = ptr_next + 1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_worker
 *
 * Description:
 *      Esp8266 worker thread.
 *
 * Input Parameters:
 *   args  : unused
 *
 * Returned Value:
 *   NULL
 *
 ****************************************************************************/

static void *lesp_worker(void *args)
{
  int ret = 0;
  int rxlen = 0;

  lesp_worker_t *worker = &g_lesp_state.worker;

  UNUSED(args);

  ninfo("worker Started\n");

  while (worker->running)
    {
      uint8_t c;

      ret = lesp_low_level_read(&c, 1);

      if (ret < 0)
        {
          nerr("ERROR: worker read data Error %d\n", ret);
        }
      else if (ret > 0)
        {
          /* ninfo("c:0x%02X (%c)\n", c); */

          pthread_mutex_lock(&(worker->mutex));
          if (c == '\n')
            {
              if (worker->rxbuf[rxlen - 1] == '\r')
                {
                  rxlen--;
                }

              DEBUGASSERT(rxlen >= 0);
              DEBUGASSERT(rxlen < BUF_WORKER_LEN);

              worker->rxbuf[rxlen] = '\0';

              if (rxlen != 0)
                {
                  if (strcmp(worker->rxbuf, "OK") == 0)
                    {
                      worker->and = LESP_OK;
                    }
                  else if ((strcmp(worker->rxbuf, "FAIL") == 0) ||
                           (strcmp(worker->rxbuf, "ERROR") == 0)
                          )
                    {
                      worker->and = LESP_ERR;
                    }
                  else if ((rxlen == 8) &&
                            (memcmp(worker->rxbuf + 1, ",CLOSED", 7) == 0))
                    {
                      unsigned int sockid = worker->rxbuf[0] - '0';
                      if (sockid < SOCKET_NBR)
                        {
                          set_sock_closed(sockid);
                        }
                    }
                  else
                    {
                      if (worker->buf[0] != '\0')
                        {
                          pthread_mutex_unlock(&(worker->mutex));
                          usleep(100); /* leave time of aplicative to read buffer */
                          pthread_mutex_lock(&(worker->mutex));
                        }

                      /* ninfo("Worker Read data:%s\n", worker->rxbuf); */

                      if (rxlen + 1 <= BUF_ANS_LEN)
                        {
                          memcpy(worker->buf, worker->rxbuf, rxlen + 1);
                        }
                      else
                        {
                          nerr("Worker and line is too long:%s\n",
                               worker->rxbuf);
                        }
                    }

                  sem_post(&worker->sem);
                  worker->rxbuf[0] = '\0';
                  rxlen = 0;
                }
            }
          else if (rxlen < BUF_WORKER_LEN - 1)
            {
              worker->rxbuf[rxlen++] = c;
              if ((c == ':') && (memcmp(worker->rxbuf, "+IPD,", 5) == 0))
                {
                  int sockfd;
                  int len;
                  char *ptr = worker->rxbuf + 5;

                  sockfd = lesp_str_to_unsigned(&ptr, ',');
                  if (sockfd >= 0)
                    {
                      len = lesp_str_to_unsigned(&ptr, ':');
                      if (len >= 0)
                        {
                          lesp_read_ipd(sockfd, len);
                        }
                    }

                  rxlen = 0;
                }
            }
          else
            {
              nerr("Read char overflow:%c\n", c);
            }

          pthread_mutex_unlock(&(worker->mutex));
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: lesp_create_worker
 *
 * Description:
 *      start Esp8266 worker thread.
 *
 * Input Parameters:
 *   priority  : POSIX priority of worker thread.
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

static inline int lesp_create_worker(int priority)
{
  int ret = 0;

  /* Thread */

  pthread_attr_t  thread_attr;

  /* Ihm priority lower than normal */

  struct sched_param param;

  ret = pthread_attr_init(&thread_attr);

  if (ret < 0)
    {
      nerr("ERROR: Cannot Set scheduler parameter thread (%d)\n", ret);
    }
  else
    {
      ret = pthread_attr_getschedparam(&thread_attr, &param);
      if (ret >= 0)
        {
          param.sched_priority += priority;
          ret = pthread_attr_setschedparam(&thread_attr, &param);
        }
      else
        {
          nerr("ERROR: "
               "Cannot Get/Set scheduler parameter thread (%d)\n", ret);
        }

      g_lesp_state.worker.running = true;

      ret = pthread_create(&g_lesp_state.worker.thread,
                           (ret < 0)?NULL:&thread_attr, lesp_worker, NULL);
      if (ret < 0)
        {
          nerr("ERROR: Cannot Create thread return (%d)\n", ret);
          g_lesp_state.worker.running = false;
        }

#if CONFIG_TASK_NAME_SIZE > 0
      if (ret >= 0)
        {
          pthread_setname_np(g_lesp_state.worker.thread, "ESP8266");
        }
#endif

      if (pthread_attr_destroy(&thread_attr) < 0)
        {
          nerr("ERROR: Cannot destroy thread attribute (%d)\n", ret);
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lesp_initialize
 *
 * Description:
 *   initialise Esp8266 class.
 *      - open port
 *      - configure port
 *      - etc...
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

int lesp_initialize(void)
{
  int ret = 0;

  pthread_mutex_lock(&g_lesp_state.mutex);

  if (g_lesp_state.is_initialized)
    {
      pthread_mutex_unlock(&g_lesp_state.mutex);
      ninfo("Esp8266 already initialized\n");
      return 0;
    }

  pthread_mutex_lock(&g_lesp_state.worker.mutex);

  ninfo("Initializing Esp8266...\n");

  memset(g_lesp_state.sockets, 0, SOCKET_NBR * sizeof(lesp_socket_t));

  if (sem_init(&g_lesp_state.worker.sem, 0, 0) < 0)
    {
      ninfo("Cannot create semaphore\n");
      ret = -1;
    }

  if (ret >= 0 && g_lesp_state.fd < 0)
    {
      g_lesp_state.fd = open(CONFIG_NETUTILS_ESP8266_DEV_PATH, O_RDWR);
    }

  if (ret >= 0 && g_lesp_state.fd < 0)
    {
      nerr("ERROR: Cannot open %s\n", CONFIG_NETUTILS_ESP8266_DEV_PATH);
      ret = -1;
    }

#ifdef CONFIG_SERIAL_TERMIOS
  if (ret >= 0 && lesp_set_baudrate(CONFIG_NETUTILS_ESP8266_BAUDRATE) < 0)
    {
      nerr("ERROR: Cannot set baud rate %d\n",
            CONFIG_NETUTILS_ESP8266_BAUDRATE);
      ret = -1;
    }
#endif

  if ((ret >= 0) && (g_lesp_state.worker.running == false))
    {
      ret = lesp_create_worker(CONFIG_NETUTILS_ESP8266_THREADPRIO);
    }

  if (ret < 0)
    {
      ninfo("Esp8266 initialisation failed!\n");
      ret = -1;
    }
  else
    {
      g_lesp_state.is_initialized = true;
      ninfo("Esp8266 initialized\n");
    }

  pthread_mutex_unlock(&g_lesp_state.worker.mutex);
  pthread_mutex_unlock(&g_lesp_state.mutex);

  return 0;
}

/****************************************************************************
 * Name: lesp_soft_reset
 *
 * Description:
 *    reset esp8266 (command "AT+RST");
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

int lesp_soft_reset(void)
{
  int ret = 0;
  int i;

  pthread_mutex_lock(&g_lesp_state.mutex);

  /* Rry to close opened reset */

  pthread_mutex_lock(&g_lesp_state.worker.mutex);

  for (i = 0; i < SOCKET_NBR; i++)
    {
      if ((g_lesp_state.sockets[i].flags & FLAGS_SOCK_USED) != 0)
        {
          set_sock_closed(i);
        }
    }

  pthread_mutex_unlock(&g_lesp_state.worker.mutex);

  /* Leave time to close socket */

  sleep(1);

  /* Send reset */

  lesp_send_cmd("AT+RST\r\n");

  /* Leave time to reset */

  sleep(1);

  lesp_flush();

  while (lesp_ask_ans_ok(LESP_TIMEOUT_MS, "ATE0\r\n") < 0)
    {
      sleep(1);
      lesp_flush();
    }

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT+GMR\r\n");
    }

  /* Enable the module to act as a “Station” */

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT+CWMODE_CUR=1\r\n");
    }

  /* Enable the multi connection */

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT+CIPMUX=1\r\n");
    }

  if (ret < 0)
    {
      ret = -1;
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  return 0;
}

/****************************************************************************
 * Name: lesp_soft_reset
 *
 * Description:
 *    reset esp8266 (command "AT+RST");
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

int lesp_ap_connect(const char *ssid_name, const char *ap_key, int timeout_s)
{
  int ret = 0;

  ninfo("Starting manual connect...\n");

  pthread_mutex_lock(&g_lesp_state.mutex);

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(timeout_s * 1000,
                            "AT+CWJAP=\"%s\",\"%s\"\r\n",
                            ssid_name, ap_key);
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      return -1;
    }

  ninfo("Wifi connected\n");
  return 0;
}

/****************************************************************************
 * Name: lesp_ap_get
 *
 * Description:
 *    Read the current connected Access Point.
 *
 * Input Parameters:
 *   None
 *
 * Output Parameters:
 *   ap : details of connected access point
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

int lesp_ap_get(lesp_ap_t *ap)
{
  int ret = 0;
  ninfo("Get Access Point info...\n");

  pthread_mutex_lock(&g_lesp_state.mutex);

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_send_cmd("AT+CWJAP_CUR?\r\n");
    }

  if (ret >= 0)
    {
      ret = lesp_read(LESP_TIMEOUT_MS);
    }

  if (ret >= 0)
    {
      ninfo("Read:%s\n", g_lesp_state.bufans);
      ret = lesp_parse_cwjap_ans_line(g_lesp_state.bufans, ap);
      if (ret < 0)
        {
          nerr("ERROR: Line badly formed.\n");
        }
    }

  if (ret >= 0)
    {
      ret = lesp_read_ans_ok(LESP_TIMEOUT_MS);
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: Get access point information.\n");
      return -1;
    }

  ninfo("Connected to %s\n", ap->ssid);
  return 0;
}

/****************************************************************************
 * Name: lesp_get_net
 *
 * Description:
 *    Read the current network details.
 *
 * Input Parameters:
 *   mode : LESP_MODE_AP or LESP_MODE_STATION
 *
 * Output Parameters:
 *   ip     : ip of interface
 *   mask   : mask of interface (not implemented by esp8266)
 *   gw     : gateway of interface (not implemented by esp8266)
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

int lesp_get_net(lesp_mode_t mode, in_addr_t *ip,
                 in_addr_t *mask, in_addr_t *gw)
{
  int ret = 0;
  ninfo("Get IP info...\n");

  pthread_mutex_lock(&g_lesp_state.mutex);

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_send_cmd("AT+CIP%s_CUR?\r\n",
                          (mode == LESP_MODE_STATION) ? "STA" : "AP");
    }

  if (ret >= 0)
    {
      ret = lesp_read(LESP_TIMEOUT_MS);
    }

  if (ret >= 0)
    {
      ninfo("Read:%s\n", g_lesp_state.bufans);

      ret = lesp_parse_cipxxx_ans_line(g_lesp_state.bufans, ip);
      if (ret < 0)
        {
          nerr("ERROR: Line badly formed.\n");
        }
    }

  if (ret >= 0)
    {
      ret = lesp_read(LESP_TIMEOUT_MS);
    }

  if (ret >= 0)
    {
      ninfo("Read:%s\n", g_lesp_state.bufans);

      ret = lesp_parse_cipxxx_ans_line(g_lesp_state.bufans, mask);
      if (ret < 0)
        {
          nerr("ERROR: Line badly formed.\n");
        }
    }

  if (ret >= 0)
    {
      ret = lesp_read(LESP_TIMEOUT_MS);
    }

  if (ret >= 0)
    {
      ninfo("Read:%s\n", g_lesp_state.bufans);

      ret = lesp_parse_cipxxx_ans_line(g_lesp_state.bufans, mask);
      if (ret < 0)
        {
          nerr("ERROR: Line badly formed.\n");
        }
    }

  if (ret >= 0)
    {
      ret = lesp_read_ans_ok(LESP_TIMEOUT_MS);
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: Get network information.\n");
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_set_net
 *
 * Description:
 *   It will set network ip of mode.
 *   Warning: use LESP_MODE_STATION or LESP_MODE_AP.
 *
 * Input Parameters:
 *   mode    : mode to configure.
 *   ip      : ip of interface.
 *   mask    : network mask of interface.
 *   gateway : gateway ip of network.
 *
 * Returned Value:
 *   0 on success, -1 on error.
 *
 ****************************************************************************/

int lesp_set_net(lesp_mode_t mode, in_addr_t ip,
                 in_addr_t mask, in_addr_t gateway)
{
  int ret = 0;

  pthread_mutex_lock(&g_lesp_state.mutex);

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT+CIP%s_CUR=\"%d.%d.%d.%d\","
                            "\"%d.%d.%d.%d\",\"%d.%d.%d.%d\"\r\n",
                            (mode == LESP_MODE_STATION) ? "STA" : "AP",
                            *((uint8_t *)&(ip)+0), *((uint8_t *)&(ip)+1),
                            *((uint8_t *)&(ip)+2), *((uint8_t *)&(ip)+3),
                            *((uint8_t *)&(gateway)+0),
                            *((uint8_t *)&(gateway)+1),
                            *((uint8_t *)&(gateway)+2),
                            *((uint8_t *)&(gateway)+3),
                            *((uint8_t *)&(mask)+0),
                            *((uint8_t *)&(mask)+1),
                            *((uint8_t *)&(mask)+2),
                            *((uint8_t *)&(mask)+3));
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_set_dhcp
 *
 * Description:
 *   It will Enable or disable DHCP of mode.
 *
 * Input Parameters:
 *   mode    : mode to configure.
 *   enable  : true for enable, false for disable.
 *
 * Returned Value:
 *   0 on success, -1 on error.
 *
 ****************************************************************************/

int lesp_set_dhcp(lesp_mode_t mode, bool enable)
{
  int ret = 0;

  pthread_mutex_lock(&g_lesp_state.mutex);

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT+CWDHCP_CUR=%d,%c\r\n",
                            mode, enable ? '1' : '0');
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: lesp_get_dhcp
 *
 * Description:
 *   It will get if DHCP is Enable or disable for each mode.
 *
 * Input Parameters:
 *   ap_enable   : true DHCP is enable in Access Point mode, false otherwise.
 *   sta_enable  : true DHCP is enable in Station mode, false otherwise.
 *
 * Returned Value:
 *   0 on success, -1 on error.
 *
 ****************************************************************************/

int lesp_get_dhcp(bool *ap_enable, bool *sta_enable)
{
  int ret = 0;

  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("Get DHCP State...\n");

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_send_cmd("AT+CWDHCP_CUR?\r\n");
    }

  if (ret >= 0)
    {
      ret = lesp_read(LESP_TIMEOUT_MS);
    }

  if (ret >= 0)
    {
      char *ptr = g_lesp_state.bufans;

      ninfo("Read:%s\n", ptr);

      ptr = strchr(ptr, ':');
      ptr++;
      switch (*ptr - '0')
        {
          case 0:
              *ap_enable = 0;
              *sta_enable = 0;
              break;
          case 1:
              *ap_enable = 1;
              *sta_enable = 0;
              break;
          case 2:
              *ap_enable = 0;
              *sta_enable = 1;
              break;
          case 3:
              *ap_enable = 1;
              *sta_enable = 1;
              break;
          default:
              nerr("ERROR: Line badly formed.\n");
              break;
        }
    }

  if (ret >= 0)
    {
      ret = lesp_read_ans_ok(LESP_TIMEOUT_MS);
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: Get DHCP.\n");
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name:  lesp_list_access_points
 *
 * Description:
 *   Search all access points.
 *
 * Input Parameters:
 *   cb  : call back call for each access point found.
 *
 * Returned Value:
 *   Number access point(s) found on success, -1 on error.
 *
 ****************************************************************************/

int lesp_list_access_points(lesp_cb_t cb)
{
  lesp_ap_t ap;
  int ret = 0;
  int number = 0;

  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("List access point(s)...\n");

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT\r\n");
    }

  if (ret >= 0)
    {
      ret = lesp_send_cmd("AT+CWLAP\r\n");
    }

  while (ret >= 0)
    {
      ret = lesp_read(LESP_TIMEOUT_MS_LISP_AP);
      if (ret < 0)
        {
          continue;
        }

      ninfo("Read:%s\n", g_lesp_state.bufans);

      if (strcmp(g_lesp_state.bufans, "OK") == 0)
        {
          break;
        }

      ret = lesp_parse_cwlap_ans_line(g_lesp_state.bufans, &ap);
      if (ret < 0)
        {
          nerr("ERROR: Line badly formed.");
        }

      cb(&ap);
      number++;
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: list access points.");
      return -1;
    }

  ninfo("Access Point list finished with %d ap founds\n", number);

  return number;
}

/****************************************************************************
 * Name:  lesp_security_to_str
 *
 * Description:
 *   return corresponding string of security enum.
 *
 * Input Parameters:
 *   security : enum value of string
 *
 * Returned Value:
 *   String corresponding to security value.
 *
 ****************************************************************************/

const char *lesp_security_to_str(lesp_security_t security)
{
  switch (security)
    {
      case LESP_SECURITY_NONE:
          return "NONE";
      case LESP_SECURITY_WEP:
          return "WEP";
      case LESP_SECURITY_WPA_PSK:
          return "WPA_PSK";
      case LESP_SECURITY_WPA2_PSK:
          return "WPA2_PSK";
      case LESP_SECURITY_WPA_WPA2_PSK:
          return "WPA_WPA2_PSK";
      default:
          return "Unknown";
    }
}

/****************************************************************************
 * Name:  lesp_socket
 *
 * Description:
 *   Equivalent of POSIX socket for esp8266.
 *   socket() creates an endpoint for communication and returns a descriptor.
 *
 * Input Parameters:
 *   domain   : only PF_INET is supported
 *   type     : only SOCK_STREAM is supported
 *   protocol : only IPPROTO_TCP is supported
 *
 * Returned Value:
 *   A non-negative socket descriptor on success; -1 on error.
 *
 ****************************************************************************/

int lesp_socket(int domain, int type, int protocol)
{
  int ret = 0;
  int i = CON_NBR;
  int flags = 0;

  if (domain != PF_INET)
    {
      nerr("ERROR: only PF_INET Implemented!\n");
      return -1;
    }

  switch (type)
    {
      case SOCK_STREAM:
          flags |= FLAGS_SOCK_TYPE_TCP;
          break;
      case SOCK_DGRAM:
          flags |= FLAGS_SOCK_TYPE_UDP;
          break;
      case -1:
          flags |= FLAGS_SOCK_TYPE_SSL;
          break;
      default:
          nerr("ERROR: Only SOCK_DGRAM and SOCK_STREAM Implemented!\n");
          errno = ESOCKTNOSUPPORT;
          return -1;
    }

  flags |= FLAGS_SOCK_USED;

  pthread_mutex_lock(&g_lesp_state.worker.mutex);

  ret = -1;
  if (!g_lesp_state.is_initialized)
    {
      ninfo("Esp8266 not initialized; can't list access points\n");
      errno = ENETDOWN;
    }
  else
    {
      for (i = 0; i < CON_NBR; i++)
        {
          if ((g_lesp_state.sockets[i].flags & FLAGS_SOCK_USED) == 0)
            {
              g_lesp_state.sockets[i].flags = flags;
              g_lesp_state.sockets[i].rcv_timeo.tv_sec =
                                              LESP_TIMEOUT_MS_RECV_S;
              g_lesp_state.sockets[i].rcv_timeo.tv_nsec = 0;
              ret = i;
              break;
            }
        }

      if (ret < 0)
        {
          errno = EAGAIN;
        }
    }

  pthread_mutex_unlock(&g_lesp_state.worker.mutex);

  return ret;
}

/****************************************************************************
 * Name:  lesp_closesocket
 *
 * Description:
 *   Equivalent of POSIX close for esp8266.
 *   close socket creates with lesp_socket.
 *
 * Input Parameters:
 *   sockfd   : socket indentifer.
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

int lesp_closesocket(int sockfd)
{
  int ret = 0;
  lesp_socket_t *sock = NULL;

  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("List access point(s)...\n");

  ret = lesp_check();

  if (ret >= 0)
    {
      sock = get_sock_protected(sockfd);
      if (sock == NULL)
        {
          ret = -1;
        }
    }

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT+CIPCLOSE=%d\r\n", sockfd);

      pthread_mutex_lock(&g_lesp_state.worker.mutex);
      set_sock_closed(sockfd);
      pthread_mutex_unlock(&g_lesp_state.worker.mutex);
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: Close socket %d.\n", sockfd);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name:  lesp_bind
 *
 * Description:
 *   Equivalent of POSIX bind for esp8266.
 *
 * Input Parameters:
 *   sockfd   Socket descriptor of the socket to bind
 *   addr     Socket local address
 *   addrlen  Length of 'addr'
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

int lesp_bind(int sockfd, FAR const struct sockaddr *addr, socklen_t addrlen)
{
  int ret = 0;
  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("Bind socket %d...\n", sockfd);

  ret = lesp_check();

  if (ret >= 0)
    {
      nerr("ERROR: Not implemented %s\n", __func__);
      errno = EIO;
      ret = -1;
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  return ret;
}

/****************************************************************************
 * Name:  lesp_connect
 *
 * Description:
 *   Equivalent of POSIX connect for esp8266.
 *
 * Input Parameters:
 *   sockfd    Socket descriptor returned by socket()
 *   addr      Server address (form depends on type of socket)
 *   addrlen   Length of actual 'addr'
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

int lesp_connect(int sockfd, FAR const struct sockaddr *addr,
                 socklen_t addrlen)
{
  int ret = 0;
  const char *proto_str;
  lesp_socket_t *sock;
  struct sockaddr_in *in;
  unsigned short port;
  in_addr_t ip;

  in = (struct sockaddr_in *)addr;
  port = ntohs(in->sin_port);     /* e.g. htons(3490) */
  ip = in->sin_addr.s_addr;

  DEBUGASSERT(in->sin_family == AF_INET);
  DEBUGASSERT(addrlen == sizeof(struct sockaddr_in));

  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("Connect %d...\n", sockfd);

  ret = lesp_check();

  if (ret >= 0)
    {
      pthread_mutex_lock(&g_lesp_state.worker.mutex);
      sock = get_sock(sockfd);
      if (sock == NULL)
        {
          ret = -1;
        }
    }

  if (ret >= 0)
    {
      switch (FLAGS_SOCK_TYPE_MASK & sock->flags)
        {
          case FLAGS_SOCK_TYPE_TCP:
              proto_str = "TCP";
              break;
          case FLAGS_SOCK_TYPE_UDP:
              proto_str = "UDP";
              break;
          case FLAGS_SOCK_TYPE_SSL:
              proto_str = "SSL";
              break;
          default:
              errno = ESOCKTNOSUPPORT;
              ret = -1;
        }
    }

  pthread_mutex_unlock(&g_lesp_state.worker.mutex);

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS, "AT+CIPSTART=%d,\"%s\","
                            "\"%d.%d.%d.%d\",%d\r\n", sockfd, proto_str,
                            *((uint8_t *)&ip + 0), *((uint8_t *)&ip +1),
                            *((uint8_t *)&ip + 2), *((uint8_t *)&ip + 3),
                            port);
      if (ret < 0)
        {
          errno = EIO;
        }
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: Connect socket %d.\n", sockfd);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name:  lesp_listen
 *
 * Description:
 *   Equivalent of POSIX listen for esp8266.
 *
 * Input Parameters:
 *   sockfd    Socket descriptor returned by socket()
 *   backlog  The maximum length the queue of pending connections may grow.
 *            If a connection request arrives with the queue full, the client
 *            may receive an error with an indication of ECONNREFUSED or,
 *            if the underlying protocol supports retransmission, the request
 *            may be ignored so that retries succeed.
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

int lesp_listen(int sockfd, int backlog)
{
  int ret = 0;
  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("Connect %d...\n", sockfd);

  ret = lesp_check();

  if (ret >= 0)
    {
      nerr("ERROR: Not implemented %s\n", __func__);
      errno = EIO;
      ret = -1;
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  return ret;
}

/****************************************************************************
 * Name:  lesp_accept
 *
 * Description:
 *   Equivalent of POSIX accept for esp8266.
 *
 * Input Parameters:
 *   sockfd    Socket descriptor returned by socket()
 *   addr     Receives the address of the connecting client
 *   addrlen  Input: allocated size of 'addr',
 *            Return: returned size of 'addr'
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

int lesp_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  int ret = 0;
  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("Connect %d...\n", sockfd);

  ret = lesp_check();

  if (ret >= 0)
    {
      nerr("ERROR: Not implemented %s\n", __func__);
      errno = EIO;
      ret = -1;
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  return ret;
}

/****************************************************************************
 * Name:  lesp_send
 *
 * Description:
 *   Equivalent of POSIX send for esp8266.
 *
 * Input Parameters:
 *   sockfd    Socket descriptor returned by socket()
 *   buf      Data to send
 *   len      Length of data to send
 *   flags    Send flags
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

ssize_t lesp_send(int sockfd, FAR const uint8_t *buf, size_t len, int flags)
{
  int ret = 0;
  lesp_socket_t *sock = NULL;

  UNUSED(flags);

  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("Send %d bytes in %d socket...\n", len, sockfd);

  ret = lesp_check();

  if (ret >= 0)
    {
      sock = get_sock_protected(sockfd);
      if (sock == NULL)
        {
          ret = -1;
        }
    }

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(LESP_TIMEOUT_MS,
                            "AT+CIPSEND=%d,%d\r\n", sockfd, len);
    }

  if (ret >= 0)
    {
      ninfo("Sending in socket %d, %d bytes\n", sockfd, len);
      ret = write(g_lesp_state.fd, buf, len);
    }

  while (ret >= 0)
    {
      char * ptr = g_lesp_state.bufans;
      ret = lesp_read(LESP_TIMEOUT_MS);

      if (ret < 0)
        {
          break;
        }

      while ((*ptr != 0) && (*ptr != 'S'))
        {
          ptr++;
        }

      if (*ptr == 'S')
        {
          if (strcmp(ptr, "SEND OK") == 0)
              break;
        }
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: Cannot send in socket %d, %d bytes\n", sockfd, len);
      return -1;
    }

  ninfo("Sent\n");

  return len;
}

/****************************************************************************
 * Name:  lesp_recv
 *
 * Description:
 *   Equivalent of POSIX recv for esp8266.
 *
 * Input Parameters:
 *   sockfd   Socket descriptor returned by socket()
 *   buf      Data to receive
 *   len      Length of data to receive
 *   flags    Send flags
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

ssize_t lesp_recv(int sockfd, FAR uint8_t *buf, size_t len, int flags)
{
  int ret = 0;
  lesp_socket_t *sock;
  sem_t sem;

  if (sem_init(&sem, 0, 0) < 0)
    {
      ninfo("Cannot create semaphore\n");
      return -1;
    }

  pthread_mutex_lock(&g_lesp_state.worker.mutex);

  UNUSED(flags);

  sock = get_sock(sockfd);
  if (sock == NULL)
    {
      ret = -1;
    }

  if (ret >= 0 && sock->inndx == sock->outndx)
    {
      struct timespec ts;

      if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
        {
          ret = -1;
        }
      else
        {
          ts.tv_sec  += sock->rcv_timeo.tv_sec;
          ts.tv_nsec += sock->rcv_timeo.tv_nsec;
          if (ts.tv_nsec >= 1000000000)
            {
              ts.tv_sec++;
              ts.tv_nsec -= 1000000000;
            }

          sock->sem = &sem;

          while (ret >= 0 && sock->inndx == sock->outndx)
            {
              pthread_mutex_unlock(&g_lesp_state.worker.mutex);
              ret = sem_timedwait(&sem, &ts);
              pthread_mutex_lock(&g_lesp_state.worker.mutex);
            }

          sock->sem = NULL;
        }
    }

  if (ret >= 0)
    {
      ret = 0;
      while (ret < len && sock->outndx != sock->inndx)
        {
          /* Remove one byte from the circular buffer */

          int ndx = sock->outndx;
          *buf++  = sock->rxbuf[ndx];

          /* Increment the circular buffer 'outndx' */

          if (++ndx >= SOCKET_FIFO_SIZE)
            {
              ndx -= SOCKET_FIFO_SIZE;
            }

          sock->outndx = ndx;

          /* Increment the count of bytes returned */

          ret++;
        }
    }

  pthread_mutex_unlock(&g_lesp_state.worker.mutex);

  if (ret < 0)
    {
      return -1;
    }

  return ret;
}

/****************************************************************************
 * Name:  lesp_setsockopt
 *
 * Description:
 *   Equivalent of POSIX setsockopt for esp8266.
 *
 * Input Parameters:
 *   sockfd   Socket descriptor returned by socket()
 *   level     Protocol level to set the option
 *   option    identifies the option to set
 *   value     Points to the argument value
 *   value_len The length of the argument value
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

int lesp_setsockopt(int sockfd, int level, int option,
                    FAR const void *value, socklen_t value_len)
{
  int ret = 0;
  lesp_socket_t *sock;

  if (level != SOL_SOCKET)
    {
      nerr("ERROR: %s:Not implemented level:%d\n", __func__, level);
      errno = EINVAL;
      return -1;
    }

  pthread_mutex_lock(&g_lesp_state.worker.mutex);

  sock = get_sock(sockfd);
  if (sock == NULL)
    {
      ret = -1;
    }
  else
    {
      switch (option)
        {
          case SO_RCVTIMEO:
              if (value_len == sizeof(struct timeval))
                {
                  sock->rcv_timeo.tv_sec = ((struct timeval *)
                                             (value))->tv_sec;
                  sock->rcv_timeo.tv_nsec = ((struct timeval *)
                                             (value))->tv_usec;
                  sock->rcv_timeo.tv_nsec *= 1000; /* tv_usec to tv_nsec */
                }
              else
                {
                  nerr("ERROR: Wrong size:%d\n", level);
                  errno = EINVAL;
                  ret = -1;
                }
              break;

          default:
              nerr("ERROR: Not implemented %s\n", __func__);
              errno = EINVAL;
              ret = -1;
              break;
        }
    }

  pthread_mutex_unlock(&g_lesp_state.worker.mutex);

  return ret;
}

/****************************************************************************
 * Name:  lesp_getsockopt
 *
 * Description:
 *   Equivalent of POSIX getsockopt for esp8266.
 *
 * Input Parameters:
 *   sockfd   Socket descriptor returned by socket()
 *   level     Protocol level to set the option
 *   option    identifies the option to set
 *   value     Points to the argument value
 *   value_len The length of the argument value
 *
 * Returned Value:
 *   A 0 on success; -1 on error.
 *
 ****************************************************************************/

int lesp_getsockopt(int sockfd, int level, int option, FAR void *value,
                    FAR socklen_t *value_len)
{
  int ret = 0;
  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("getsockopt on %d socket...\n", sockfd);

  ret = lesp_check();

  if (ret >= 0)
    {
      nerr("ERROR: Not implemented %s\n", __func__);
      errno = EIO;
      ret = -1;
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  return ret;
}

/****************************************************************************
 * Name:  lesp_gethostbyname
 *
 * Description:
 *   Equivalent of POSIX gethostbyname for esp8266.
 *
 * Input Parameters:
 *   name - The name of the host to find.
 *
 * Returned Value:
 *   Upon successful completion, this function will return a pointer to a
 *   hostent structure if the requested entry was found, and a null pointer
 *   if the end of the database was reached or the requested entry was not
 *   found.
 *
 *   Upon unsuccessful completion, gethostbyname() return NULL.
 *
 ****************************************************************************/

FAR struct hostent *lesp_gethostbyname(FAR const char *hostname)
{
  int ret = 0;

  memset(&g_lesp_state.hostent, 0, sizeof(g_lesp_state.hostent));

  g_lesp_state.hostent.h_addr_list  = (char**)&g_lesp_state.h_addr_list_buf;
  g_lesp_state.hostent.h_addrtype   = AF_INET;
  g_lesp_state.hostent.h_length     = sizeof(struct sockaddr_in);

  g_lesp_state.h_addr_list_buf[0] = &g_lesp_state.in_addr;
  g_lesp_state.h_addr_list_buf[1] = NULL;

  pthread_mutex_lock(&g_lesp_state.mutex);

  ninfo("Get host by name '%s' ...\n", hostname);

  ret = lesp_check();

  if (ret >= 0)
    {
      ret = lesp_send_cmd("AT+CIPDOMAIN=\"%s\"\r\n", hostname);
    }

  if (ret >= 0)
    {
      ret = lesp_read(LESP_TIMEOUT_MS);
    }

  if (ret >= 0)
    {
      ninfo("Read:%s\n", g_lesp_state.bufans);

      ret = lesp_parse_cwdomain_ans_line(g_lesp_state.bufans,
                                         &g_lesp_state.in_addr);

      if (ret < 0)
        {
          nerr("ERROR: Line badly formed.\n");
          errno = EIO;
        }
    }

  if (ret >= 0)
    {
      ret = lesp_read_ans_ok(LESP_TIMEOUT_MS);
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      nerr("ERROR: Get host by name.\n");
      return NULL;
    }

  return &g_lesp_state.hostent;
}

#endif /* CONFIG_NETUTILS_ESP8266 */
