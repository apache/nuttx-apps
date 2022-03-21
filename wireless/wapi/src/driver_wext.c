/****************************************************************************
 * apps/wireless/wapi/src/driver_wext.c
 * Driver interaction with generic Wireless Extensions
 *
 *   Copyright (C) 2017, 2019 Gregory Nutt. All rights reserved.
 *   Author: Simon Piriou <spiriou31@gmail.com>
 *           Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for NuttX from the driver_ext.c of WPA suplicant written
 * originally by Jouni Malinen
 *
 *   Copyright (c) 2003-2015, Jouni Malinen <j@w1.fi>
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

/* This file implements a driver interface for the Linux Wireless Extensions.
 * When used with WE-18 or newer, this interface can be used as-is with
 * number of drivers. In addition to this, some of the common functions
 * in this file can be used by other driver interface implementations that
 * use generic WE ioctls, but require private ioctls for some of the
 * functionality.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include <debug.h>

#include "wireless/wapi.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wpa_driver_wext_get_key_ext
 *
 * Description:
 *
 * Input Parameters:
 *   sockfd - Opened network socket
 *   ifname - Interface name
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_get_key_ext(int sockfd, FAR const char *ifname,
                                enum wpa_alg_e *alg, FAR char *key,
                                size_t *req_len)
{
  struct iw_encode_ext *ext;
  struct iwreq iwr;
  int ret;

  ext = malloc(sizeof(*ext) + *req_len);
  if (ext == NULL)
    {
      return -1;
    }

  memset(&iwr, 0, sizeof(iwr));
  strlcpy(iwr.ifr_name, ifname, IFNAMSIZ);

  iwr.u.encoding.pointer = (caddr_t) ext;
  iwr.u.encoding.length = sizeof(*ext) + *req_len;

  ret = ioctl(sockfd, SIOCGIWENCODEEXT, (unsigned long)&iwr);
  if (ret >= 0)
    {
      switch (ext->alg)
        {
          case IW_ENCODE_ALG_NONE:
            *alg = WPA_ALG_NONE;
            break;

          case IW_ENCODE_ALG_WEP:
            *alg = WPA_ALG_WEP;
            break;

          case IW_ENCODE_ALG_TKIP:
            *alg = WPA_ALG_TKIP;
            break;

          case IW_ENCODE_ALG_CCMP:
            *alg = WPA_ALG_CCMP;
            break;

          default:
            free(ext);
            return -1;
        }

     if (key && ext->key_len < *req_len)
       {
         memcpy(key, ext->key, ext->key_len);
         *req_len = ext->key_len;
       }
  }

  free(ext);
  return ret;
}

/****************************************************************************
 * Name: wpa_driver_wext_set_key_ext
 *
 * Description:
 *
 * Input Parameters:
 *   sockfd - Opened network socket
 *   ifname - Interface name
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_set_key_ext(int sockfd, FAR const char *ifname,
                                enum wpa_alg_e alg, FAR const char *key,
                                size_t key_len)
{
  struct iwreq iwr;
  int ret = 0;
  struct iw_encode_ext *ext;

  DEBUGASSERT(ifname != NULL && key != NULL && key_len > 0);

  ext = malloc(sizeof(*ext) + key_len);
  if (ext == NULL)
    {
      return -1;
    }

  memset(&iwr, 0, sizeof(iwr));
  strlcpy(iwr.ifr_name, ifname, IFNAMSIZ);

  iwr.u.encoding.pointer = (caddr_t) ext;
  iwr.u.encoding.length = sizeof(*ext) + key_len;

  if (key && key_len)
    {
      memcpy(ext + 1, key, key_len);
      ext->key_len = key_len;
    }

  switch (alg)
    {
      case WPA_ALG_NONE:
        ext->alg = IW_ENCODE_ALG_NONE;
        break;

      case WPA_ALG_WEP:
        ext->alg = IW_ENCODE_ALG_WEP;
        break;

      case WPA_ALG_TKIP:
        ext->alg = IW_ENCODE_ALG_TKIP;
        break;

      case WPA_ALG_CCMP:
        ext->alg = IW_ENCODE_ALG_CCMP;
        break;

      default:
        nerr("ERROR: Unknown algorithm %d", alg);
        free(ext);
        return -1;
    }

  if (ioctl(sockfd, SIOCSIWENCODEEXT, (unsigned long)&iwr) < 0)
    {
      ret = errno == EOPNOTSUPP ? -2 : -1;
      nerr("ERROR: ioctl[SIOCSIWENCODEEXT]: %d", errno);
    }

  free(ext);
  return ret;
}

/****************************************************************************
 * Name: wpa_driver_wext_associate
 *
 * Description:
 *
 * Input Parameters:
 *   wconfig - Describes the wireless configuration.
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_associate(FAR struct wpa_wconfig_s *wconfig)
{
  struct iwreq req;
  int sockfd;
  int ret;

  DEBUGASSERT(wconfig != NULL);

  ninfo("sta_mode=%u auth_wpa=%08x cipher_mode=%08x alg=%d\n",
        wconfig->sta_mode, wconfig->auth_wpa, wconfig->cipher_mode,
        wconfig->alg);
  ninfo("ifname=%s ssid[%u]=%s passphrase[%u]=%s\n",
        wconfig->ifname, wconfig->ssidlen, wconfig->ssid, wconfig->phraselen,
        wconfig->passphrase);

  /* Get a socket (only so that we get access to the INET subsystem) */

  sockfd = wapi_make_socket();
  if (sockfd < 0)
    {
      return sockfd;
    }

  /* Put the driver name into the request */

  strlcpy(req.ifr_name, wconfig->ifname, IFNAMSIZ);

  ret = wapi_set_mode(sockfd, wconfig->ifname, wconfig->sta_mode);
  if (ret < 0)
    {
      nerr("ERROR: Fail set sta mode: %d\n", ret);
      goto close_socket;
    }

  ret = wpa_driver_wext_set_auth_param(sockfd, wconfig->ifname,
                                       IW_AUTH_WPA_VERSION,
                                       wconfig->auth_wpa);
  if (ret < 0)
    {
      nerr("ERROR: Fail set wpa version: %d\n", ret);
      goto close_socket;
    }

  ret = wpa_driver_wext_set_auth_param(sockfd, wconfig->ifname,
                                       IW_AUTH_CIPHER_PAIRWISE,
                                       wconfig->cipher_mode);
  if (ret < 0)
    {
      nerr("ERROR: Fail set cipher mode: %d\n", ret);
      goto close_socket;
    }

  if (wconfig->freq)
    {
      ret = wapi_set_freq(sockfd, wconfig->ifname,
                          wconfig->freq,
                          wconfig->flag == WAPI_FREQ_FIXED ?
                          IW_FREQ_FIXED : IW_FREQ_AUTO);
      if (ret < 0)
        {
          nerr("WARNING: Fail set freq: %d\n", ret);
        }
    }

  if (wconfig->phraselen > 0)
    {
      ret = wpa_driver_wext_set_key_ext(sockfd, wconfig->ifname,
                                        wconfig->alg,
                                        wconfig->passphrase,
                                        wconfig->phraselen);
      if (ret < 0)
        {
          nerr("ERROR: Fail set key: %d\n", ret);
          goto close_socket;
        }
    }

  if (wconfig->ssid)
    {
      ret = wapi_set_essid(sockfd, wconfig->ifname,
                           wconfig->ssid, WAPI_ESSID_ON);
      if (ret < 0)
        {
          nerr("ERROR: Fail set ssid: %d\n", ret);
          goto close_socket;
        }
    }
  else if (wconfig->bssid)
    {
      ret = wapi_set_ap(sockfd, wconfig->ifname,
                        (FAR const struct ether_addr *)wconfig->bssid);
      if (ret < 0)
        {
          nerr("ERROR: Fail set bssid: %d\n", ret);
        }
    }

close_socket:
  close(sockfd);
  return ret;
}

/****************************************************************************
 * Name: wpa_driver_wext_set_auth_param
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

static int wpa_driver_wext_process_auth_param(int sockfd,
                                              FAR const char *ifname,
                                              int idx,
                                              uint32_t *value,
                                              bool set)
{
  struct iwreq iwr;
  int errcode;
  int ret = 0;

  DEBUGASSERT(ifname != NULL);

  memset(&iwr, 0, sizeof(iwr));
  strlcpy(iwr.ifr_name, ifname, IFNAMSIZ);
  iwr.u.param.flags = idx & IW_AUTH_INDEX;
  iwr.u.param.value = set ? *value : 0;

  if (ioctl(sockfd, set ? SIOCSIWAUTH : SIOCGIWAUTH,
            (unsigned long)&iwr) < 0)
    {
      errcode = errno;
      if (errcode != EOPNOTSUPP)
        {
          nerr("ERROR: SIOCSIWAUTH(param %d value 0x%" PRIx32
               ") failed: %d)",
               idx, *value, errcode);
        }

      ret = errcode == EOPNOTSUPP ? -2 : -1;
    }

  if (ret == 0 && !set)
    {
      *value = iwr.u.param.value;
    }

  return ret;
}

/****************************************************************************
 * Name: wpa_driver_wext_set_auth_param
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_set_auth_param(int sockfd, FAR const char *ifname,
                                   int idx, uint32_t value)
{
  return wpa_driver_wext_process_auth_param(sockfd, ifname,
                                            idx, &value, true);
}

/****************************************************************************
 * Name: wpa_driver_wext_get_auth_param
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

int wpa_driver_wext_get_auth_param(int sockfd, FAR const char *ifname,
                                   int idx, uint32_t *value)
{
  return wpa_driver_wext_process_auth_param(sockfd, ifname,
                                            idx, value, false);
}

/****************************************************************************
 * Name: wpa_driver_wext_disconnect
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

void wpa_driver_wext_disconnect(int sockfd, FAR const char *ifname)
{
  uint8_t ssid[WAPI_ESSID_MAX_SIZE];
  const struct ether_addr bssid =
  {
  };

  struct iwreq iwr;
  int i;

  /* Only force-disconnect when the card is in infrastructure mode,
   * otherwise the driver might interpret the cleared BSSID and random
   * SSID as an attempt to create a new ad-hoc network.
   */

  memset(&iwr, 0, sizeof(iwr));
  strlcpy(iwr.ifr_name, ifname, IFNAMSIZ);

  if (ioctl(sockfd, SIOCGIWMODE, (unsigned long)&iwr) < 0)
    {
      nerr("ioctl[SIOCGIWMODE]: %s\n", strerror(errno));
      iwr.u.mode = IW_MODE_INFRA;
    }

  if (iwr.u.mode == IW_MODE_INFRA)
    {
      /* Clear the BSSID selection */

      if (wapi_set_ap(sockfd, ifname, &bssid) < 0)
        {
          nerr("WEXT: Failed to clear BSSID " "selection on disconnect\n");
        }

      /* Set a random SSID to make sure the driver will not be trying
       * to associate with something even if it does not understand
       * SIOCSIWMLME commands (or tries to associate automatically
       * after deauth/disassoc).
       */

      for (i = 0; i < WAPI_ESSID_MAX_SIZE; i++)
        {
          ssid[i] = rand() & 0xff;
        }

      if (wapi_set_essid(sockfd, ifname,
                         (FAR const char *)ssid, WAPI_ESSID_OFF) < 0)
        {
          nerr("WEXT: Failed to set bogus " "SSID to disconnect\n");
        }
    }
}
