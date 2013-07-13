/****************************************************************************
 * apps/include/zmodem.h
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * References:
 *   "The ZMODEM Inter Application File Transfer Protocol", Chuck Forsberg,
 *    Omen Technology Inc., October 14, 1988
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

#ifndef __APPS_INCLUDE_ZMODEM_H
#define __APPS_INCLUDE_ZMODEM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

/* The default device used by the Zmodem commands if the -d option is not
 * provided on the sz or rz command line.
 */

#ifndef CONFIG_SYSTEM_ZMODEM_DEVNAME
#  define CONFIG_SYSTEM_ZMODEM_DEVNAME "/dev/console"
#endif

/* The size of one buffer used to read data from the remote peer */

#ifndef CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE
#  define CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE 512
#endif

/* Data may be received in gulps of varying size and alignment.  Received
 * packets data is properly packed into a packet buffer of this size.
 */

#ifndef CONFIG_SYSTEM_ZMODEM_PKTBUFSIZE
#  define CONFIG_SYSTEM_ZMODEM_PKTBUFSIZE 512
#endif

/* The size of one transmit buffer used for composing messages sent to the
 * remote peer.
 */

#ifndef CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE
#  define CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE 512
#endif

/* Absolute pathes are not accepted.  This configuration value must be
 * set to provide the path to the file storage directory (such as a
 * mountpoint directory).
 */

#ifndef CONFIG_SYSTEM_ZMODEM_MOUNTPOINT
#  define CONFIG_SYSTEM_ZMODEM_MOUNTPOINT "/tmp"
#endif

/* Response time for sender to respond to requests */

#ifndef CONFIG_SYSTEM_ZMODEM_RESPTIME
#  define CONFIG_SYSTEM_ZMODEM_RESPTIME 10
#endif

/* Receiver serial number */

#ifndef CONFIG_SYSTEM_ZMODEM_SERIALNO
#  define CONFIG_SYSTEM_ZMODEM_SERIALNO 1
#endif

/* Max receive errors before canceling the transfer */

#ifndef CONFIG_SYSTEM_ZMODEM_MAXERRORS
#  define CONFIG_SYSTEM_ZMODEM_MAXERRORS 20
#endif

#ifndef CONFIG_SYSTEM_ZMODEM_MOUNTPOINT
#  define CONFIG_SYSTEM_ZMODEM_MOUNTPOINT "/tmp"
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/
/* Opaque handles returned by initialization functions */

typedef FAR void *ZMHANDLE;
typedef FAR void *ZMRHANDLE;
typedef FAR void *ZMSHANDLE;

/* Enumerations describing arguments to zms_initialize() */

enum zm_xfertype_e
{
  XM_XFERTYPE_NORMAL = 0,    /* Normal file transfer */
  XM_XFERTYPE_BINARY = 1,    /* Binary transfer */
  XM_XFERTYPE_ASCII  = 2,    /* Convert \n to local EOL convention */
  XM_XFERTYPE_RESUME = 3     /* Resume interrupted transfer or append to file. */
};

enum zm_option_e
{
  XM_OPTION_NONE     = 0,    /* Transfer if source newer or longer */
  XM_OPTION_NEWL     = 1,    /* Transfer if source newer or longer */
  XM_OPTION_CRC      = 2,    /* Transfer if different CRC or length */
  XM_OPTION_APPEND   = 3,    /* Append to existing file, if any */
  XM_OPTION_REPLACE  = 4,    /* Replace existing file */
  XM_OPTION_NEW      = 5,    /* Transfer if source is newer */
  XM_OPTION_DIFF     = 6,    /* Transfer if dates or lengths different */
  XM_OPTION_CREATE   = 7,    /* Protect: transfer only if dest doesn't exist */
  XM_OPTION_RENAME   = 8     /* Change filename if destination exists */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: zmr_initialize
 *
 * Description:
 *   Initialize for Zmodem receive operation
 *
 * Input Parameters:
 *   remfd - The R/W file/socket descriptor to use for communication with the
 *      remote peer.
 *
 * Returned Value:
 *   An opaque handle that can be use with zmr_receive() and zmr_release().
 *
 ****************************************************************************/

ZMRHANDLE zmr_initialize(int remfd);

/****************************************************************************
 * Name: zmr_receive
 *
 * Description:
 *   Receive file(s) sent from the remote peer.
 *
 * Input Parameters:
 *   handle - The handler created by zmr_initialize().
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int zmr_receive(ZMRHANDLE handle);

/****************************************************************************
 * Name: zmr_release
 *
 * Description:
 *   Called by the user when there are no more files to receive.
 *
 * Input Parameters:
 *   handle - The handler created by zmr_initialize().
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int zmr_release(ZMRHANDLE);

/****************************************************************************
 * Name: zms_initialize
 *
 * Description:
 *   Initialize for Zmodem send operation.  The overall usage is as follows:
 *
 *   1) Call zms_initialize() to create the partially initialized struct
 *      zm_state_s instance.
 *   2) Make any custom settings in the struct zms_state_s instance.
 *   3) Create a stream instance to get the "file" to transmit and add
 *      the filename to pzms
 *   4) Call zms_upload() to transfer the file.
 *   5) Repeat 3) and 4) to transfer as many files as desired.
 *   6) Call zms_release() when the final file has been transferred.
 *
 * Input Parameters:
 *   remfd - The R/W file/socket descriptor to use for communication with the
 *      remote peer.
 *
 * Returned Value:
 *   An opaque handle that can be use with zmx_send() and zms_release()
 *
 ****************************************************************************/

ZMSHANDLE zms_initialize(int remfd);

/****************************************************************************
 * Name: zms_send
 *
 * Description:
 *   Send a file.
 *
 * Input Parameters:
 *   handle    - Handle previoulsy returned by xms_initialize()
 *   filename  - The name of the local file to transfer
 *   rfilename - The name of the remote file name to create
 *   option    - Describes optional transfer behavior
 *   f1        - The F1 transfer flags
 *   skip      - True:  Skip if file not present at receiving end.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int zms_send(ZMSHANDLE handle, FAR const char *filename,
             FAR const char *rfilename, enum zm_xfertype_e xfertype,
             enum zm_option_e option,  bool skip);

/****************************************************************************
 * Name: zms_release
 *
 * Description:
 *   Called by the user when there are no more files to send.
 *
 * Input Parameters:
 *   handle - The handler created by zms_initialize().
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int zms_release(ZMSHANDLE handle);

/****************************************************************************
 * Name: zms_hwflowcontrol
 *
 * Description:
 *   If CONFIG_SYSTEM_ZMODEM_FULLSTREAMING is defined, then the system
 *   must provide the following interface in order to enable/disable hardware
 *   flow control on the device used to communicate with the remote peer.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_ZMODEM_FULLSTREAMING
int zms_hwflowcontrol(int remfd, bool enable);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_ZMODEM_H */
