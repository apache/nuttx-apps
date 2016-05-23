/****************************************************************************
 * apps/netutils/esp8266/esp8266.c
 *
 * Derives from an application to demo an Arduino connected to the ESPRESSIF
 * ESP8266 with AT command firmware.
 *
 *   Copyright (C) 2015 Pierre-Noel Bouteville. All rights reserved.
 *   Author: Pierre-Noel Bouteville <pnb990@gmail.com>
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <poll.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>
#include "sys/socket.h"

#include "apps/netutils/esp8266.h"

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

#define BUF_CMD_LEN     CONFIG_NETUTILS_ESP8266_MAXTXLEN
#define BUF_ANS_LEN     CONFIG_NETUTILS_ESP8266_MAXRXLEN
#define BUF_WORKER_LEN  1024

#define CON_NBR 4

#define ESP8266_ACCESS_POINT_NBR_MAX 32

#define lespWAITING_OK_POLLING_MS   250   
#define lespTIMEOUT_MS              1000
#define lespTIMEOUT_MS_SEND         1000
#define lespTIMEOUT_MS_CONNECTION   30000
#define lespTIMEOUT_MS_LISP_AP      5000
#define lespTIMEOUT_FLOODING_OFFSET_S 3
#define lespTIMEOUT_MS_RECV_S       60

#define lespCON_USED_MASK(idx)      (1<<(idx))
#define lespPOLLING_TIME_MS         1000

/* Must be a power of 2 */

#define SOCKET_FIFO_SIZE            2048
#define SOCKET_NBR                  4

#define FLAGS_SOCK_USED            (1 << 0)
#define FLAGS_SOCK_CONNECTED       (1 << 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  sem_t          *sem;
  uint8_t         flags;
  uint16_t        inndx;
  uint16_t        outndx;
  uint8_t         rxbuf[SOCKET_FIFO_SIZE];
} lesp_socket_t;

typedef struct
{
  bool            running;
  pthread_t       thread;
  uint8_t         buf[BUF_WORKER_LEN];
  int             bufsize;
} lesp_worker_t;

typedef struct
{
  pthread_mutex_t mutex;
  bool            is_initialized;
  int             fd;
  lesp_worker_t   worker;
  lesp_socket_t   sockets[SOCKET_NBR];
  sem_t           sem;
  char            buf[BUF_ANS_LEN];
  char            bufans[BUF_ANS_LEN];
  char            bufcmd[BUF_CMD_LEN];
} lesp_state_t;

/****************************************************************************
 *  Private Functions prototypes
 ****************************************************************************/

static int lesp_low_level_read(uint8_t* buf,int size);
static inline int lesp_read_ipd(void);
static lesp_socket_t* get_sock(int sockfd);

/****************************************************************************
 * Private Data
 ****************************************************************************/

lesp_state_t g_lesp_state =
{
  .mutex          = PTHREAD_MUTEX_INITIALIZER,
  .is_initialized = false,
  .fd             = -1,
  .worker.running = false,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_sock
 *
 * Description:
 *   Get socket structure pointer of sockfd.
 *
 * Input Parmeters:
 *   sockfd : socket id
 *
 * Returned Value:
 *   socket id pointer, NULL on error.
 *
 ****************************************************************************/

static lesp_socket_t *get_sock(int sockfd)
{
  if (!g_lesp_state.is_initialized)
    {
      nvdbg("Esp8266 not initialized; can't list access points\n");
      return NULL;
    }

  if (((unsigned int)sockfd) >= SOCKET_NBR)
    {
      nvdbg("Esp8266 invalid sockfd\n", sockfd);
      return NULL;
    }

  if ((g_lesp_state.sockets[sockfd].flags & FLAGS_SOCK_USED) == 0)
    {
      ndbg("Connection id %d not Created!\n", sockfd);
      return NULL;
    }

  return &g_lesp_state.sockets[sockfd];
}

/****************************************************************************
 * Name: lesp_low_level_read
 *
 * Description:
 *   put size data from esp8266 into buf.
 *
 * Input Parmeters:
 *   buf  : buffer to store read data
 *   size : size of data to read
 *
 * Returned Value:
 *    number of byte read, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_low_level_read(uint8_t* buf, int size)
{
  int ret = 0;

  struct pollfd fds[1];

  memset(fds, 0, sizeof( struct pollfd));
  fds[0].fd       = g_lesp_state.fd;
  fds[0].events   = POLLIN;

  /* poll return 1=>even occur 0=>timeout or -1=>error */

  ret = poll(fds, 1, lespPOLLING_TIME_MS);
  if (ret < 0)
    {
      int err = errno;
      ndbg("worker read Error %d (errno %d)\n", ret, err);
      UNUSED(err);
    } 
  else if ((fds[0].revents & POLLERR) && (fds[0].revents & POLLHUP))
    {
      ndbg("worker poll read Error %d\n", ret);
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
 * Input Parmeters:
 *   None
 *
 * Returned Value:
 *   1 was an IPD, 0 is not an IPD, -1 on error.
 *
 ****************************************************************************/

static inline int lesp_read_ipd(void)
{
  int sockfd;
  lesp_socket_t *sock;
  char *ptr;
  int len;

  ptr = (char *)g_lesp_state.worker.buf;

  /* Put a null at end */

  *(ptr + g_lesp_state.worker.bufsize) = '\0';

  if (memcmp(ptr,"+IPD,",5) != 0)
    {
      return 0;
    }

  ptr += 5;

  sockfd = strtol(ptr, &ptr, 10);

  sock = get_sock(sockfd);

  if (sock == NULL)
    {
      return -1;
    }

  if (*ptr++ != ',')
    {
      return -1;
    }

  len = strtol(ptr,&ptr,10);

  if (*ptr != ':')
    {
      return -1;
    }

  nvdbg("Read %d bytes for socket %d \n", len, sockfd);

  while(len)
    {
      int size;
      uint8_t *buf = g_lesp_state.worker.buf;

      size = len;
      if (size >= BUF_WORKER_LEN)
        {
          size = BUF_WORKER_LEN;
        }

      size = lesp_low_level_read(buf,size);
      if (size <= 0)
        {
          return -1;
        }

      len -= size;
      pthread_mutex_lock(&g_lesp_state.mutex);
      while (size--)
        {
          int next;
          uint8_t b;

          /* Read the next byte from the buffer */

          b    = *buf++;

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

          if (next != sock->outndx)
            {
              /* Yes.. add the byte to the circular buffer */

              sock->rxbuf[sock->inndx] = b;
              sock->inndx = next;
            }
          else
            {
              /* No.. the we have lost data */

              nvdbg("overflow socket 0x%02X\n", b);
            }
        }

      if (sock->sem)
        {
          sem_post(sock->sem);
        }

      pthread_mutex_unlock(&g_lesp_state.mutex);
    } 

  return 0;
}

/****************************************************************************
 * Name: lesp_vsend_cmd
 *
 * Description:
 *   Send cmd with format and argument like sprintf.
 *
 * Input Parmeters:
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

  ret = vsnprintf(g_lesp_state.bufcmd,BUF_CMD_LEN,format,ap);
  if (ret >= BUF_CMD_LEN)
    {
      g_lesp_state.bufcmd[BUF_CMD_LEN-1]='\0';
      nvdbg("Buffer too small for '%s'...\n", g_lesp_state.bufcmd);
      ret = -1;
    }

  nvdbg("Write:%s\n", g_lesp_state.bufcmd);

  ret = write(g_lesp_state.fd,g_lesp_state.bufcmd,ret);
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
 * Input Parmeters:
 *   format : null terminated format string to send.
 *   ...    : format values.
 *
 * Returned Value:
 *   len of cmd on success, -1 on error.
 *
 ****************************************************************************/

static int lesp_send_cmd(FAR const IPTR char *format, ...)
{
  int ret = 0;
  va_list ap;

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
 * Input Parmeters:
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
      nvdbg("Esp8266 not initialized; can't list access points\n");
      return -1;
    }

  if (clock_gettime(CLOCK_REALTIME,&ts) < 0)
    {
      return -1;
    }

  ts.tv_sec += (timeout_ms/1000) + lespTIMEOUT_FLOODING_OFFSET_S;

  do 
    {
      if (sem_timedwait(&g_lesp_state.sem,&ts) < 0)
        {
          return -1;
        }

      pthread_mutex_lock(&g_lesp_state.mutex);

      ret = strlen(g_lesp_state.buf);
      if (ret > 0)
        {
          memcpy(g_lesp_state.bufans,g_lesp_state.buf,ret+1); /* +1 to copy null */
        }

      g_lesp_state.buf[0] = '\0';

      pthread_mutex_unlock(&g_lesp_state.mutex);

    }
  while (ret == 0);

  nvdbg("read %d=>%s\n", ret, g_lesp_state.bufans);

  return ret;
}

/****************************************************************************
 * Name: lesp_flush
 *
 * Description:
 *   Read and discard all waiting data in rx buffer.
 *
 * Input Parmeters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void lesp_flush(void)
{
  while (lesp_read(lespTIMEOUT_MS) >= 0);
}

/****************************************************************************
 * Name: lesp_read_ans_ok
 *
 * Description:
 *   Read up to read OK, ERROR, or FAIL.
 *
 * Input Parmeters:
 *   timeout_ms : timeout in millisecond.
 *
 * Returned Value:
 *   0 on OK, -1 on error.
 *
 ****************************************************************************/

int lesp_read_ans_ok(int timeout_ms)
{
  time_t end;
  end = time(NULL) + (timeout_ms/1000) + lespTIMEOUT_FLOODING_OFFSET_S;
  do
    {
      if (lesp_read(timeout_ms) < 0)
        {
          return -1;
        }

      /* Answers sorted in probability case */

      if (strcmp(g_lesp_state.bufans,"OK") == 0)
        {
          return 0;
        }

      if (strcmp(g_lesp_state.bufans,"FAIL") == 0)
        {
          return -1;
        }

      if (strcmp(g_lesp_state.bufans,"ERROR") == 0)
        {
          return -1;
        }

      nvdbg("Got:%s\n", g_lesp_state.bufans);

      /* Ro quit in case of message flooding */
    }
  while (time(NULL) < end);

  lesp_flush();

  return -1;
}

/****************************************************************************
 * Name: lesp_ask_ans_ok
 *
 * Description:
 *   Ask and ignore line start with '+' and except a "OK" answer.
 *
 * Input Parmeters:
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
 * Name:
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
 * Input Parmeters:
 *   ptr   : +CWLAP line null terminated string pointer.
 *   ap    : ap result of parsing.
 *
 * Returned Value:
 *   0 on success, -1 in case of error.
 *
 ****************************************************************************/

static int lesp_parse_cwlap_ans_line(char* ptr, lesp_ap_t *ap)
{
  int field_idx;
  char* ptr_next;

  for(field_idx = 0; field_idx <= 5; field_idx++)
    {
      if (field_idx == 0)
        {
          ptr_next = strchr(ptr,'(');
        }
      else if (field_idx == 5)
        {
          ptr_next = strchr(ptr,')');
        }
      else
        {
          ptr_next = strchr(ptr,',');
        }

      if (ptr_next == NULL)
        {
          return -1;
        }

      *ptr_next = '\0';

      switch (field_idx)
        {
          case 0:
            if (strcmp(ptr,"+CWLAP:") != 0)
              {
                return -1;
              }

            break;

          case 1:
            {
              int i = *ptr - '0';

              if ((i < 0) || (i >= lesp_eSECURITY_NBR))
                {
                  return -1;
                }

              ap->security = i;
            }
            break;

          case 2:
              ptr++; /* Remove first '"' */
              *(ptr_next -1 ) = '\0';
              ap->ssid = ptr;
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

              for (i = 0; i < lespBSSID_SIZE ; i++)
                {
                  ap->bssid[i] = strtol(ptr,&ptr,16);
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

static void *lesp_worker(void *args)
{
  int ret = 0;

  lesp_worker_t *p = &g_lesp_state.worker;

  UNUSED(args);

  pthread_mutex_lock(&g_lesp_state.mutex);
  nvdbg("worker Started \n");
  p->bufsize = 0;
  pthread_mutex_unlock(&g_lesp_state.mutex);

  while(p->running)
    {
      uint8_t c;

      ret = lesp_low_level_read(&c,1);

      if (ret < 0)
        {
          ndbg("worker read data Error %d\n", ret);
        } 
      else if (ret > 0)
        {
          //nvdbg("c:0x%02X (%c)\n", c);

          pthread_mutex_lock(&g_lesp_state.mutex);
          if (c == '\n')
            {
              if (p->buf[p->bufsize-1] == '\r')
                {
                  p->bufsize--;
                }

              if (p->bufsize != 0)
                {
                  p->buf[p->bufsize] = '\0';
                  memcpy(g_lesp_state.buf,p->buf,p->bufsize+1);
                  nvdbg("Read data:%s\n", g_lesp_state.buf);
                  sem_post(&g_lesp_state.sem);
                  p->bufsize = 0;
                }
            }
          else if (p->bufsize < BUF_ANS_LEN - 1)
            {
              p->buf[p->bufsize++] = c;
            }
          else
            {
              nvdbg("Read char overflow:%c\n", c);
            }

          pthread_mutex_unlock(&g_lesp_state.mutex);

          if ((c == ':') && (lesp_read_ipd() != 0))
            {
              p->bufsize = 0;
            }
        }
    }

  return NULL;
}

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
      ndbg("Cannot Set scheduler parameter thread (%d)\n", ret);
    }
  else
    {
      ret = pthread_attr_getschedparam(&thread_attr,&param);
      if (ret >= 0)
        {
          param.sched_priority += priority;
          ret = pthread_attr_setschedparam(&thread_attr,&param);
        }
      else
        {
          ndbg("Cannot Get/Set scheduler parameter thread (%d)\n", ret);
        }

      g_lesp_state.worker.running = true;

      ret = pthread_create(&g_lesp_state.worker.thread, 
                           (ret < 0)?NULL:&thread_attr, lesp_worker, NULL);
      if (ret < 0) 
        {
          ndbg("Cannot Create thread return (%d)\n", ret);
          g_lesp_state.worker.running = false;
        }

      if (pthread_attr_destroy(&thread_attr) < 0)
        {
          ndbg("Cannot destroy thread attribute (%d)\n", ret);
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int lesp_initialize(void)
{
  int ret = 0;

  pthread_mutex_lock(&g_lesp_state.mutex);

  if (g_lesp_state.is_initialized)
    {
      nvdbg("Esp8266 already initialized\n");
      pthread_mutex_unlock(&g_lesp_state.mutex);
      return 0;
    }

  nvdbg("Initializing Esp8266...\n");

  memset(g_lesp_state.sockets, 0, SOCKET_NBR * sizeof(lesp_socket_t));

  if (sem_init(&g_lesp_state.sem, 0, 0) < 0)
    {
      nvdbg("Cannot create semaphore\n");
      pthread_mutex_unlock(&g_lesp_state.mutex);
      return -1;
    }

  if (g_lesp_state.fd < 0)
    {
      g_lesp_state.fd = open(CONFIG_NETUTILS_ESP8266_DEV_PATH, O_RDWR);
    }

  if (g_lesp_state.fd < 0)
    {
      ndbg("Cannot open %s\n", CONFIG_NETUTILS_ESP8266_DEV_PATH);
      ret = -1;
    }

#if 0 // lesp_set_baudrate is not defined
  if (ret >= 0 && lesp_set_baudrate(g_lesp_state.fd, CONFIG_NETUTILS_ESP8266_BAUDRATE) < 0)
    {
      ndbg("Cannot set baud rate %d\n", CONFIG_NETUTILS_ESP8266_BAUDRATE);
      ret = -1;
    }
#endif

  if ((ret >= 0) && (g_lesp_state.worker.running == false))
    {
      ret = lesp_create_worker(CONFIG_NETUTILS_ESP8266_THREADPRIO);
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);
  g_lesp_state.is_initialized = true;
  nvdbg("Esp8266 initialized\n");

  return 0;
}

int lesp_soft_reset(void)
{
  int ret = 0;
  int i;

  /* Rry to close opened reset */

  for(i = 0; i < SOCKET_NBR; i++)
    {
      if ((g_lesp_state.sockets[i].flags & FLAGS_SOCK_USED) != 0)
        {
          lesp_closesocket(i);
        }
    }

  /* Leave time to close socket */

  sleep(1);

  /* Send reset */

  lesp_send_cmd("AT+RST\r\n");

  /* Leave time to reset */

  sleep(1);

  lesp_flush();

  while(lesp_ask_ans_ok(lespTIMEOUT_MS, "ATE0\r\n") < 0)
    {
      sleep(1);
      lesp_flush();
    }

  if (ret >= 0) 
    {
      ret = lesp_ask_ans_ok(lespTIMEOUT_MS,"AT+GMR\r\n");
    }

  /* Enable the module to act as a “Station” */

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(lespTIMEOUT_MS,"AT+CWMODE_CUR=1\r\n");
    }

  /* Enable the multi connection */

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(lespTIMEOUT_MS,"AT+CIPMUX=1\r\n");
    }

  if (ret < 0)
    {
      ret = -1;
    }

  return 0;
}

int lesp_ap_connect(const char* ssid_name, const char* ap_key, int timeout_s)
{
  int ret = 0;

  nvdbg("Starting manual connect...\n");

  if (! g_lesp_state.is_initialized)
    {
      ndbg("ESP8266 not initialized; can't run manual connect\n");
      ret = -1;
    }
  else
    {
      ret = lesp_ask_ans_ok(timeout_s*1000,
                            "AT+CWJAP=\"%s\",\"%s\"\r\n",
                            ssid_name,ap_key);
    }

  if (ret < 0)
    {
      return -1;
    }

  nvdbg("Wifi connected\n");
  return 0;
}

/****************************************************************************
 * Name: lesp_set_net
 *
 * Description:
 *   It will set network ip of mode.
 *   Warning: use lesp_eMODE_STATION or lesp_eMODE_AP.
 *
 * Input Parmeters:
 *   mode    : mode to configure.
 *   ip      : ip of interface.
 *   mask    : network mask of interface.
 *   gateway : gateway ip of network.
 *
 * Returned Value:
 *   0 on success, -1 on error.
 *
 ****************************************************************************/

int lesp_set_net(lesp_mode_t mode, in_addr_t ip, in_addr_t mask, in_addr_t gateway)
{
  int ret = 0;

  ret = lesp_ask_ans_ok(lespTIMEOUT_MS,
                        "AT+CIP%s_CUR="
                        "\"%d.%d.%d.%d\","
                        "\"%d.%d.%d.%d\","
                        "\"%d.%d.%d.%d\""
                        "\r\n",
                        (mode==lesp_eMODE_STATION)?"STA":"AP",
                        *((uint8_t*)&(ip)+0),
                        *((uint8_t*)&(ip)+1),
                        *((uint8_t*)&(ip)+2),
                        *((uint8_t*)&(ip)+3),
                        *((uint8_t*)&(gateway)+0),
                        *((uint8_t*)&(gateway)+1),
                        *((uint8_t*)&(gateway)+2),
                        *((uint8_t*)&(gateway)+3),
                        *((uint8_t*)&(mask)+0),
                        *((uint8_t*)&(mask)+1),
                        *((uint8_t*)&(mask)+2),
                        *((uint8_t*)&(mask)+3));

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
 * Input Parmeters:
 *   mode    : mode to configure.
 *   enable  : true for enable, false for disable.
 *
 * Returned Value:
 *   0 on success, -1 on error.
 *
 ****************************************************************************/

int lesp_set_dhcp(lesp_mode_t mode,bool enable)
{
  int ret = 0;

  ret = lesp_ask_ans_ok(lespTIMEOUT_MS,
                        "AT+CWDHCP_CUR=%d,%c\r\n",
                        mode,(enable)?'1':'0');
  
  if (ret < 0)
    {
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
 * Input Parmeters:
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

  /* Check esp */

  ret = lesp_ask_ans_ok(lespTIMEOUT_MS,"AT\r\n");

  if (ret < 0) 
    {
      ret = lesp_send_cmd("AT+CWLAP\r\n");
    }

  while (ret >= 0)
    {
      ret = lesp_read(lespTIMEOUT_MS_LISP_AP);
      if (ret < 0)
        {
          continue;
        }

      nvdbg("Read:%s\n", g_lesp_state.bufans);

      if (strcmp(g_lesp_state.bufans,"OK") == 0)
        {
          break;
        }

      ret = lesp_parse_cwlap_ans_line(g_lesp_state.bufans,&ap);
      if (ret < 0)
        {
          ndbg("Line badly formed.");
        }

      cb(&ap);
      number++;
    }

  if (ret < 0)
    {
      return -1;
    }

  nvdbg("Access Point list finished with %d ap founds\n", number);

  return number;
}

const char *lesp_security_to_str(lesp_security_t security)
{
  switch(security)
    {
      case lesp_eSECURITY_NONE:
        return "NONE";
      case lesp_eSECURITY_WEP:
        return "WEP";
      case lesp_eSECURITY_WPA_PSK:
        return "WPA_PSK";
      case lesp_eSECURITY_WPA2_PSK:
        return "WPA2_PSK";
      case lesp_eSECURITY_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";
      default:
        return "Unknown";
    }
}

int lesp_socket(int domain, int type, int protocol)
{
  int ret = 0;
  int i = CON_NBR;

  if ((domain != PF_INET) && (type != SOCK_STREAM) && (IPPROTO_TCP))
    {
      ndbg("Not Implemented!\n");
      return -1;
    }

  pthread_mutex_lock(&g_lesp_state.mutex);

  ret = -1;
  if (!g_lesp_state.is_initialized)
    {
      nvdbg("Esp8266 not initialized; can't list access points\n");
    }
  else
    {
      for (i = 0; i < CON_NBR;i++)
        {
          if ((g_lesp_state.sockets[i].flags & FLAGS_SOCK_USED) == 0)
            {
              g_lesp_state.sockets[i].flags |= FLAGS_SOCK_USED;
              ret = i;
              break;
            }
        }
    }

  pthread_mutex_unlock(&g_lesp_state.mutex);

  return ret;
}

int lesp_closesocket(int sockfd)
{
  int ret = 0;
  lesp_socket_t *sock;

  sock = get_sock(sockfd);
  if (sock == NULL)
    {
      ret = -1;
    }

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(lespTIMEOUT_MS,
                            "AT+CIPCLOSE=%d\r\n",
                            sockfd);

      memset(sock, 0, sizeof(lesp_socket_t));
      sock->flags  = 0;
      sock->inndx  = 0;
      sock->outndx = 0;
    }

  if (ret < 0)
    {
      return -1;
    }

  return 0;
}

int lesp_bind(int sockfd, FAR const struct sockaddr *addr, socklen_t addrlen)
{
  ndbg("Not implemented %s\n", __func__);
  return -1;
}

int lesp_connect(int sockfd, FAR const struct sockaddr *addr, socklen_t addrlen)
{
  int ret = 0;
  lesp_socket_t *sock;
  struct sockaddr_in *in;
  unsigned short port;
  in_addr_t ip;

  in = (struct sockaddr_in*)addr;
  port = ntohs(in->sin_port);     // e.g. htons(3490)
  ip = in->sin_addr.s_addr;

  DEBUGASSERT(in->sin_family = AF_INET);

  sock = get_sock(sockfd);
  if (sock == NULL)
    {
      ret = -1;
    }

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(lespTIMEOUT_MS,
                            "AT+CIPSTART=%d,"
                            "\"TCP\","
                            "\"%d.%d.%d.%d\","
                            "%d"
                            "\r\n",
                            sockfd,
                            *((uint8_t*)&(ip)+0),
                            *((uint8_t*)&(ip)+1),
                            *((uint8_t*)&(ip)+2),
                            *((uint8_t*)&(ip)+3),
                            port);
    }

  if (ret < 0)
    {
      return -1;
    }

  return 0;
}

int lesp_listen(int sockfd, int backlog)
{
  ndbg("Not implemented %s\n", __func__);
  return -1;
}

int lesp_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  ndbg("Not implemented %s\n", __func__);
  return -1;
}

ssize_t lesp_send(int sockfd, FAR const uint8_t *buf, size_t len, int flags)
{
  int ret = 0;
  lesp_socket_t *sock;

  UNUSED(flags);

  sock = get_sock(sockfd);
  if (sock == NULL)
    {
      ret = -1;
    }

  if (ret >= 0)
    {
      ret = lesp_ask_ans_ok(lespTIMEOUT_MS,"AT+CIPSEND=%d,%d\r\n",sockfd,len);
    }
  
  if (ret >= 0)
    {
      nvdbg("Sending in socket %d, %d bytes\n", sockfd,len);
      ret = write(g_lesp_state.fd,buf,len);
    }

  while (ret >= 0)
    {
      char * ptr = g_lesp_state.bufans;
      ret = lesp_read(lespTIMEOUT_MS);

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
          if (strcmp(ptr,"SEND OK") == 0)
              break;
        }
    }

  if (ret >= 0)
    {
      nvdbg("Sent\n");
    }

  if (ret < 0)
    {
      ndbg("Cannot send in socket %d, %d bytes\n", sockfd, len);
      return -1;
    }

  return 0;
}

ssize_t lesp_recv(int sockfd, FAR uint8_t *buf, size_t len, int flags)
{
  int ret = 0;
  lesp_socket_t *sock;
  sem_t sem;
  
  if (sem_init(&sem, 0, 0) < 0)
    {
      nvdbg("Cannot create semaphore\n");
      return -1;
    }

  pthread_mutex_lock(&g_lesp_state.mutex);

  UNUSED(flags);

  sock = get_sock(sockfd);
  if (sock == NULL)
    {
      ret = -1;
    }

  if (ret >= 0 && sock->inndx == sock->outndx)
    {
      struct timespec ts;

      if (clock_gettime(CLOCK_REALTIME,&ts) < 0)
        {
          ret = -1;
        }
      else
        {
          ts.tv_sec += lespTIMEOUT_MS_RECV_S;
          sock->sem = &sem;
          while (ret >= 0 && sock->inndx == sock->outndx)
            {
              pthread_mutex_unlock(&g_lesp_state.mutex);
              ret = sem_timedwait(&sem,&ts);
              pthread_mutex_lock(&g_lesp_state.mutex);
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

  pthread_mutex_unlock(&g_lesp_state.mutex);

  if (ret < 0)
    {
      return -1;
    }

  return ret;
}

int lesp_setsockopt(int sockfd, int level, int option,
                    FAR const void *value, socklen_t value_len)
{
  ndbg("Not implemented %s\n", __func__);
  return -1;
}

int lesp_getsockopt(int sockfd, int level, int option, FAR void *value,
                    FAR socklen_t *value_len)
{
  ndbg("Not implemented %s\n", __func__);
  return -1;
}

int lesp_gethostbyname(char *hostname, uint16_t usNameLen,
                       unsigned long *out_ip_addr)
{
  ndbg("Not implemented %s\n", __func__);
  return -1;
}

int lesp_mdnsadvertiser(uint16_t mdnsEnabled, char *deviceServiceName,
                        uint16_t deviceServiceNameLength)
{
  ndbg("Not implemented %s\n", __func__);
  return -1;
}

#endif /* CONFIG_NETUTILS_ESP8266 */
