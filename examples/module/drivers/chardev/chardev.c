/****************************************************************************
 * apps/examples/module/drivers/chardev/chardev.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <debug.h>

#include <nuttx/module.h>
#include <nuttx/lib/modlib.h>
#include <nuttx/fs/fs.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/****************************************************************************
 * Private data
 ****************************************************************************/

static const char g_read_string[] = "Hi there, apps/examples/module test\n";

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t chardev_read(FAR struct file *filep, FAR char *buffer,
                 size_t buflen);
static ssize_t chardev_write(FAR struct file *filep, FAR const char *buffer,
                 size_t buflen);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations chardev_fops =
{
  0,             /* open */
  0,             /* close */
  chardev_read,  /* read */
  chardev_write, /* write */
  0,             /* seek */
  0              /* ioctl */
#ifndef CONFIG_DISABLE_POLL
  , 0            /* poll */
#endif
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  , 0            /* unlink */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: chardev_read
 ****************************************************************************/

static ssize_t chardev_read(FAR struct file *filep, FAR char *buffer,
                            size_t len)
{
  size_t rdlen = strlen(g_read_string);
  ssize_t ret = MIN(len, rdlen);

  memcpy(buffer, g_read_string, ret);

  syslog(LOG_INFO, "chardev_read: Returning %d bytes\n", (int)ret);
  lib_dumpbuffer("chardev_read: Returning", (FAR const uint8_t *)buffer, ret);
  return ret;
}

/****************************************************************************
 * Name: chardev_write
 ****************************************************************************/

static ssize_t chardev_write(FAR struct file *filep, FAR const char *buffer,
                             size_t len)
{
  syslog(LOG_INFO, "chardev_write: Writing %d bytes\n", (int)len);
  lib_dumpbuffer("chardev_write: Writing", (FAR const uint8_t *)buffer, len);
  return len;
}

/****************************************************************************
 * Name: module_uninitialize
 ****************************************************************************/

static int module_uninitialize(FAR void *arg)
{
  /* TODO: Check if there are any open references to the driver */

  syslog(LOG_INFO, "module_uninitialize: arg=%p\n", arg);
  return unregister_driver("/dev/chardev");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: module_initialize
 *
 * Description:
 *   Register /dev/chardev
 *
 ****************************************************************************/

int module_initialize(FAR struct mod_info_s *modinfo)
{
  syslog(LOG_INFO, "module_initialize:\n");

  modinfo->uninitializer = module_uninitialize;
  modinfo->arg           = NULL;
  modinfo->exports       = NULL;
  modinfo->nexports      = 0;

  return register_driver("/dev/chardev", &chardev_fops, 0666, NULL);
}
