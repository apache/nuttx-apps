/****************************************************************************
 * apps/platform/mikroe-stm32f4/mikroe_configdata.c
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
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <sys/ioctl.h>

#include "platform/configdata.h"
#include <nuttx/fs/ioctl.h>
#include <nuttx/mtd/configdata.h>

#ifdef CONFIG_PLATFORM_CONFIGDATA

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: platform_setconfig
 *
 * Description:
 *   Save platform-specific configuration data
 *
 * Input Parameter:
 *   id         - Defines the class of configuration data
 *   instance   - Defines which instance of configuration data.  For example,
 *                if a board has two networks, then there would be two MAC
 *                addresses:  instance 0 and instance 1
 *   configdata - The new configuration data to be saved
 *   datalen    - The size of the configuration data in bytes.
 *
 * Returned Value:
 *   This is an end-user function, so it follows the normal convention:
 *   Returns the OK (zero) on success. On failure, it.returns -1 (ERROR) and
 *   sets errno appropriately.
 *
 *   Values for the errno would include:
 *
 *     EINVAL - The configdata point is invalid
 *     ENOSYS - The request ID/instance is not supported on this platform
 *
 *   Other errors may be returned from lower level drivers on failure to
 *   write to the underlying media (if applicable)
 *
 ****************************************************************************/

int platform_setconfig(enum config_data_e id, int instance,
                       FAR const uint8_t *configdata, size_t datalen)
{
#ifdef CONFIG_MIKROE_STM32F4_CONFIGDATA_FS
  FILE *fd;
#endif
#ifdef CONFIG_MIKROE_STM32F4_CONFIGDATA_PART
  struct config_data_s config;
  int ret;
  int fd;

  /* Try to open the /dev/config device file */

  if ((fd = open("/dev/config", O_RDOK)) == -1)
    {
      /* Error opening the config device */

      return -1;
    }

  /* Setup structure for the SETCONFIG ioctl */

  config.id         = (enum config_data_e)id;
  config.instance   = instance;
  config.configdata = (FAR uint8_t *) configdata;
  config.len        = datalen;

  ret = ioctl(fd, CFGDIOC_SETCONFIG, (unsigned long) &config);
  close(fd);
  return ret;

#else /* CONFIG_MIKROE_STM32F4_CONFIGDATA_PART */

  switch (id)
    {
      case CONFIGDATA_TSCALIBRATION:

#ifdef CONFIG_MIKROE_STM32F4_CONFIGDATA_FS
        /* Save config data in a file on the filesystem.  Try to open
         * the file.
         */

        fd = fopen(CONFIG_MIKROE_STM32F4_CONFIGDATA_FILENAME, "w+");
        if (fd == NULL)
          {
            /* Error opening the file */

            errno = ENOENT;
            return -1;
          }

        /* Write data to the file.  For now, we only support one entry, but
         * may / will expand it later.
         */

        fwrite(&id, sizeof(id), 1, fd);
        fwrite(&instance, sizeof(instance), 1, fd);
        fwrite(configdata, 1, datalen, fd);

        /* CLose the file and exit */

        fclose(fd);
        return OK;

#elif defined(CONFIG_MIKROE_STM32F4_CONFIGDATA_ROM)
        /* We are reading from a read-only system, so nothing to do. */

        return OK;

#else
        break;
#endif /* CONFIG_MIKROE_STM32F4_CONFIGDATA_ROM */

      default:
        break;
    }

  errno = ENOSYS;
  return -1;

#endif /* CONFIG_MIKROE_STM32F4_CONFIGDATA_PART */
}

/****************************************************************************
 * Name: platform_getconfig
 *
 * Description:
 *   Get platform-specific configuration data
 *
 * Input Parameter:
 *   id         - Defines the class of configuration data
 *   instance   - Defines which instance of configuration data.  For example,
 *                if a board has two networks, then there would be two MAC
 *                addresses:  instance 0 and instance 1
 *   configdata - The new configuration data to be saved
 *   datalen    - The size of the configuration data in bytes.
 *
 * Returned Value:
 *   This is an end-user function, so it follows the normal convention:
 *   Returns the OK (zero) on success. On failure, it.returns -1 (ERROR) and
 *   sets errno appropriately.
 *
 *   Values for the errno would include:
 *
 *     EINVAL - The configdata point is invalid
 *     ENOSYS - The request ID/instance is not supported on this platform
 *
 *   Other errors may be returned from lower level drivers on failure to
 *   write to the underlying media (if applicable)
 *
 ****************************************************************************/

int platform_getconfig(enum config_data_e id, int instance,
                       FAR uint8_t *configdata, size_t datalen)
{
#ifdef CONFIG_MIKROE_STM32F4_CONFIGDATA_FS
  FILE   *fd;
  size_t  bytes;
  enum    config_data_e saved_id;
  int     saved_instance;
#elif defined(CONFIG_MIKROE_STM32F4_CONFIGDATA_ROM)
  static const uint8_t touch_cal_data[] =
  {
      0x9a, 0x2f, 0x00, 0x00,
      0x40, 0xbc, 0x69, 0xfe, 0x70, 0x2e, 0x00,
      0x00, 0xb8, 0x2d, 0xdb, 0xff
  };
#endif

#ifdef CONFIG_MIKROE_STM32F4_CONFIGDATA_PART
  struct config_data_s  config;
  int                   ret;
  int                   fd;

  /* Try to open the /dev/config device file */

  if ((fd = open("/dev/config", O_RDOK)) == -1)
    {
      /* Error opening the config device */

      return -1;
    }

  /* Setup structure for the SETCONFIG ioctl */

  config.id = (enum config_data_e)id;
  config.instance = instance;
  config.configdata = configdata;
  config.len = datalen;

  ret = ioctl(fd, CFGDIOC_GETCONFIG, (unsigned long) &config);
  close(fd);
  return ret;

#else /* CONFIG_MIKROE_STM32F4_CONFIGDATA_PART */
  switch (id)
    {
      case CONFIGDATA_TSCALIBRATION:

#ifdef CONFIG_MIKROE_STM32F4_CONFIGDATA_FS
        /* Load config data fram a file on the filesystem.  Try to open
         * the file.
         */

        fd = fopen(CONFIG_MIKROE_STM32F4_CONFIGDATA_FILENAME, "r");
        if (fd == NULL)
          {
            /* Error opening the file */

            errno = ENOENT;
            return -1;
          }

        /* Write data to the file.  For now, we only support one entry, but
         * may / will expand it later.
         */

        bytes = fread(&saved_id, sizeof(saved_id), 1, fd);
        bytes += fread(&saved_instance, sizeof(saved_instance), 1, fd);
        bytes += fread(configdata, 1, datalen, fd);
        if (bytes != sizeof(saved_id) + sizeof(saved_instance) + datalen)
          {
            /* Error!  Not enough data in the file */

            errno = EINVAL;
            fclose(fd);
            return -1;
          }

        /* Close the file and exit */

        fclose(fd);
        return OK;

#elif defined(CONFIG_MIKROE_STM32F4_CONFIGDATA_ROM)
        memcpy(configdata, touch_cal_data, datalen);
        return OK;

#else
        break;
#endif

      default:
        break;
    }

  errno = ENOSYS;
  return -1;

#endif /* CONFIG_MIKROE_STM32F4_CONFIGDATA_PART */
}

#endif /* CONFIG_PLATFORM_CONFIGDATA */
