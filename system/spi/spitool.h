/****************************************************************************
 * apps/system/spi/spitool.h
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
 *                             0 = CPOL=0, CPHA=0
 *                             1 = CPOL=0, CPHA=1
 *                             2 = CPOL=1, CPHA=0
 *                             3 = CPOL=1, CPHA=1
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
#define  CONFIG_SPITOOL_DEFMODE  0 /* CPOL=0, CPHA=0 */
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
int spitool_printf(FAR struct spitool_s *spitool, const char *fmt, ...)
    printflike(2, 3);
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
