/****************************************************************************
 * apps/fsutils/ipcfg/ipcfg.c
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
#include <fcntl.h>
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "fsutils/ipcfg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_LINESIZE  80
#define MAX_IPv4PROTO IPv4PROTO_FALLBACK

/* Values for the record type field */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* IP Configuration record header. */

struct ipcfg_header_s
{
  uint8_t next;         /* Offset to the next IP configuration record */
  sa_family_t type;     /* Must be AF_INET */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && !defined(CONFIG_IPCFG_BINARY)
static const char *g_ipv4proto_name[] =
{
  "none",      /* IPv4PROTO_NONE */
  "static",    /* IPv4PROTO_STATIC */
  "dhcp",      /* IPv4PROTO_DHCP */
  "fallback"   /* IPv4PROTO_FALLBACK */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_allocpath
 *
 * Description:
 *   Allocate memory for and construct the full path to the IPv4
 *   Configuration file.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *
 * Returned Value:
 *   A pointer to the allocated path is returned.  NULL is returned only on
 *  on a failure to allocate.
 *
 ****************************************************************************/

static inline FAR char *ipcfg_allocpath(FAR const char *netdev)
{
#ifndef CONFIG_IPCFG_CHARDEV
  FAR char *path = NULL;
  int ret;

  /* Create the full path to the ipcfg file for the network device.
   * the path of CONFIG_IPCFG_PATH returns to a directory containing
   * multiple files of the form ipcfg-<dev> where <dev> is the network
   * device name.  For example, ipcfg-eth0.
   */

  ret = asprintf(&path, CONFIG_IPCFG_PATH "/ipcfg-%s", netdev);
  if (ret < 0 || path == NULL)
    {
      /* Assume that asprintf failed to allocate memory */

      fprintf(stderr, "ERROR: Failed to create path to ipcfg file: %d\n",
              ret);
      return NULL;
    }

  return path;

#else
  /* In this case CONFIG_IPCFG_PATH is the full path to a character device
   * that describes the configuration of a single network device.
   *
   * It is dup'ed to simplify typing (const vs non-const) and to make the
   * free operation unconditional.
   */

  return strdup(CONFIG_IPCFG_PATH);
#endif
}

/****************************************************************************
 * Name: ipcfg_open (for ASCII mode)
 *
 * Description:
 *   Form the complete path to the ipcfg file and open it.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   stream - Location to return the opened stream
 *   mode   - File fopen mode
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifndef CONFIG_IPCFG_BINARY
static int ipcfg_open(FAR const char *netdev, FAR FILE **stream,
                      FAR const char *mode)
{
  FAR char *path;
  int ret;

  /* Create the full path to the ipcfg file for the network device */

  path = ipcfg_allocpath(netdev);
  if (path == NULL)
    {
      /* Assume failure to allocate memory */

      fprintf(stderr, "ERROR: Failed to create path to ipcfg file\n");
      return -ENOMEM;
    }

  /* Now open the file */

  *stream = fopen(path, mode);
  ret     = OK;

  if (*stream == NULL)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", path, ret);
    }

  free(path);
  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_open (for binary mode)
 *
 * Description:
 *   Form the complete path to the ipcfg file and open it.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   oflags - File open flags
 *   mode   - File creation mode
 *
 * Returned Value:
 *   The open file descriptor is returned on success; a negated errno value
 *   is returned on any failure.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_BINARY
static int ipcfg_open(FAR const char *netdev, int oflags, mode_t mode)
{
  FAR char *path;
  int fd;
  int ret;

  /* Create the full path to the ipcfg file for the network device */

  path = ipcfg_allocpath(netdev);
  if (path == NULL)
    {
      /* Assume failure to allocate memory */

      fprintf(stderr, "ERROR: Failed to create path to ipcfg file\n");
      return -ENOMEM;
    }

  /* Now open the file */

  fd  = open(path, oflags, mode);
  if (fd < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", path, ret);
      goto errout_with_path;
    }

#if defined(CONFIG_IPCFG_OFFSET) && CONFIG_IPCFG_OFFSET > 0
  /* If the binary file is accessed on a character device as a binary
   * file, then there is also an option to seek to a location on the
   * media before reading or writing the file.
   */

  ret = lseek(fd, CONFIG_IPCFG_OFFSET, SEEK_SET);
  if (ret < 0)
    {
      ret = -errno;
      close(fd);
      fprintf(stderr, "ERROR: Failed to seek to $ld: %d\n",
              (long)CONFIG_IPCFG_OFFSET, ret);
      goto errout_with_path;
    }

#endif
  ret = fd;

errout_with_path:
  free(path);
  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_trim
 *
 * Description:
 *   Skip over any whitespace.
 *
 * Input Parameters:
 *   line  - Pointer to line buffer
 *   index - Current index into the line buffer
 *
 * Returned Value:
 *   New value of index.
 *
 ****************************************************************************/

#ifndef CONFIG_IPCFG_BINARY
static int ipcfg_trim(FAR char *line, int index)
{
  int ret;
  while (line[index] != '\0' && isspace(line[index]))
    {
      index++;
    }

  ret = index;
  while (line[index] != '\0')
    {
      if (!isprint(line[index]))
        {
          line[index] = '\0';
          break;
        }

      index++;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_putaddr
 *
 * Description:
 *   Write a <variable>=<address> value pair to the stream.
 *
 * Input Parameters:
 *   stream   - The output stream
 *   variable - The variable namespace
 *   address  - The IP address to write
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && !defined(CONFIG_IPCFG_BINARY)
static void ipcfg_putaddr(FAR FILE *stream, FAR const char *variable,
                          in_addr_t address)
{
  /* REVISIT:  inet_ntoa() is not thread safe. */

  if (address != 0)
    {
      struct in_addr saddr =
      {
        address
      };

      fprintf(stream, "%s=%s\n", variable, inet_ntoa(saddr));
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_read
 *
 * Description:
 *   Read and parse the IP configuration file for the specified network
 *   device.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   ipcfg  - Pointer to a user provided location to receive the IP
 *            configuration.  Refers to either struct ipv4cfg_s or
 *            ipv6cfg_s, depending on the value of af.
 *   af     - Identifies the address family whose IP configuration is
 *            requested.  May be either AF_INET or AF_INET6.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int ipcfg_read(FAR const char *netdev, FAR void *ipcfg, sa_family_t af)
{
#ifdef CONFIG_IPCFG_BINARY
  FAR struct ipv4cfg_s *ipv4cfg = (FAR struct ipv4cfg_s *)ipcfg;
  struct ipcfg_header_s hdr;
  ssize_t nread;
  int fd;
  int ret;

  DEBUGASSERT(netdev != NULL && ipv4cfg != NULL && af == AF_INET);

  /* Open the file */

  fd = ipcfg_open(netdev, O_RDONLY, 0666);
  if (fd < 0)
    {
      return fd;
    }

  /* Read the header */

  nread = read(fd, &hdr, sizeof(struct ipcfg_header_s));
  if (nread < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: Failed to read file: %d\n", ret);
      goto errout_with_fd;
    }
  else if (nread != sizeof(struct ipcfg_header_s))
    {
      ret = -EIO;
      fprintf(stderr, "ERROR: Bad read size: %ld\n", (long)nread);
      goto errout_with_fd;
    }

  /* Verify the header.  Only a single IPv4 header is anticipated */

  if (hdr.next != 0 || hdr.type != AF_INET)
    {
      ret = -EINVAL;
      fprintf(stderr, "ERROR: Bad header: {%u,%u}\n",
              hdr.next, hdr.type);
      goto errout_with_fd;
    }

  /* Read the file content */

  nread = read(fd, ipv4cfg, sizeof(struct ipv4cfg_s));
  if (nread < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: Failed to read file: %d\n", ret);
    }
  else if (nread != sizeof(struct ipv4cfg_s))
    {
      ret = -EIO;
      fprintf(stderr, "ERROR: Bad read size: %ld\n", (long)nread);
    }
  else
    {
      ret = OK;
    }

errout_with_fd:
  close(fd);
  return ret;

#else
  FAR struct ipv4cfg_s *ipv4cfg = (FAR struct ipv4cfg_s *)ipcfg;
  FAR FILE *stream;
  char line[MAX_LINESIZE];
  int index;
  int ret;

  DEBUGASSERT(netdev != NULL && ipv4cfg != NULL && af == AF_INET);

  /* Open the file */

  ret = ipcfg_open(netdev, &stream, "r");
  if (ret < 0)
    {
      return ret;
    }

  /* Process each line in the file */

  memset(ipv4cfg, 0, sizeof(FAR struct ipv4cfg_s));

  while (fgets(line, MAX_LINESIZE, stream) != NULL)
    {
      /* Skip any leading whitespace */

      index = ipcfg_trim(line, 0);

      /* Check for a blank line or a comment */

      if (line[index] != '\0' && line[index] != '#')
        {
          FAR char *variable = &line[index];
          FAR char *value;

          /* Expect <variable>=<value> pair */

          value = strchr(variable, '=');
          if (value == NULL)
            {
              fprintf(stderr, "ERROR: Skipping malformed line in file: %s\n",
                      line);
              continue;
            }

          /* NUL-terminate the variable string */

          *value++ = '\0';

          /* Process the variable assignment */

          if (strcmp(variable, "DEVICE") == 0)
            {
              /* Just assure that it matches the filename */

              if (strcmp(value, netdev) != 0)
                {
                  fprintf(stderr, "ERROR: Bad device in file: %s=%s\n",
                          variable, value);
                }
            }
          else if (strcmp(variable, "IPv4PROTO") == 0)
            {
              if (strcmp(value, "none") == 0)
                {
                  ipv4cfg->proto = IPv4PROTO_NONE;
                }
              else if (strcmp(value, "static") == 0)
                {
                  ipv4cfg->proto = IPv4PROTO_STATIC;
                }
              else if (strcmp(value, "dhcp") == 0)
                {
                  ipv4cfg->proto = IPv4PROTO_DHCP;
                }
              else if (strcmp(value, "fallback") == 0)
                {
                  ipv4cfg->proto = IPv4PROTO_FALLBACK;
                }
              else
                {
                  fprintf(stderr, "ERROR: Unrecognized IPv4PROTO: %s=%s\n",
                          variable, value);
                }
            }
          else if (strcmp(variable, "IPv4IPADDR") == 0)
            {
              ipv4cfg->ipaddr = inet_addr(value);
            }
          else if (strcmp(variable, "IPv4NETMASK") == 0)
            {
              ipv4cfg->netmask = inet_addr(value);
            }
          else if (strcmp(variable, "IPv4ROUTER") == 0)
            {
              ipv4cfg->router = inet_addr(value);
            }
          else if (strcmp(variable, "IPv4DNS") == 0)
            {
              ipv4cfg->dnsaddr = inet_addr(value);
            }
          else
            {
              fprintf(stderr, "ERROR: Unrecognized variable: %s=%s\n",
                     variable, value);
            }
        }
    }

  /* Close the file and return */

  fclose(stream);
  return OK;
#endif
}

/****************************************************************************
 * Name: ipcfg_write
 *
 * Description:
 *   Write the IP configuration file for the specified network device.
 *
 * Input Parameters:
 *   netdev - The network device.  For examplel "eth0"
 *   ipcfg  - The IP configuration to be written.  Refers to either struct
 *            ipv4cfg_s or ipv6cfg_s, depending on the value of af.
 *   af     - Identifies the address family whose IP configuration is
 *            to be written.  May be either AF_INET or AF_INET6.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_WRITABLE
int ipcfg_write(FAR const char *netdev, FAR const void *ipcfg,
                sa_family_t af)
{
#ifdef CONFIG_IPCFG_BINARY
  FAR struct ipv4cfg_s *ipv4cfg = (FAR struct ipv4cfg_s *)ipcfg;
  struct ipcfg_header_s hdr;
  ssize_t nwritten;
  int fd;
  int ret;

  DEBUGASSERT(netdev != NULL && ipv4cfg != NULL && af == AF_INET);

  /* Open the file */

  fd = ipcfg_open(netdev, O_WRONLY | O_TRUNC | O_CREAT, 0666);
  if (fd < 0)
    {
      return fd;
    }

  /* Write the file header */

  hdr.next = 0;
  hdr.type = AF_INET;

  nwritten = write(fd, &hdr, sizeof(struct ipcfg_header_s));
  if (nwritten < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: Failed to write to file: %d\n", ret);
      goto errout_with_fd;
    }
  else if (nwritten != sizeof(struct ipcfg_header_s))
    {
      ret = -EIO;
      fprintf(stderr, "ERROR: Bad write size: %ld\n", (long)nwritten);
      goto errout_with_fd;
    }

  /* Write the file content */

  nwritten = write(fd, ipv4cfg, sizeof(struct ipv4cfg_s));
  if (nwritten < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: Failed to write file: %d\n", ret);
    }
  else if (nwritten != sizeof(struct ipv4cfg_s))
    {
      ret = -EIO;
      fprintf(stderr, "ERROR: Bad write size: %ld\n", (long)nwritten);
    }
  else
    {
      ret = OK;
    }

errout_with_fd:
  close(fd);
  return ret;

#else
  FAR struct ipv4cfg_s *ipv4cfg = (FAR struct ipv4cfg_s *)ipcfg;
  FAR FILE *stream;
  int ret;

  DEBUGASSERT(netdev != NULL && ipv4cfg != NULL && af == AF_INET);

  /* Open the file */

  ret = ipcfg_open(netdev, &stream, "w");
  if (ret < 0)
    {
      return ret;
    }

  /* Format and write the file */

  if ((unsigned)ipv4cfg->proto > MAX_IPv4PROTO)
    {
      fprintf(stderr, "ERROR: Unrecognized IPv4PROTO value: %d\n",
              ipv4cfg->proto);
      return -EINVAL;
    }

  fprintf(stream, "DEVICE=%s\n", netdev);
  fprintf(stream, "IPv4PROTO=%s\n", g_ipv4proto_name[ipv4cfg->proto]);

  ipcfg_putaddr(stream, "IPv4IPADDR",  ipv4cfg->ipaddr);
  ipcfg_putaddr(stream, "IPv4NETMASK", ipv4cfg->netmask);
  ipcfg_putaddr(stream, "IPv4ROUTER",  ipv4cfg->router);
  ipcfg_putaddr(stream, "IPv4DNS",     ipv4cfg->dnsaddr);

  /* Close the file and return */

  fclose(stream);
  return OK;
#endif
}
#endif
