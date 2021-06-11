/****************************************************************************
 * apps/include/platform/configdata.h
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

#ifndef __APPS_INCLUDE_PLATFORM_CONFIGDATA_H
#define __APPS_INCLUDE_PLATFORM_CONFIGDATA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#ifdef CONFIG_PLATFORM_CONFIGDATA

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This enumeration identifies classes of configuration data */

enum config_data_e
{
  /* Product identification */

  CONFIGDATA_SERIALNUMBER = 0,  /* Product serial number */

  /* Prduct networking configuration */

  CONFIGDATA_MACADDRESS,        /* Assigned MAC address */
  CONFIGDATA_IPADDRESS,         /* Configured IP address */
  CONFIGDATA_NETMASK,           /* Configured network mask */
  CONFIGDATA_DIPADDR,           /* Configured default router address */

  /* GUI configuration */

  CONFIGDATA_TSCALIBRATION,     /* Measured touchscreen calibration data */
  CONFIGDATA_OTHER              /* Other configuration data */
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
 * Public Functions Prototypes
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
                       FAR const uint8_t *configdata, size_t datalen);

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
 *   configdata - The user provided location to return the configuration data
 *   datalen    - The expected size of the configuration data to be returned.
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
 *   read from the underlying media (if applicable)
 *
 ****************************************************************************/

int platform_getconfig(enum config_data_e id, int instance,
                       FAR uint8_t *configdata, size_t datalen);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* CONFIG_PLATFORM_CONFIGDATA */
#endif /* __APPS_INCLUDE_PLATFORM_CONFIGDATA_H */
