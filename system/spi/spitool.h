/****************************************************************************
 * apps/system/spi/spitool.h
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Dave Marples <dave@marples.net>
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

#ifndef __APPS_SYSTEM_SPI_SPITOOLS_H
#define __APPS_SYSTEM_SPI_SPITOOLS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <nuttx/spi/spi_transfer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* CONFIG_NSH_BUILTIN_APPS - Build the tools as an NSH built-in command
 * CONFIG_SPITOOL_MINBUS   - Smallest bus index supported by the hardware
 *                           (default 0).
 * CONFIG_SPITOOL_MAXBUS   - Largest bus index supported by the hardware
 *                           (default 3)
 * CONFIG_SPITOOL_DEFFREQ  - Default frequency (default: 40000000)
 * CONFIG_SPITOOL_DEFMODE  - Default mode, where;
 *                             0 = CPOL=0, CHPHA=0
 *                             1 = CPOL=0, CHPHA=1
 *                             2 = CPOL=1, CHPHA=0
 *                             3 = CPOL=1, CHPHA=1
 * CONFIG_SPITOOL_DEFWIDTH - Default bit width (default 8)
 * CONFIG_SPITOOL_DEFWORDS - Default number of words to exchange
 *                           (default 1)
 */

#ifndef CONFIG_SPITOOL_MINBUS
#  define CONFIG_SPITOOL_MINBUS 0
#endif

#ifndef CONFIG_SPITOOL_MAXBUS
#  define CONFIG_SPITOOL_MAXBUS 3
#endif

#ifndef CONFIG_SPITOOL_DEFFREQ
#  define CONFIG_SPITOOL_DEFFREQ 4000000
#endif

#ifndef CONFIG_SPITOOL_DEFMODE
#define  CONFIG_SPITOOL_DEFMODE  0 /* CPOL=0, CHPHA=0 */
#endif

#ifndef CONFIG_SPITOOL_DEFWIDTH
#define  CONFIG_SPITOOL_DEFWIDTH 8
#endif

#ifndef CONFIG_SPITOOL_DEFWORDS
#define  CONFIG_SPITOOL_DEFWORDS 1
#endif

/* This is the maximum number of arguments that will be accepted for a
 * command.  This is only used for sizing a hardcoded array and is set
 * to be sufficiently large to support all possible SPI tool arguments and
 * then some.
 */

#define MAX_ARGUMENTS 12

/* Maximum size of one command line */

#define MAX_LINELEN 80
#define MAX_XDATA  (MAX_LINELEN/2)

/* Are we using the NuttX console for I/O?  Or some other character device? */

#ifdef CONFIG_SPITOOL_INDEV
#  define INFD(p)      ((p)->ss_infd)
#  define INSTREAM(p)  ((p)->ss_instream)
#else
#  define INFD(p)      0
#  define INSTREAM(p)  stdin
#endif

#ifdef CONFIG_SPITOOL_OUTDEV
#  define OUTFD(p)     ((p)->ss_outfd)
#  define OUTSTREAM(p) ((p)->ss_outstream)
#else
#  define OUTFD(p)     1
#  define OUTSTREAM(p) stdout
#endif

/* Output is via printf but can be changed using this macro */

# define spi_output         printf

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct spitool_s
{
  /* Sticky options */

  uint8_t  bus;        /* [-b bus] is the SPI bus number           */
  uint8_t  width;      /* [-w width] is the data width (8 or 16)   */
  uint32_t freq;       /* [-f freq] SPI frequency                  */
  uint32_t count;      /* [-x count] No of words to exchange       */
  uint32_t csn;        /* [-n CSn] Chip select number for devtype  */
  uint32_t devtype;    /* [-t devtype] DevType (see spi_devtype_e) */
  bool command;        /* [-c 0|1] Send as command or data?        */
  useconds_t udelay;   /* [-u udelay] Delay in uS after transfer   */
  uint8_t mode;        /* [-m mode] Mode to use for transfer       */

  /* Output streams */

#ifdef CONFIG_SPITOOL_OUTDEV
  int    ss_outfd;     /* Output file descriptor */
  FILE  *ss_outstream; /* Output stream */
#endif
};

typedef int (*cmd_t)(FAR struct spitool_s *spitool, int argc,
                     FAR char **argv);

struct cmdmap_s
{
  FAR const char *cmd;        /* Name of the command */
  cmd_t           handler;    /* Function that handles the command */
  FAR const char *desc;       /* Short description */
  FAR const char *usage;      /* Usage instructions for 'help' command */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char g_spiargrequired[];
extern const char g_spiarginvalid[];
extern const char g_spiargrange[];
extern const char g_spiincompleteparam[];
extern const char g_spicmdnotfound[];
extern const char g_spitoomanyargs[];
extern const char g_spicmdfailed[];
extern const char g_spixfrerror[];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Message handler */

ssize_t spitool_exchange(FAR struct spitool_s *spitool,
                         FAR const void *outbuffer, size_t noutbytes,
                         FAR void *inbuffer, size_t ninbytes);
int spitool_printf(FAR struct spitool_s *spitool, const char *fmt, ...);
void spitool_flush(FAR struct spitool_s *spitool);

/* Command handlers */

int spicmd_bus(FAR struct spitool_s *spitool, int argc, FAR char **argv);
int spicmd_exch(FAR struct spitool_s *spitool, int argc, FAR char **argv);

/* Common logic */

int spitool_common_args(FAR struct spitool_s *spitool, FAR char **arg);

/* Driver access utilities */

FAR char *spidev_path(int bus);
bool spidev_exists(int bus);
int spidev_open(int bus);
int spidev_transfer(int fd, FAR struct spi_sequence_s *seq);

#endif /* __APPS_SYSTEM_SPI_SPITOOLS_H */
