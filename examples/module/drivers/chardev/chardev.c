/****************************************************************************
 * apps/examples/module/drivers/chardev/chardev.c
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
  NULL,          /* open */
  NULL,          /* close */
  chardev_read,  /* read */
  chardev_write, /* write */
  NULL,          /* seek */
  NULL,          /* ioctl */
  NULL           /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  , NULL         /* unlink */
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
