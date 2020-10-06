/****************************************************************************
 * apps/fsutils/ipcfg/ipcfg_text.c
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
#include <ctype.h>
#include <debug.h>

#include <arpa/inet.h>

#include "fsutils/ipcfg.h"
#include "ipcfg.h"

#ifndef CONFIG_IPCFG_BINARY

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv4)
static const char *g_ipv4proto_name[] =
{
  "none",      /* IPv4PROTO_NONE */
  "static",    /* IPv4PROTO_STATIC */
  "dhcp",      /* IPv4PROTO_DHCP */
  "fallback"   /* IPv4PROTO_FALLBACK */
};
#endif

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv6)
static const char *g_ipv6proto_name[] =
{
  "none",      /* IPv6PROTO_NONE */
  "static",    /* IPv6PROTO_STATIC */
  "autoconf",  /* IPv6PROTO_AUTOCONF */
  "fallback"   /* IPv6PROTO_FALLBACK */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
 * Name: ipcfg_put_ipv4addr
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
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv4)
static int ipcfg_put_ipv4addr(FAR FILE *stream, FAR const char *variable,
                              in_addr_t address)
{
  if (address != 0)
    {
      struct in_addr saddr =
      {
        address
      };

      char converted[INET_ADDRSTRLEN];

      /* Convert the address to ASCII text */

      if (inet_ntop(AF_INET, &saddr, converted, INET_ADDRSTRLEN) == NULL)
        {
          int ret = -errno;
          ferr("ERROR: inet_ntop() failed: %d\n", ret);
          return ret;
        }

      fprintf(stream, "%s=%s\n", variable, converted);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: ipcfg_check_ipv6addr
 *
 * Description:
 *   Check for a valid IPv6 address, i.e., not all zeroes.
 *
 * Input Parameters:
 *   address - A pointer to the address to check.
 *
 * Returned value:
 *   Zero (OK) is returned if the address is non-zero.  -ENXIO is returned if
 *   the address is zero.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
static int ipcfg_check_ipv6addr(FAR const struct in6_addr *address)
{
  int i;

  for (i = 0; i < 4; i++)
    {
      if (address->s6_addr32[i] != 0)
        {
          return OK;
        }
    }

  return -ENXIO;
}
#endif

/****************************************************************************
 * Name: ipcfg_put_ipv6addr
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
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv6)
static int ipcfg_put_ipv6addr(FAR FILE *stream, FAR const char *variable,
                              FAR const struct in6_addr *address)
{
  /* If the address is all zero, then omit it */

  if (ipcfg_check_ipv6addr(address) == OK)
    {
      char converted[INET6_ADDRSTRLEN];

      /* Convert the address to ASCII text */

      if (inet_ntop(AF_INET6, address, converted, INET6_ADDRSTRLEN) == NULL)
        {
          int ret = -errno;
          ferr("ERROR: inet_ntop() failed: %d\n", ret);
          return ret;
        }

      fprintf(stream, "%s=%s\n", variable, converted);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: ipcfg_write_ipv4
 *
 * Description:
 *   Write the IPv4 configuration to a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   stream  - Stream of the open file to write to
 *   ipv4cfg - The IPv4 configration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv4)
static int ipcfg_write_ipv4(FAR FILE *stream,
                            FAR const struct ipv4cfg_s *ipv4cfg)
{
  /* Format and write the file */

  if ((unsigned)ipv4cfg->proto > MAX_IPv4PROTO)
    {
      ferr("ERROR: Unrecognized IPv4PROTO value: %d\n", ipv4cfg->proto);
      return -EINVAL;
    }

  fprintf(stream, "IPv4PROTO=%s\n", g_ipv4proto_name[ipv4cfg->proto]);

  ipcfg_put_ipv4addr(stream, "IPv4IPADDR",  ipv4cfg->ipaddr);
  ipcfg_put_ipv4addr(stream, "IPv4NETMASK", ipv4cfg->netmask);
  ipcfg_put_ipv4addr(stream, "IPv4ROUTER",  ipv4cfg->router);
  ipcfg_put_ipv4addr(stream, "IPv4DNS",     ipv4cfg->dnsaddr);

  return OK;
}
#endif

/****************************************************************************
 * Name: ipcfg_write_ipv6
 *
 * Description:
 *   Write the IPv6 configuration to a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   stream  - Stream of the open file to write to
 *   ipv6cfg - The IPv6 configration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv6)
static int ipcfg_write_ipv6(FAR FILE *stream,
                            FAR const struct ipv6cfg_s *ipv6cfg)
{
  /* Format and write the file */

  if ((unsigned)ipv6cfg->proto > MAX_IPv6PROTO)
    {
      ferr("ERROR: Unrecognized IPv6PROTO value: %d\n", ipv6cfg->proto);
      return -EINVAL;
    }

  fprintf(stream, "IPv6PROTO=%s\n", g_ipv6proto_name[ipv6cfg->proto]);

  ipcfg_put_ipv6addr(stream, "IPv6IPADDR",  &ipv6cfg->ipaddr);
  ipcfg_put_ipv6addr(stream, "IPv6NETMASK", &ipv6cfg->netmask);
  ipcfg_put_ipv6addr(stream, "IPv6ROUTER",  &ipv6cfg->router);

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_read_text_ipv4
 *
 * Description:
 *   Read IPv4 configuration from a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv4cfg - Location to read IPv4 configration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
int ipcfg_read_text_ipv4(FAR const char *path, FAR const char *netdev,
                         FAR struct ipv4cfg_s *ipv4cfg)
{
  char line[MAX_LINESIZE];
  FAR FILE *stream;
  int index;
  int ret = -ENOENT;

  DEBUGASSERT(path != NULL && netdev != NULL && ipv4cfg != NULL);

  /* Open the file for reading */

  stream = fopen(path, "r");
  if (stream == NULL)
    {
      ret = -errno;
      if (ret != -ENOENT)
        {
          ferr("ERROR: Failed to open %s: %d\n", path, ret);
        }

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
              ferr("ERROR: Skipping malformed line in file: %s\n", line);
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
                  ferr("ERROR: Bad device in file: %s=%s\n",
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
                  ferr("ERROR: Unrecognized IPv4PROTO: %s=%s\n",
                       variable, value);
                }

              /* Assume IPv4 settings are present if the IPv4PROTO
               * setting is encountered.
               */

              ret = OK;
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

          /* Anything other than some IPv6 settings would be an error.
           * This is a sloppy check because it does not detect invalid
           * names variables that begin with "IPv6".
           */

          else if (strncmp(variable, "IPv6", 4) != 0)
            {
              ferr("ERROR: Unrecognized variable: %s=%s\n", variable, value);
            }
        }
    }

  /* Close the file and return */

  fclose(stream);
  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_read_text_ipv6
 *
 * Description:
 *   Read IPv6 configuration from a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv6cfg - Location to read IPv6 configration to
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
int ipcfg_read_text_ipv6(FAR const char *path, FAR const char *netdev,
                         FAR struct ipv6cfg_s *ipv6cfg)
{
  char line[MAX_LINESIZE];
  FAR FILE *stream;
  bool found = false;
  int index;
  int ret;

  DEBUGASSERT(path != NULL && netdev != NULL && ipv6cfg != NULL);

  /* Open the file for reading */

  stream = fopen(path, "r");
  if (stream == NULL)
    {
      ret = -errno;
      if (ret != -ENOENT)
        {
          ferr("ERROR: Failed to open %s: %d\n", path, ret);
        }

      return ret;
    }

  /* Process each line in the file */

  memset(ipv6cfg, 0, sizeof(FAR struct ipv6cfg_s));

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
              ferr("ERROR: Skipping malformed line in file: %s\n", line);
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
                  ferr("ERROR: Bad device in file: %s=%s\n",
                       variable, value);
                }
            }
          else if (strcmp(variable, "IPv6PROTO") == 0)
            {
              if (strcmp(value, "none") == 0)
                {
                  ipv6cfg->proto = IPv6PROTO_NONE;
                }
              else if (strcmp(value, "static") == 0)
                {
                  ipv6cfg->proto = IPv6PROTO_STATIC;
                }
              else if (strcmp(value, "autoconf") == 0)
                {
                  ipv6cfg->proto = IPv6PROTO_AUTOCONF;
                }
              else if (strcmp(value, "fallback") == 0)
                {
                  ipv6cfg->proto = IPv6PROTO_FALLBACK;
                }
              else
                {
                  ferr("ERROR: Unrecognized IPv6PROTO: %s=%s\n",
                       variable, value);
                }

              /* Assume IPv4 settings are present if the IPv6BOOTPROTO
               * setting is encountered.
               */

              found = true;
            }
          else if (strcmp(variable, "IPv6IPADDR") == 0)
            {
              ret = inet_pton(AF_INET6, value, &ipv6cfg->ipaddr);
              if (ret < 0)
                {
                  ret = -errno;
                  ferr("ERROR: inet_pton() failed: %d\n", ret);
                  return ret;
                }
            }
          else if (strcmp(variable, "IPv6NETMASK") == 0)
            {
              ret = inet_pton(AF_INET6, value, &ipv6cfg->netmask);
              if (ret < 0)
                {
                  ret = -errno;
                  ferr("ERROR: inet_pton() failed: %d\n", ret);
                  return ret;
                }
            }
          else if (strcmp(variable, "IPv6ROUTER") == 0)
            {
              ret = inet_pton(AF_INET6, value, &ipv6cfg->router);
              if (ret < 0)
                {
                  ret = -errno;
                  ferr("ERROR: inet_pton() failed: %d\n", ret);
                  return ret;
                }
            }

          /* Anything other than some IPv4 settings would be an error.
           * This is a sloppy check because it does not detect invalid
           * names variables that begin with "IPv4".
           */

          else if (strncmp(variable, "IPv4", 4) != 0)
            {
              ferr("ERROR: Unrecognized variable: %s=%s\n", variable, value);
            }
        }
    }

  /* Close the file and return */

  fclose(stream);
  return found ? OK : -ENOENT;
}
#endif

/****************************************************************************
 * Name: ipcfg_write_text_ipv4
 *
 * Description:
 *   Write the IPv4 configuration to a human-readable, text IP Configuration
 *   file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv4cfg - The IPv4 configration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv4)
int ipcfg_write_text_ipv4(FAR const char *path, FAR const char *netdev,
                          FAR const struct ipv4cfg_s *ipv4cfg)
{
#ifdef CONFIG_NET_IPv6
  struct ipv6cfg_s ipv6cfg;
  bool ipv6 = false;
#endif
  FAR FILE *stream;
  int ret;

  DEBUGASSERT(path != NULL && netdev != NULL && ipv4cfg != NULL);

#ifdef CONFIG_NET_IPv6
  /* Read any IPv6 data in the file */

  ret = ipcfg_read_text_ipv6(path, netdev, &ipv6cfg);
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

  stream = fopen(path, "w");
  if (stream == NULL)
    {
      ret = -errno;
      ferr("ERROR: Failed to open %s: %d\n", path, ret);
      return ret;
    }

  /* Save the device name */

  fprintf(stream, "DEVICE=%s\n", netdev);

  /* Write the IPv4 configuration */

  ret = ipcfg_write_ipv4(stream, ipv4cfg);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_NET_IPv6
  /* Followed by any IPv6 data in the file */

  if (ipv6)
    {
      ret = ipcfg_write_ipv6(stream, &ipv6cfg);
    }
#endif

  fclose(stream);
  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_write_text_ipv6
 *
 * Description:
 *   Write the IPv6 configuration to a binary IP Configuration file.
 *
 * Input Parameters:
 *   path    - The full path to the IP configuration file
 *   netdev  - Network device name string
 *   ipv6cfg - The IPv6 configration to write
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

#if defined(CONFIG_IPCFG_WRITABLE) && defined(CONFIG_NET_IPv6)
int ipcfg_write_text_ipv6(FAR const char *path, FAR const char *netdev,
                          FAR const struct ipv6cfg_s *ipv6cfg)
{
#ifdef CONFIG_NET_IPv4
  struct ipv4cfg_s ipv4cfg;
  bool ipv4 = false;
#endif
  FAR FILE *stream;
  int ret;

  DEBUGASSERT(path != NULL && netdev != NULL && ipv6cfg != NULL);

#ifdef CONFIG_NET_IPv4
  /* Read any IPv4 data in the file */

  ret = ipcfg_read_text_ipv4(path, netdev, &ipv4cfg);
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

  stream = fopen(path, "w");
  if (stream == NULL)
    {
      ret = -errno;
      ferr("ERROR: Failed to open %s: %d\n", path, ret);
      return ret;
    }

  /* Save the device name */

  fprintf(stream, "DEVICE=%s\n", netdev);

#ifdef CONFIG_NET_IPv4
  if (ipv4)
    {
      /* Write the IPv4 configuration.  This is really unnecessary in most
       * cases since the IPv4 data should already be in place.
       */

      ret = ipcfg_write_ipv4(stream, &ipv4cfg);
      if (ret < 0)
        {
          return ret;
        }
    }
#endif

  /* Write the IPv6 configuration */

  ret = ipcfg_write_ipv6(stream, ipv6cfg);
  fclose(stream);
  return ret;
}
#endif

#endif /* !CONFIG_IPCFG_BINARY */
