/************************************************************************************
 * apps/include/graphics/wext.c
 * Driver interaction with generic Wireless Extensions
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Simon Piriou <spiriou31@gmail.com>
 *           Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted to NuttX from driver_ext.c
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
 ************************************************************************************/

/* This file implements a driver interface for the Linux Wireless Extensions.
 * When used with WE-18 or newer, this interface can be used as-is with number
 * of drivers. In addition to this, some of the common functions in this file
 * can be used by other driver interface implementations that use generic WE
 * ioctls, but require private ioctls for some of the functionality.
 */

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <debug.h>

#include "wireless/wext.h"

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: wpa_driver_wext_set_auth_param
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ************************************************************************************/

int wpa_driver_wext_set_auth_param(int sockfd, FAR char *ifname, int idx,
                                   uint32_t value)
{
  struct iwreq iwr;
  int errcode;
  int ret = 0;

  memset(&iwr, 0, sizeof(iwr));
  strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
  iwr.u.param.flags = idx & IW_AUTH_INDEX;
  iwr.u.param.value = value;

  if (ioctl(sockfd, SIOCSIWAUTH, (unsigned long)&iwr) < 0)
    {
      errcode = errno;
      if (errcode != EOPNOTSUPP)
        {
          nerr("ERROR: SIOCSIWAUTH(param %d value 0x%x) failed: %d)",
               idx, value, errcode);
        }

      ret = errcode == EOPNOTSUPP ? -2 : -1;
    }

  return ret;
}

/************************************************************************************
 * Name: wpa_driver_wext_set_ssid
 *
 * Description:
 *   Set SSID, SIOCSIWESSID
 *
 * Input Parameters:
 *   sockfd   - Opened network socket
 *   ifname   - Interface name
 *   ssid     -  SSID
 *   ssid_len - Length of SSID (0..32)
 *
 * Returned Value:
 *   0 on success, -1 on failure
 *
 ************************************************************************************/

int wpa_driver_wext_set_ssid(int sockfd,  FAR char *ifname, FAR const uint8_t *ssid,
                             size_t ssid_len)
{
  struct iwreq iwr;
  int ret = 0;
  char buf[33];

  if (ssid_len > SSID_MAX_LEN)
    {
      return -1;
    }

  memset(&iwr, 0, sizeof(iwr));
  strncpy(iwr.ifr_name, ifname, IFNAMSIZ);

  /* flags: 1 = ESSID is active, 0 = not (promiscuous) */

  iwr.u.essid.flags = (ssid_len != 0);
  memset(buf, 0, sizeof(buf));
  memcpy(buf, ssid, ssid_len);
  iwr.u.essid.pointer = (caddr_t) buf;

  iwr.u.essid.length = ssid_len;

  if (ioctl(sockfd, SIOCSIWESSID, (unsigned long)&iwr) < 0)
    {
      nerr("ERROR: ioctl[SIOCSIWESSID]: %d", errno);
      ret = -1;
    }

  return ret;
}

/************************************************************************************
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
 ************************************************************************************/

int wpa_driver_wext_set_key_ext(int sockfd,  FAR char *ifname, enum wpa_alg alg,
                                FAR const uint8_t *key, size_t key_len)
{
  struct iwreq iwr;
  int ret = 0;
  struct iw_encode_ext *ext;

  ext = malloc(sizeof(*ext) + key_len);
  if (ext == NULL)
    {
      return -1;
    }

  memset(&iwr, 0, sizeof(iwr));
  strncpy(iwr.ifr_name, ifname, IFNAMSIZ);

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

/************************************************************************************
 * Name: wpa_driver_wext_associate
 *
 * Description:
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ************************************************************************************/

int wpa_driver_wext_associate(void)
{
  int ret;
  int sockfd;
  struct iwreq req;

  int sta_mode = IW_MODE_INFRA;

  // IW_AUTH_WPA_VERSION_DISABLED
  // IW_AUTH_WPA_VERSION_WPA

  int auth_wpa = IW_AUTH_WPA_VERSION_WPA2;

  // IW_AUTH_CIPHER_NONE
  // IW_AUTH_CIPHER_WEP40
  // IW_AUTH_CIPHER_TKIP
  // IW_AUTH_CIPHER_WEP104

  int cipher_mode = IW_AUTH_CIPHER_CCMP;

  enum wpa_alg alg = WPA_ALG_CCMP;

  char ifname[] = "wlan0";
  const uint8_t ssid[] = "myApSSID";
  const uint8_t passphrase[] = "mySSIDpassphrase";

  ninfo("Entry\n");

  /* Get a socket (only so that we get access to the INET subsystem) */

  sockfd = socket(PF_INETX, NETLIB_SOCK_IOCTL, 0);
  if (sockfd < 0)
    {
      return sockfd;
    }

  /* Put the driver name into the request */

  strncpy(req.ifr_name, ifname, IFNAMSIZ);

  if (wpa_driver_wext_set_mode(sockfd, ifname, sta_mode) < 0)
    {
      nerr("Fail set sta mode\n");
      ret = -1;
      goto close_socket;
    }

  if (wpa_driver_wext_set_auth_param(sockfd, ifname, IW_AUTH_WPA_VERSION,
                                     auth_wpa) < 0)
    {
      nerr("Fail set wpa version\n");
      ret = -1;
      goto close_socket;
    }

  if (wpa_driver_wext_set_auth_param(sockfd, ifname,
             IW_AUTH_CIPHER_PAIRWISE, cipher_mode) < 0)
  {
    nerr("Fail set cipher mode\n");
    ret = -1;
    goto close_socket;
  }

  if (wpa_driver_wext_set_key_ext(sockfd, ifname, alg, passphrase,
                                  sizeof(passphrase)-1))
    {
      nerr("Fail set key\n");
      ret = -1;
      goto close_socket;
    }

  if (wpa_driver_wext_set_ssid(sockfd, ifname, ssid, sizeof(ssid)-1) < 0)
    {
      nerr("Fail set ssid\n");
      ret = -1;
      goto close_socket;
    }

close_socket:
  close(sockfd);
  return ret;
}

/************************************************************************************
 * Name: wpa_driver_wext_set_mode
 *
 * Description:
 *   Set wireless mode (infra/adhoc), SIOCSIWMODE
 *
 * Input Parameters:
 *   sockfd - Opened network socket
 *   ifname - Interface name
 *   mode   - 0 = infra/BSS (associate with an AP), 1 = adhoc/IBSS
 *
 * Returned Value:
 *   0 on success, -1 on failure
 *
 ************************************************************************************/

int wpa_driver_wext_set_mode(int sockfd, FAR char *ifname, int mode)
{
  struct iwreq iwr;
  int ret = -1;

  memset(&iwr, 0, sizeof(iwr));
  strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
  iwr.u.mode = mode;

  if (ioctl(sockfd, SIOCSIWMODE, (unsigned long)&iwr) == 0)
    {
      ret = 0;
      goto done;
    }

  nerr("ERROR: ioctl[SIOCSIWMODE]: %d", errno);

done:
  return ret;
}
