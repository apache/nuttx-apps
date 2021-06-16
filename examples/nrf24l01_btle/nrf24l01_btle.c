/****************************************************************************
 * apps/examples/nrf24l01_btle/nrf24l01_btle.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <debug.h>

#include <nuttx/signal.h>
#include <nuttx/sensors/dhtxx.h>
#include <nuttx/wireless/nrf24l01.h>

#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <fcntl.h>

#include "nrf24l01_btle.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define DEV_NAME   "/dev/nrf24l01"

#ifndef STDIN_FILENO
#  define STDIN_FILENO 0
#endif

#ifdef CONFIG_DEBUG_WIRELESS
#  define nrf24_dumpbuffer(m,b,s) lib_dumpbuffer(m,b,s)
#endif

#ifdef CONFIG_WL_NRF24L01_RXSUPPORT
/* If RX support is enabled, poll both stdin and the message reception */
#  define N_PFDS  2
#else
/* If RX support is not enabled, we cannot poll the wireless device */
#  define N_PFDS  1
#endif

static struct pollfd pfds[N_PFDS];
#define DEFAULT_TXPOWER    -6    /* (0, -6, -12, or -18 dBm) */

static uint8_t mac[6] =
  {
    0x79, 0x6a, 0x64, 0x77, 0x62, 0x6a
  };

/* logical BTLE channel number (37-39) */

const uint8_t channel[3] =
  {
    37, 38, 39
  };

/* physical frequency (2400+x MHz)  */

const uint8_t frequency[3] =
  {
    2, 26, 80
  };

const uint8_t adve_name[5] =
  {
    'n', 'R', 'F', '2', '4'
  };

struct btle_adv_pdu buffer;
volatile bool quit;

uint8_t current = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

uint8_t swapbits(uint8_t a)
{
  /* reverse the bit order in a single byte */

  uint8_t v = 0;
  if (a & 0x80) v |= 0x01;
  if (a & 0x40) v |= 0x02;
  if (a & 0x20) v |= 0x04;
  if (a & 0x10) v |= 0x08;
  if (a & 0x08) v |= 0x10;
  if (a & 0x04) v |= 0x20;
  if (a & 0x02) v |= 0x40;
  if (a & 0x01) v |= 0x80;
  return v;
}

/* see BT Core Spec 4.0, Section 6.B.3.2 */

static inline void whiten(uint8_t len)
{
  uint8_t i;
  uint8_t * buf = (uint8_t *)&buffer;

  /* initialize LFSR with current channel, set bit 6 */

  uint8_t lfsr = channel[current] | 0x40;

  while (len--)
    {
      uint8_t res = 0;

      /* LFSR in "wire bit order" */

      for (i = 1; i; i <<= 1)
        {
          if (lfsr & 0x01)
            {
              lfsr ^= 0x88;
              res |= i;
            }

          lfsr >>= 1;
        }

      *(buf++) ^= res;
    }
}

/* see BT Core Spec 4.0, Section 6.B.3.1.1 */

static inline void crc(uint8_t len, uint8_t * dst)
{
  uint8_t i;
  uint8_t * buf = (uint8_t *)&buffer;

  /**
   * initialize 24-bit shift register in "wire bit order"
   * dst[0] = bits 23-16, dst[1] = bits 15-8, dst[2] = bits 7-0.
   **/

  dst[0] = 0xaa;
  dst[1] = 0xaa;
  dst[2] = 0xaa;

  while (len--)
    {
      uint8_t d = *(buf++);
      for (i = 1; i; i <<= 1, d >>= 1)
        {
          /**
           * save bit 23 (highest-value),
           * left-shift the entire register by one
           **/

          uint8_t t = dst[0] & 0x01;         dst[0] >>= 1;
          if (dst[1] & 0x01) dst[0] |= 0x80; dst[1] >>= 1;
          if (dst[2] & 0x01) dst[1] |= 0x80; dst[2] >>= 1;

          /**
           * if the bit just shifted out (former bit 23) and the incoming
           * data bit are not equal (i.e. bit_out ^ bit_in == 1) => toggle
           * tap bits
           */

          if (t != (d & 1))
            {
              /**
               * toggle register tap bits (=XOR with 1)
               * according to CRC polynom
               **/

              /* 0b11011010 inv. = 0b01011011 ^= x^6+x^4+x^3+x+1 */

              dst[2] ^= 0xda;

              /* 0b01100000 inv. = 0b00000110 ^= x^10+x^9 */

              dst[1] ^= 0x60;
            }
        }
    }
}

/* change buffer contents to "wire bit order" */

static inline void swapbuf(uint8_t len)
{
  uint8_t * buf = (uint8_t *)&buffer;
  while (len--)
    {
      uint8_t a = *buf;
      *(buf++) = swapbits(a);
    }
}

int nrf24_cfg(int fd)
{
  int error = 0;

  uint32_t rf = NRF24L01_MIN_FREQ + frequency[current];
  int32_t txpow = DEFAULT_TXPOWER;

  /**************************************************************************
   * if using RATE_1Mbps from include/nuttx/wireless/nrf24l01.h,
   * tools/checkpatch.sh report error: Mixed case identifier found.
   *
   **************************************************************************/

  nrf24l01_datarate_t datarate = 0; /* RATE_1Mbps */
  nrf24l01_retrcfg_t retrcfg =
    {
      .count = 0,
      .delay = 3 /* DELAY_1000us */
    };

  uint32_t addrwidth = 4;

  uint8_t pipes_en = (1 << 0);  /* Only pipe #0 is enabled */

  /**************************************************************************
   * Define the pipe #0 parameters (AA enabled and dynamic payload length).
   * 4 byte of access address, which is always 0x8E89BED6 for advertizing
   * packets.
   *
   **************************************************************************/

  nrf24l01_pipecfg_t pipe0cfg =
    {
     .en_aa = false,
     .payload_length = 32,
     .rx_addr =
        {
          swapbits(0x8e), swapbits(0x89), swapbits(0xbe), swapbits(0xd6)
        }
    };

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

  /* Set radio parameters */

  ioctl(fd, NRF24L01IOC_SETRETRCFG,
        (unsigned long)((nrf24l01_retrcfg_t *)&retrcfg));

  ioctl(fd, WLIOC_SETRADIOFREQ, (unsigned long)((uint32_t *)&rf));
  ioctl(fd, WLIOC_SETTXPOWER, (unsigned long)((int32_t *)&txpow));
  ioctl(fd, NRF24L01IOC_SETDATARATE,
        (unsigned long)((nrf24l01_datarate_t *)&datarate));

  ioctl(fd, NRF24L01IOC_SETADDRWIDTH,
        (unsigned long)((uint32_t *)&addrwidth));

  /* set advertisement address: 0x8E89BED6 (bit-reversed -> 0x6B7D9171) */

  ioctl(fd, NRF24L01IOC_SETTXADDR,
           (unsigned long)((uint8_t *)&pipe0cfg.rx_addr));

  ioctl(fd, NRF24L01IOC_SETPIPESCFG,
        (unsigned long)((nrf24l01_pipecfg_t **)&pipes_cfg));
  ioctl(fd, NRF24L01IOC_SETPIPESENABLED,
        (unsigned long)((uint8_t *)&pipes_en));

  ioctl(fd, NRF24L01IOC_SETSTATE,
        (unsigned long)((nrf24l01_state_t *)&primrxstate));

  return error;
}

int nrf24_open(void)
{
  int fd;

  fd = open(DEV_NAME, O_RDWR);

  if (fd < 0)
    {
      perror("Cannot open nRF24L01 device");
    }
  else
    {
      nrf24_cfg(fd);
    }

  return fd;
}

int nrf24_send(int wl_fd, uint8_t * buf, uint8_t len)
{
  int ret;

  wlinfo("send buffer len %d\n", len);
  ret = write(wl_fd, buf, len);
  if (ret < 0)
    {
      wlerr("Error sending packet\n");
      return ret;
    }

  return OK;
}

static inline void generate_mac(void)
{
  srand(time(NULL));
  mac[0] = 0x42;
  mac[1] = rand() % 256;
  mac[2] = rand() % 256;
  mac[3] = rand() % 256;
  mac[4] = rand() % 256;
  mac[5] = rand() % 256;
}

/****************************************************************************
 * Broadcast an advertisement packet with a specific data type
 * Standardized data types can be seen here:
 * https://www.bluetooth.org/en-us/specification/assigned-
 * numbers/generic-access-profile
 *
 ****************************************************************************/

static void adv_packet(int wl_fd)
{
  uint8_t namelen;
  uint8_t pls = 0;
  uint8_t i;
  uint8_t ret;
#ifdef CONFIG_NRF24L01_BTLE_DHT11
  int fd;
  struct dhtxx_sensor_data_s dht_data;
  struct nrf_service_data *temp;
  struct nrf_service_data *hum;
  fd = open("/dev/hum0", O_RDWR);
#endif

  namelen = sizeof(adve_name);

  /* hop channel */

  ioctl(wl_fd, WLIOC_SETRADIOFREQ,
        (unsigned long)((uint32_t *)&frequency[current]));

  memcpy(&buffer.mac[0], &mac[0], 6);

  /* add device descriptor chunk */

  chunk(buffer, pls)->size = 0x02;  /* chunk size: 2 */
  chunk(buffer, pls)->type = 0x01;  /* chunk type: device flags */

  /* flags: LE-only, limited discovery mode */

  chunk(buffer, pls)->data[0] = 0x06;
  pls += 3;

  /* add device name chunk */

  chunk(buffer, pls)->size = namelen + 1;
  chunk(buffer, pls)->type = 0x09;
  for (i = 0; i < namelen; i++)
    chunk(buffer, pls)->data[i] = adve_name[i];
  pls += namelen + 2;

  /* add custom data, if applicable */

#ifdef CONFIG_NRF24L01_BTLE_DHT11
  ret = read(fd, &dht_data, sizeof(struct dhtxx_sensor_data_s));
  if (ret < 0)
    {
      printf("Read error.\n");
      printf("Sensor reported error %d\n", dht_data.status);
    }

  /* set temperature */

  chunk(buffer, pls)->size = 3 + 1;  /* chunk size */

  /* chunk type */

  chunk(buffer, pls)->type = 0x16;
  temp  = chunk(buffer, pls)->data;
  temp->service_uuid = NRF_TEMPERATURE_SERVICE_UUID;
  temp->value = (uint8_t)dht_data.temp;
  pls += 3 + 2;

  /* set humidity  */

  chunk(buffer, pls)->size = 3 + 1;
  chunk(buffer, pls)->type = 0x16;
  hum  = chunk(buffer, pls)->data;
  hum->service_uuid = NRF_ENVIRONMENTAL_SERVICE_UUID;
  hum->value = (uint8_t)dht_data.hum;
  pls += 3 + 2;
#else
  chunk(buffer, pls)->size = 4 + 1;
  chunk(buffer, pls)->type = 0xff; /* custom data */
  chunk(buffer, pls)->data[0] = 't';
  chunk(buffer, pls)->data[1] = 'e';
  chunk(buffer, pls)->data[2] = 's';
  chunk(buffer, pls)->data[3] = 't';
  pls += 4 + 2;
  sleep(1);
#endif

  if (pls > 21)
    {
     wlerr("Total payload size must be 21 bytes or less.\n");
    }

  buffer.payload[pls] = 0x55;
  buffer.payload[pls + 1] = 0x55;
  buffer.payload[pls + 2] = 0x55;

  /**************************************************************************
   * The Payload field consists of AdvA and AdvData fields.
   * The AdvA field shall  contain the advertiser’s public or
   * random device address as indicated by TxAdd.
   * -----------------------------------------------------------
   * |   PDU  |   Type  |  RFU   | TxAdd  | RxAdd   |Length RFU|
   * |--------+---------+--------+--------+---------+----------|
   * |(4 bits)| (2 bits)| (1 bit)| (1 bit)| (6 bits)| (2 bits) |
   * -----------------------------------------------------------
   * 0x42 = 0b1000010; include ADV_NONCONN_IND and TxAdd.
   *
   **************************************************************************/

  buffer.pdu_type = 0x42;

  /* set final payload size in header include MAC length */

  buffer.pl_size = pls + 6;

  /* calculate CRC over header+MAC+payload, append after payload */

  uint8_t * outbuf = (uint8_t *)&buffer;
#ifdef CONFIG_DEBUG_WIRELESS
  syslog(LOG_INFO, "payload len: %d\n ", pls);
  nrf24_dumpbuffer("Hex Dump", outbuf, sizeof(buffer));
#endif
  crc(pls + 8, outbuf + pls + 8);
  whiten(pls + 11);
  swapbuf(pls + 11);
#ifdef CONFIG_NRF24L01_BTLE_DHT11
  close(fd);
#endif
  nrf24_send(wl_fd, (uint8_t *)&buffer, pls + 11);
}

FAR void *advertise(FAR void *arg)
{
  uint32_t wl_fd = *(uint32_t *)arg;
  while (!quit)
    {
      if (current == 2)
        {
         current = 0;
        }

       adv_packet(wl_fd);
       current++;
    }
}

int nrf24_read(int wl_fd)
{
  int ret;
  uint32_t pipeno;
  uint8_t rbuf[32];

  ret = read(wl_fd, rbuf, sizeof(rbuf));
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

  rbuf[ret] = '\0';   /* end the string */

#ifdef CONFIG_DEBUG_WIRELESS
  syslog(LOG_INFO, "Message received : (on pipe %d)\n",  pipeno);
  nrf24_dumpbuffer("Hex Dump", &rbuf[0], 32);
#endif
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  struct sched_param param;
  pthread_t thread;
  pthread_attr_t attr;
  char c;
  int wl_fd;
  quit = false;
  current = 0;
  wl_fd = nrf24_open();
  if (wl_fd < 0)
    {
      return -1;
    }

#ifdef EXAMPLES_NRF24L01_BTLE_RAND_MAC
  generate_mac();
#endif
  pthread_attr_init(&attr);
  param.sched_priority = CONFIG_EXAMPLES_NRF24L01_BTLE_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NRF24L01_BTLE_STACKSIZE);

  ret = pthread_create(&thread, &attr, advertise, &wl_fd);
  if (ret != 0)
    {
      printf("nrf24l01_btle: pthread_create failed: %d\n", ret);
      return ERROR;
    }

  printf("nRF24L01+ wireless btle demo.\n");
  printf("For basic Bluetooth Low Energy support using the nRF24L01+\n");
  printf("sending on the advertising broadcast channel\n");
  printf("Type 'q' to exit.\n\n");

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
          read(STDIN_FILENO, &c, 1);

          if (c == 'q')
            {
              /* Any non printable char -> exits */

              quit = true;
              printf ("Bye nRF24l01!\n");
              sleep(2);
              goto out;
            }
        }

      if (!quit && (pfds[1].revents & POLLIN))
        {
          nrf24_read(wl_fd);
        }
    }

out:
  close(wl_fd);
  return 0;
}
