/****************************************************************************
 * apps/system/i2c/i2ctool.h
 *
 *   Copyright (C) 2011, 2014, 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#ifndef __APPS_SYSTEM_I2C_I2CTOOLS_H
#define __APPS_SYSTEM_I2C_I2CTOOLS_H

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

#include <nuttx/i2c/i2c_master.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* CONFIG_I2CTOOL_MINBUS     - Smallest bus index supported by the hardware
 *                             (default 0).
 * CONFIG_I2CTOOL_MAXBUS     - Largest bus index supported by the hardware
 *                             (default 3)
 * CONFIG_I2CTOOL_MINADDR    - Minimum device address (default: 0x03)
 * CONFIG_I2CTOOL_MAXADDR    - Largest device address (default: 0x77)
 * CONFIG_I2CTOOL_MAXREGADDR - Largest register address (default: 0xff)
 * CONFIG_I2CTOOL_DEFFREQ    - Default frequency (default: 4000000)
 */

#ifndef CONFIG_I2CTOOL_MINBUS
#  define CONFIG_I2CTOOL_MINBUS 0
#endif

#ifndef CONFIG_I2CTOOL_MAXBUS
#  define CONFIG_I2CTOOL_MAXBUS 3
#endif

#ifndef CONFIG_I2CTOOL_MINADDR
#  define CONFIG_I2CTOOL_MINADDR 0x03
#endif

#ifndef CONFIG_I2CTOOL_MAXADDR
#  define CONFIG_I2CTOOL_MAXADDR 0x77
#endif

#ifndef CONFIG_I2CTOOL_MAXREGADDR
#  define CONFIG_I2CTOOL_MAXREGADDR 0xff
#endif

#ifndef CONFIG_I2CTOOL_DEFFREQ
#  define CONFIG_I2CTOOL_DEFFREQ 100000
#endif

/* This is the maximum number of arguments that will be accepted for a
 * command.  This is only used for sizing a hardcoded array and is set
 * to be sufficiently large to support all possible I2C tool arguments and
 * then some.
 */

#define MAX_ARGUMENTS 12

/* Maximum number of bytes to dump */

#define MAX_DUMP_CNT  256

/* Maximum size of one command line */

#define MAX_LINELEN 80

/* Are we using the NuttX console for I/O?  Or some other character device? */

#ifdef CONFIG_I2CTOOL_INDEV
#  define INFD(p)      ((p)->ss_infd)
#  define INSTREAM(p)  ((p)->ss_instream)
#else
#  define INFD(p)      0
#  define INSTREAM(p)  stdin
#endif

#ifdef CONFIG_I2CTOOL_OUTDEV
#  define OUTFD(p)     ((p)->ss_outfd)
#  define OUTSTREAM(p) ((p)->ss_outstream)
#else
#  define OUTFD(p)     1
#  define OUTSTREAM(p) stdout
#endif

/* Output is via printf but can be changed using this macro */

# define i2c_output         printf

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct i2ctool_s
{
  /* Sticky options */

  uint8_t  addr;       /* [-a addr] is the I2C device address */
  uint8_t  bus;        /* [-b bus] is the I2C bus number */
  uint8_t  regaddr;    /* [-r regaddr] is the I2C device register address */
  uint8_t  width;      /* [-w width] is the data width (8 or 16) */
  bool     start;      /* [-s|n], send|don't send start between command and data */
  bool     autoincr;   /* [-i|j], Auto increment|don't increment regaddr on repetitions */
  bool     hasregindx; /* true with the use of -r */
  uint32_t freq;       /* [-f freq] I2C frequency */

  /* Output streams */

#ifdef CONFIG_I2CTOOL_OUTDEV
  int    ss_outfd;     /* Output file descriptor */
  FILE  *ss_outstream; /* Output stream */
#endif
};

typedef int  (*cmd_t)(FAR struct i2ctool_s *i2ctool, int argc,
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

extern const char g_i2cargrequired[];
extern const char g_i2carginvalid[];
extern const char g_i2cargrange[];
extern const char g_i2ccmdnotfound[];
extern const char g_i2ctoomanyargs[];
extern const char g_i2ccmdfailed[];
extern const char g_i2cxfrerror[];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Message handler */

ssize_t i2ctool_write(FAR struct i2ctool_s *i2ctool, FAR const void *buffer,
                      size_t nbytes);
int i2ctool_printf(FAR struct i2ctool_s *i2ctool, const char *fmt, ...);
void i2ctool_flush(FAR struct i2ctool_s *i2ctool);
void i2ctool_hexdump(FILE *outstream, void *addr, int len);

/* Command handlers */

int i2ccmd_bus(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv);
int i2ccmd_dev(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv);
int i2ccmd_get(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv);
int i2ccmd_dump(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv);
int i2ccmd_set(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv);
int i2ccmd_verf(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv);

#ifdef CONFIG_I2C_RESET
int i2ccmd_reset(FAR struct i2ctool_s *i2ctool, int argc, FAR char **argv);
#endif

/* I2C access functions */

int i2ctool_get(FAR struct i2ctool_s *i2ctool, int fd, uint8_t regaddr,
                FAR uint16_t *result);
int i2ctool_set(FAR struct i2ctool_s *i2ctool, int fd, uint8_t regaddr,
                uint16_t value);

/* Common logic */

int i2ctool_common_args(FAR struct i2ctool_s *i2ctool, FAR char **arg);

/* Driver access utilities */

FAR char *i2cdev_path(int bus);
bool i2cdev_exists(int bus);
int i2cdev_open(int bus);
int i2cdev_transfer(int fd, FAR struct i2c_msg_s *msgv, int msgc);

#ifdef CONFIG_I2C_RESET
int i2cdev_reset(int fd);
#endif

#endif /* __APPS_SYSTEM_I2C_I2CTOOLS_H */
