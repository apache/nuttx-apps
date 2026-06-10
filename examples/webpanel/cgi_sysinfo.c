/****************************************************************************
 * apps/examples/webpanel/cgi_sysinfo.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void get_version(char *ver, size_t vlen,
                        char *build, size_t blen,
                        char *board, size_t boardlen);
static void get_arch(char *arch, size_t archlen);
static unsigned long get_uptime(char *buf, size_t len);
static void get_net_info(char *ip, size_t iplen,
                         char *mask, size_t masklen,
                         char *gw, size_t gwlen,
                         char *mac, size_t maclen);
static int count_files(const char *path);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_version
 *
 * Description:
 *   Read version, build hash, and board information from /proc/version.
 *
 * Input Parameters:
 *   ver      - Output buffer for version string.
 *   vlen     - Size of ver buffer.
 *   build    - Output buffer for build/hash string.
 *   blen     - Size of build buffer.
 *   board    - Output buffer for board string.
 *   boardlen - Size of board buffer.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void get_version(char *ver, size_t vlen,
                        char *build, size_t blen,
                        char *board, size_t boardlen)
{
  FILE *fp;
  char line[256];
  char *p;
  char *sp;
  char *last;
  char *tok;
  size_t n;

  strncpy(ver,   "unknown", vlen);
  strncpy(build, "unknown", blen);
  strncpy(board, "unknown", boardlen);

  fp = fopen("/proc/version", "r");
  if (fp == NULL)
    {
      return;
    }

  if (fgets(line, sizeof(line), fp) != NULL)
    {
      /* Format: "NuttX version X.Y.Z HASH DATE TIME BOARD:CONFIG" */

      p = strstr(line, "version ");
      if (p != NULL)
        {
          p += 8;

          /* version token */

          sp = strchr(p, ' ');
          if (sp != NULL)
            {
              n = sp - p;
              if (n >= vlen)
                {
                  n = vlen - 1;
                }

              memcpy(ver, p, n);
              ver[n] = '\0';

              /* build (git hash) token */

              p = sp + 1;
              sp = strchr(p, ' ');
              if (sp != NULL)
                {
                  n = sp - p;
                  if (n >= blen)
                    {
                      n = blen - 1;
                    }

                  memcpy(build, p, n);
                  build[n] = '\0';
                }
            }
        }

      /* Board name is the last whitespace-separated token; trim :config */

      last = NULL;
      tok  = strtok(line, " \t\r\n");
      while (tok != NULL)
        {
          last = tok;
          tok  = strtok(NULL, " \t\r\n");
        }

      if (last != NULL)
        {
          char *colon = strchr(last, ':');
          if (colon != NULL)
            {
              *colon = '\0';
            }

          strncpy(board, last, boardlen - 1);
          board[boardlen - 1] = '\0';
        }
    }

  fclose(fp);
}

/****************************************************************************
 * Name: get_arch
 *
 * Description:
 *   Query architecture information via uname().
 *
 * Input Parameters:
 *   arch    - Output buffer for architecture string.
 *   archlen - Size of arch buffer.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void get_arch(char *arch, size_t archlen)
{
  struct utsname uts;

  strncpy(arch, "unknown", archlen);

  if (uname(&uts) == 0)
    {
      strncpy(arch, uts.machine, archlen - 1);
      arch[archlen - 1] = '\0';
    }
}

/****************************************************************************
 * Name: get_uptime
 *
 * Description:
 *   Read system uptime based on CLOCK_MONOTONIC and format it.
 *
 * Input Parameters:
 *   buf - Output buffer for formatted uptime.
 *   len - Size of buf.
 *
 * Returned Value:
 *   Uptime in seconds; zero on error.
 *
 ****************************************************************************/

static unsigned long get_uptime(char *buf, size_t len)
{
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
      unsigned long secs = ts.tv_sec;
      unsigned int days = secs / 86400;
      unsigned int hrs  = (secs % 86400) / 3600;
      unsigned int mins = (secs % 3600) / 60;
      unsigned int s    = secs % 60;

      if (days > 0)
        {
          snprintf(buf, len, "%ud %02u:%02u:%02u", days, hrs, mins, s);
        }
      else
        {
          snprintf(buf, len, "%02u:%02u:%02u", hrs, mins, s);
        }

      return secs;
    }

  snprintf(buf, len, "unknown");
  return 0;
}

/****************************************************************************
 * Name: get_net_info
 *
 * Description:
 *   Query IP, netmask, gateway, and MAC for the configured interface.
 *
 * Input Parameters:
 *   ip      - Output buffer for IPv4 address.
 *   iplen   - Size of ip buffer.
 *   mask    - Output buffer for netmask.
 *   masklen - Size of mask buffer.
 *   gw      - Output buffer for gateway.
 *   gwlen   - Size of gw buffer.
 *   mac     - Output buffer for MAC address.
 *   maclen  - Size of mac buffer.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void get_net_info(char *ip, size_t iplen,
                         char *mask, size_t masklen,
                         char *gw, size_t gwlen,
                         char *mac, size_t maclen)
{
  int sockfd;
  struct ifreq ifr;

  strncpy(ip, "N/A", iplen);
  strncpy(mask, "N/A", masklen);
  strncpy(gw, "N/A", gwlen);
  strncpy(mac, "N/A", maclen);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      return;
    }

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, CONFIG_EXAMPLES_WEBPANEL_NETIF, IFNAMSIZ);

  if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0)
    {
      struct sockaddr_in *sa = (struct sockaddr_in *)&ifr.ifr_addr;
      inet_ntop(AF_INET, &sa->sin_addr, ip, iplen);
    }

  if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) == 0)
    {
      struct sockaddr_in *sa = (struct sockaddr_in *)&ifr.ifr_netmask;
      inet_ntop(AF_INET, &sa->sin_addr, mask, masklen);
    }

  if (ioctl(sockfd, SIOCGIFDSTADDR, &ifr) == 0)
    {
      struct sockaddr_in *sa = (struct sockaddr_in *)&ifr.ifr_dstaddr;
      inet_ntop(AF_INET, &sa->sin_addr, gw, gwlen);
    }

  if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0)
    {
      unsigned char *hw = (unsigned char *)ifr.ifr_hwaddr.sa_data;
      snprintf(mac, maclen, "%02x:%02x:%02x:%02x:%02x:%02x",
               hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
    }

  close(sockfd);
}

/****************************************************************************
 * Name: count_files
 *
 * Description:
 *   Count visible files in a directory.
 *
 * Input Parameters:
 *   path - Directory path.
 *
 * Returned Value:
 *   Number of visible entries.
 *
 ****************************************************************************/

static int count_files(const char *path)
{
  DIR *dir;
  struct dirent *ent;
  int count = 0;

  dir = opendir(path);
  if (dir == NULL)
    {
      return 0;
    }

  while ((ent = readdir(dir)) != NULL)
    {
      if (ent->d_name[0] != '.')
        {
          count++;
        }
    }

  closedir(dir);
  return count;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sysinfo_main
 *
 * Description:
 *   CGI entry point that returns board and runtime information as JSON.
 *
 * Input Parameters:
 *   argc - Number of arguments.
 *   argv - Argument vector.
 *
 * Returned Value:
 *   Zero (OK).
 *
 ****************************************************************************/

int sysinfo_main(int argc, FAR char *argv[])
{
  char version[16];
  char build[16];
  char board[64];
  char arch[32];
  char uptime[32];
  char ip[16];
  char mask[16];
  char gw[16];
  char mac[24];
  unsigned long uptime_sec;
  int nfiles;

  get_version(version, sizeof(version), build, sizeof(build),
              board, sizeof(board));
  get_arch(arch, sizeof(arch));
  uptime_sec = get_uptime(uptime, sizeof(uptime));
  get_net_info(ip, sizeof(ip), mask, sizeof(mask),
               gw, sizeof(gw), mac, sizeof(mac));
  nfiles = count_files("/mnt");

  puts("Content-type: application/json\r\n"
       "\r\n");

  printf("{"
         "\"chip\":\"" CONFIG_ARCH_CHIP "\","
         "\"board\":\"%s\","
         "\"os\":\"NuttX\","
         "\"version\":\"%s\","
         "\"build\":\"%s\","
         "\"arch\":\"%s\","
         "\"ifname\":\"" CONFIG_EXAMPLES_WEBPANEL_NETIF "\","
         "\"ip\":\"%s\","
         "\"mask\":\"%s\","
         "\"gw\":\"%s\","
         "\"mac\":\"%s\","
         "\"uptime\":\"%s\","
         "\"uptime_sec\":%lu,"
         "\"files\":%d"
         "}\n",
         board, version, build, arch,
         ip, mask, gw, mac,
         uptime, uptime_sec, nfiles);

  return 0;
}
