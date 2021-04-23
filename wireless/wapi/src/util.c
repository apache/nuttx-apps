/****************************************************************************
 * apps/wireless/wapi/src/util.c
 *
 *   Copyright (C) 2011, 2017Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for NuttX from WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of  source code must  retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <netinet/ether.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "wireless/wapi.h"
#include "util.h"

#ifdef CONFIG_WIRELESS_WAPI_INITCONF
#include "netutils/cJSON.h"
#endif /* CONFIG_WIRELESS_WAPI_INITCONF */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Size of the command buffer */

#define WAPI_IOCTL_COMMAND_NAMEBUFSIZ 24

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static char g_ioctl_command_namebuf[WAPI_IOCTL_COMMAND_NAMEBUFSIZ];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_WIRELESS_WAPI_INITCONF
static FAR void *wapi_json_load(FAR const char *confname)
{
  FAR cJSON *root = NULL;
  struct stat sb;
  FAR char *buf;
  int fd = -1;

  if (stat(confname, &sb) < 0)
    {
      return NULL;
    }

  buf = malloc(sb.st_size);
  if (!buf)
    {
      goto errout;
    }

  fd = open(confname, O_RDONLY);
  if (fd < 0)
    {
      goto errout;
    }

  if (read(fd, buf, sb.st_size) != sb.st_size)
    {
      goto errout;
    }

  root = cJSON_Parse(buf);

errout:
  if (buf)
    {
      free(buf);
    }

  if (fd > 0)
    {
      close(fd);
    }

  return root;
}

static bool wapi_json_update(FAR cJSON *root,
                             FAR const char *key,
                             FAR void *value,
                             bool integer)
{
  intptr_t intval = (intptr_t)value;
  FAR cJSON *item;
  FAR cJSON *obj;

  obj = cJSON_GetObjectItem(root, key);
  if (obj)
    {
      if (integer)
        {
          if (intval == obj->valueint)
            {
              return false;
            }

          item = cJSON_CreateNumber(intval);
        }
      else
        {
          int len = strlen(obj->valuestring);
          if (len > 0)
            {
              if (!strncmp(value, obj->valuestring, len))
                {
                  return false;
                }
            }
          else if (len == 0 && value == NULL)
            {
              return false;
            }

          item = cJSON_CreateString(value);
        }

      cJSON_ReplaceItemInObject(root, key, item);
    }
  else
    {
      integer ? cJSON_AddNumberToObject(root, key, intval) :
                cJSON_AddStringToObject(root, key, value);
    }

  return true;
}
#endif /* CONFIG_WIRELESS_WAPI_INITCONF */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_make_socket
 *
 * Description:
 *   Creates an AF_INET socket to be used in ioctl() calls.
 *
 * Returned Value:
 *   Non-negative on success.
 *
 ****************************************************************************/

int wapi_make_socket(void)
{
  return socket(PF_INETX, SOCK_WAPI, 0);
}

/****************************************************************************
 * Name: wapi_ioctl_command_name
 *
 * Description:
 *   Return name string for IOCTL command
 *
 * Returned Value:
 *   Name string for IOCTL command
 *
 ****************************************************************************/

FAR const char *wapi_ioctl_command_name(int cmd)
{
  switch (cmd)
    {
    case SIOCADDRT:
      return "SIOCADDRT";

    case SIOCDELRT:
      return "SIOCDELRT";

    case SIOCGIFADDR:
      return "SIOCGIFADDR";

    case SIOCGIWAP:
      return "SIOCGIWAP";

    case SIOCGIWESSID:
      return "SIOCGIWESSID";

    case SIOCGIWFREQ:
      return "SIOCGIWFREQ";

    case SIOCGIWMODE:
      return "SIOCGIWMODE";

    case SIOCGIWRANGE:
      return "SIOCGIWRANGE";

    case SIOCGIWRATE:
      return "SIOCGIWRATE";

    case SIOCGIWSCAN:
      return "SIOCGIWSCAN";

    case SIOCGIWTXPOW:
      return "SIOCGIWTXPOW";

    case SIOCSIFADDR:
      return "SIOCSIFADDR";

    case SIOCSIWAP:
      return "SIOCSIWAP";

    case SIOCSIWESSID:
      return "SIOCSIWESSID";

    case SIOCSIWFREQ:
      return "SIOCSIWFREQ";

    case SIOCSIWMODE:
      return "SIOCSIWMODE";

    case SIOCSIWRATE:
      return "SIOCSIWRATE";

    case SIOCSIWSCAN:
      return "SIOCSIWSCAN";

    case SIOCSIWTXPOW:
      return "SIOCSIWTXPOW";

    default:
      snprintf(g_ioctl_command_namebuf, WAPI_IOCTL_COMMAND_NAMEBUFSIZ,
               "0x%x", cmd);
      return g_ioctl_command_namebuf;
    }
}

#ifdef CONFIG_WIRELESS_WAPI_INITCONF

/****************************************************************************
 * Name: wapi_unload_config
 *
 * Description:
 *
 * Input Parameters:
 *  load - Config resource handler, allocate by wapi_load_config()
 *
 * Returned Value:
 *
 ****************************************************************************/

void wapi_unload_config(FAR void *load)
{
  if (load)
    {
      cJSON_Delete(load);
    }
}

/****************************************************************************
 * Name: wapi_load_config
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Return a pointer to the hold the config resource, NULL On error.
 *
 ****************************************************************************/

FAR void *wapi_load_config(FAR const char *ifname,
                           FAR const char *confname,
                           FAR struct wpa_wconfig_s *conf)
{
  FAR struct ether_addr *ap;
  FAR cJSON *ifobj;
  FAR cJSON *root;
  FAR cJSON *obj;

  if (ifname == NULL ||
      conf == NULL)
    {
      return NULL;
    }

  if (confname == NULL)
    {
      confname = CONFIG_WIRELESS_WAPI_CONFIG_PATH;
    }

  root = wapi_json_load(confname);
  if (!root)
    {
      return NULL;
    }

  /* Set up the network configuration */

  ifobj = cJSON_GetObjectItem(root, ifname);
  if (!ifobj)
    {
      goto errout;
    }

  memset(conf, 0, sizeof(*conf));

  obj = cJSON_GetObjectItem(ifobj, "mode");
  if (!obj)
    {
      goto errout;
    }

  conf->sta_mode = obj->valueint;

  obj = cJSON_GetObjectItem(ifobj, "auth");
  if (!obj)
    {
      goto errout;
    }

  conf->auth_wpa = obj->valueint;

  obj = cJSON_GetObjectItem(ifobj, "cmode");
  if (!obj)
    {
      goto errout;
    }

  conf->cipher_mode = obj->valueint;

  obj = cJSON_GetObjectItem(ifobj, "alg");
  if (!obj)
    {
      goto errout;
    }

  conf->alg = obj->valueint;

  obj = cJSON_GetObjectItem(ifobj, "ssid");
  if (!obj || !obj->valuestring)
    {
      goto errout;
    }

  conf->ssid = (FAR const char *)obj->valuestring;

  obj = cJSON_GetObjectItem(ifobj, "bssid");
  if (!obj || !obj->valuestring)
    {
      goto errout;
    }

  ap = ether_aton(obj->valuestring);
  if (ap != NULL)
    {
      conf->bssid = (FAR const char *)ap->ether_addr_octet;
    }

  obj = cJSON_GetObjectItem(ifobj, "psk");
  if (!obj || !obj->valuestring)
    {
      goto errout;
    }

  conf->passphrase = (FAR const char *)obj->valuestring;

  conf->ifname     = ifname;
  conf->ssidlen    = strlen(conf->ssid);
  conf->phraselen  = conf->passphrase ? strlen(conf->passphrase) : 0;

  return root;

errout:
  cJSON_Delete(root);

  return NULL;
}

/****************************************************************************
 * Name: wapi_save_config
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int wapi_save_config(FAR const char *ifname,
                     FAR const char *confname,
                     FAR const struct wpa_wconfig_s *conf)
{
  FAR char *buf = NULL;
  FAR cJSON *ifobj;
  FAR cJSON *root;
  int ret = -1;
  int fd = -1;
  bool update;

  if (ifname == NULL || conf == NULL)
    {
      return ret;
    }

  if (confname == NULL)
    {
      confname = CONFIG_WIRELESS_WAPI_CONFIG_PATH;
    }

  root = wapi_json_load(confname);
  if (!root)
    {
      root = cJSON_CreateObject();
      if (root == NULL)
        {
          return ret;
        }
    }

  ifobj = cJSON_GetObjectItem(root, ifname);
  if (!ifobj)
    {
      ifobj = cJSON_CreateObject();
      if (ifobj == NULL)
        {
          goto errout;
        }

      cJSON_AddItemToObject(root, ifname, ifobj);
    }

  update =  wapi_json_update(ifobj, "mode",
                             (FAR void *)(intptr_t)conf->sta_mode, true);
  update |= wapi_json_update(ifobj, "auth",
                             (FAR void *)(intptr_t)conf->auth_wpa, true);
  update |= wapi_json_update(ifobj, "cmode",
                             (FAR void *)(intptr_t)conf->cipher_mode, true);
  update |= wapi_json_update(ifobj, "alg",
                             (FAR void *)(intptr_t)conf->alg, true);
  update |= wapi_json_update(ifobj, "ssid",
                             (FAR void *)conf->ssid, false);
  update |= wapi_json_update(ifobj, "bssid",
                             (FAR void *)conf->bssid, false);
  update |= wapi_json_update(ifobj, "psk",
                             (FAR void *)conf->passphrase, false);

  if (!update)
    {
      ret = OK;
      goto errout;
    }

  buf = cJSON_PrintUnformatted(root);
  if (!buf)
    {
      goto errout;
    }

  fd = open(confname, O_RDWR | O_CREAT | O_TRUNC);
  if (fd < 0)
    {
      goto errout;
    }

  ret = write(fd, buf, strlen(buf));

errout:
  if (buf)
    {
      free(buf);
    }

  if (fd > 0)
    {
      close(fd);
    }

  cJSON_Delete(root);

  return ret < 0 ? ret : OK;
}
#endif /* CONFIG_WIRELESS_WAPI_INITCONF */
