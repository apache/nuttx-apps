/****************************************************************************
 * apps/fsutils/ipcfg/ipcfg_binary.c
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
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <debug.h>

#include "fsutils/ipcfg.h"
#include "ipcfg.h"

#ifdef CONFIG_IPCFG_BINARY

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_open (for binary mode)
 *
 * Description:
 *   Form the complete path to the ipcfg file and open it.
 *
 * Input Parameters:
 *   path   - The full path to the IP configuration file
 *   oflags - File open flags
 *   mode   - File creation mode
 *
 * Returned Value:
 *   The open file descriptor is returned on success; a negated errno value
 *   is returned on any failure.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_BINARY
static int ipcfg_open(FAR const char *path, int oflags, mode_t mode)
{
  int fd;
  int ret;

  /* Now open the file */

  fd  = open(path, oflags, mode);
  if (fd < 0)
    {
      ret = -errno;
      if (ret != -ENOENT)
        {
          ferr("ERROR: Failed to open %s: %d\n", path, ret);
        }

      return ret;
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
      ferr("ERROR: Failed to seek to $ld: %d\n",
           (long)CONFIG_IPCFG_OFFSET, ret);

      close(fd);
      return ret;
    }

#endif

  return fd;
}
#endif

/****************************************************************************
 * Name: ipcfg_read_binary
 *
 * Description:
 *   Read from a binary IP Configuration file.
 *
 * Input Parameters:
 *   fd     - File descriptor of the open file to read from
 *   buffer - Location to read from
 *   nbytes - Number of bytes to read
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

static int ipcfg_read_binary(int fd, FAR void *buffer, size_t nbytes)
{
  ssize_t nread;
  int ret;

  /* Read from the file */

  nread = read(fd, buffer, nbytes);
  if (nread < 0)
    {
      ret = -errno;
      ferr("ERROR: Failed to read from file: %d\n", ret);
    }
  else if (nread != nbytes)
    {
      ret = -EIO;
      ferr("ERROR: Bad read size: %ld\n", (long)nread);
    }
  else
    {
      ret = OK;
    }

  return ret;
}

/****************************************************************************
 * Name: ipcfg_write_binary
 *
 * Description:
 *   Write to a binary IP Configuration file.
 *
 * Input Parameters:
 *   fd     - File descriptor of the open file to write to
 *   buffer - Location to write to
 *   nbytes - Number of bytes to wrtie
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_WRITABLE
static int ipcfg_write_binary(int fd, FAR const void *buffer, size_t nbytes)
{
  ssize_t nwritten;
  int ret;

  /* Read from the file */

  nwritten = write(fd, buffer, nbytes);
  if (nwritten < 0)
    {
      ret = -errno;
      ferr("ERROR: Failed to write to file: %d\n", ret);
    }
  else if (nwritten != nbytes)
    {
      ret = -EIO;
      ferr("ERROR: Bad write size: %ld\n", (long)nwritten);
    }
  else
    {
      ret = OK;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_find_binary
 *
 * Description:
 *   Read the location of IPv4 data in a binary IP Configuration file.
 *
 * Input Parameters:
 *   fd     - File descriptor of the open file to read from
 *   af     - Identifies the address family whose IP configuration is
 *            requested.  May be either AF_INET or AF_INET6.
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

static int ipcfg_find_binary(int fd, sa_family_t af)
{
  struct ipcfg_header_s hdr;
  off_t pos;
  int ret;

  for (; ; )
    {
      /* Read the header (careful.. could be uninitialized in the case of a
       * character driver).
       */

      ret = ipcfg_read_binary(fd, &hdr, sizeof(struct ipcfg_header_s));
      if (ret < 0)
        {
          /* Return on any read error */

          return (int)ret;
        }
      else if (hdr.version == IPCFG_VERSION && hdr.type == af)
        {
          return OK;
        }
      else if (hdr.version != IPCFG_VERSION ||
               (hdr.type != AF_INET && hdr.type != AF_INET6))
        {
          return -EINVAL;
        }
      else if (hdr.next == 0)
        {
          ferr("ERROR: IP configuration not found\n");
          return -ENOENT;
        }

      /* Skip to the next IP configuration record */

      pos = lseek(fd, hdr.next, SEEK_CUR);
      if (pos < 0)
        {
          ret = -errno;
          ferr("ERROR: lseek failed: %d\n", ret);
          return ret;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_read_binary_ipv4
 *
 * Description:
 *   Read IPv4 configuration from a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv4cfg - Location to read IPv4 configration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
int ipcfg_read_binary_ipv4(FAR const char *path,
                           FAR struct ipv4cfg_s *ipv4cfg)
{
  int fd;
  int ret;

  DEBUGASSERT(path != NULL && ipv4cfg != NULL);

  /* Open the file for reading */

  fd = ipcfg_open(path, O_RDONLY, 0666);
  if (fd < 0)
    {
      return fd;
    }

  /* Find the IPv4 binary in the IP configuration file */

  ret = ipcfg_find_binary(fd, AF_INET);
  if (ret < 0)
    {
      goto errout_with_fd;
    }

  /* Read the IPv4 Configuration */

  ret = ipcfg_read_binary(fd, ipv4cfg, sizeof(struct ipv4cfg_s));

errout_with_fd:
  close(fd);
  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_read_binary_ipv6
 *
 * Description:
 *   Read IPv6 configuration from a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv6cfg - Location to read IPv6 configration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
int ipcfg_read_binary_ipv6(FAR const char *path,
                           FAR struct ipv6cfg_s *ipv6cfg)
{
  int fd;
  int ret;

  DEBUGASSERT(fd >= 0 && ipv6cfg != NULL);

  /* Open the file for reading */

  fd = ipcfg_open(path, O_RDONLY, 0666);
  if (fd < 0)
    {
      return fd;
    }

  /* Find the IPv6 binary in the IP configuration file */

  ret = ipcfg_find_binary(fd, AF_INET6);
  if (ret < 0)
    {
      goto errout_with_fd;
    }

  /* Read the IPv6 Configuration */

  ret = ipcfg_read_binary(fd, ipv6cfg, sizeof(struct ipv6cfg_s));

errout_with_fd:
  close(fd);
  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_write_binary_ipv4
 *
 * Description:
 *   Write the IPv4 configuration to a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv4cfg - The IPv4 configration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv4)
int ipcfg_write_binary_ipv4(FAR const char *path,
                            FAR const struct ipv4cfg_s *ipv4cfg)
{
#ifdef CONFIG_NET_IPv6
  struct ipv6cfg_s ipv6cfg;
#endif
  struct ipcfg_header_s hdr;
  bool ipv6 = false;
  int fd;
  int ret;

  DEBUGASSERT(fd >= 0 && ipv4cfg != NULL);

#ifdef CONFIG_NET_IPv6
  /* Read any IPv6 data in the file */

  ret = ipcfg_read_binary_ipv6(path, &ipv6cfg);
  if (ret < 0)
    {
      /* -ENOENT is not an error.  It simply means that there is no IPv6
       * configuration in the file.
       */

      if (ret != -ENOENT)
        {
          return ret;
        }
    }
  else
    {
      ipv6 = true;
    }
#endif

  /* Open the file for writing (truncates) */

  fd = ipcfg_open(path, O_WRONLY | O_TRUNC | O_CREAT, 0666);
  if (fd < 0)
    {
      return fd;
    }

  /* Write the IPv4 file header */

  hdr.next    = ipv6 ? sizeof(struct ipv4cfg_s) : 0;
  hdr.type    = AF_INET;
  hdr.version = IPCFG_VERSION;

  ret = ipcfg_write_binary(fd, &hdr, sizeof(struct ipcfg_header_s));
  if (ret < 0)
    {
      goto errout_with_fd;
    }

  /* Write the IPv4 configuration */

  ret = ipcfg_write_binary(fd, ipv4cfg, sizeof(struct ipv4cfg_s));
  if (ret < 0)
    {
      goto errout_with_fd;
    }

#ifdef CONFIG_NET_IPv6
  /* Followed by any IPv6 data in the file */

  if (ipv6)
    {
      /* Write the IPv6 header */

      hdr.next    = 0;
      hdr.type    = AF_INET6;
      hdr.version = IPCFG_VERSION;

      ret = ipcfg_write_binary(fd, &hdr, sizeof(struct ipcfg_header_s));
      if (ret >= 0)
        {
          /* Write the IPv6 configuration */

          ret = ipcfg_write_binary(fd, &ipv6cfg, sizeof(struct ipv6cfg_s));
        }
    }
#endif

errout_with_fd:
  close(fd);
  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_write_binary_ipv6
 *
 * Description:
 *   Write the IPv6 configuration to a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   ipv6cfg - The IPv6 configration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv6)
int ipcfg_write_binary_ipv6(FAR const char *path,
                            FAR const struct ipv6cfg_s *ipv6cfg)
{
#ifdef CONFIG_NET_IPv4
  struct ipv4cfg_s ipv4cfg;
  bool ipv4 = false;
#endif
  struct ipcfg_header_s hdr;
  int fd;
  int ret;

  DEBUGASSERT(path != NULL && ipv6cfg != NULL);

#ifdef CONFIG_NET_IPv4
  /* Read any IPv4 data in the file */

  ret = ipcfg_read_binary_ipv4(path, &ipv4cfg);
  if (ret < 0)
    {
      /* -ENOENT is not an error.  It simply means that there is no IPv4
       * configuration in the file.
       */

      if (ret != -ENOENT)
        {
          return ret;
        }
    }
  else
    {
      ipv4 = true;
    }
#endif

  /* Open the file for writing (truncates) */

  fd = ipcfg_open(path, O_WRONLY | O_TRUNC | O_CREAT, 0666);
  if (fd < 0)
    {
      return fd;
    }

#ifdef CONFIG_NET_IPv4
  if (ipv4)
    {
      /* Write the IPv4 header */

      hdr.next    = sizeof(struct ipv4cfg_s);
      hdr.type    = AF_INET;
      hdr.version = IPCFG_VERSION;

      ret = ipcfg_write_binary(fd, &hdr, sizeof(struct ipcfg_header_s));
      if (ret < 0)
        {
          goto errout_with_fd;
        }

      /* Write the IPv4 configuration.  This is really unnecessary in most
       * cases since the IPv4 data should already be in place.
       */

      ret = ipcfg_write_binary(fd, &ipv4cfg, sizeof(struct ipv4cfg_s));
      if (ret < 0)
        {
          goto errout_with_fd;
        }
    }
#endif

  /* Write the IPv6 file header */

  hdr.next    = 0;
  hdr.type    = AF_INET6;
  hdr.version = IPCFG_VERSION;

  ret = ipcfg_write_binary(fd, &hdr, sizeof(struct ipcfg_header_s));
  if (ret >= 0)
    {
      /* Write the IPv6 configuration */

      ret = ipcfg_write_binary(fd, ipv6cfg, sizeof(struct ipv6cfg_s));
    }

#ifdef CONFIG_NET_IPv4
errout_with_fd:
#endif
  close(fd);
  return ret;
}
#endif

#endif /* CONFIG_IPCFG_BINARY */
