/****************************************************************************
 * apps/lte/lapi/src/lapi_firmware.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <nuttx/wireless/lte/lte_ioctl.h>

#include "lte/lapi.h"
#include "lte/lte_api.h"
#include "lte/lte_fwupdate.h"

#include "lapi_dbg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int fw_inject_internal(FAR const char *data, int len, bool init)
{
  FAR void *inarg[3] =
    {
      (FAR void *)data, &len, &init
    };

  int dummy_arg; /* Dummy for blocking API call */

  if (data == NULL || len < 0)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_INJECTIMAGE,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)&dummy_arg, 0, NULL);
}

static int fw_generic_request(int cmdid)
{
  int dummy_arg; /* Dummy for blocking API call */

  return lapi_req(cmdid, NULL, 0, (FAR void *)&dummy_arg, 0, NULL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Synchronous APIs */

int lte_get_version_sync(FAR lte_version_t *version)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, version
    };

  if (version == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETVER,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  return (ret == 0) ? result : ret;
}

int ltefwupdate_initialize(FAR const char *initial_data, int len)
{
  return fw_inject_internal(initial_data, len, true);
}

int ltefwupdate_injectrest(FAR const char *rest_data, int len)
{
  return fw_inject_internal(rest_data, len, false);
}

int ltefwupdate_injected_datasize(void)
{
  return fw_generic_request(LTE_CMDID_GETIMAGELEN);
}

int ltefwupdate_execute(void)
{
  int ret;

  /* If wakelock is not acquired, firmware update
   * cannot be performed.
   */

  ret = lte_get_wakelock_count();
  if (ret <= 0)
    {
      if (ret == 0)
        {
          lapi_printf("wakelock is not acquired\n");
          ret = -EPERM;
        }

      return ret;
    }

  return fw_generic_request(LTE_CMDID_EXEUPDATE);
}

int ltefwupdate_result(void)
{
  return fw_generic_request(LTE_CMDID_GETUPDATERES);
}

int lte_factory_reset_sync(void)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result
    };

  ret = lapi_req(LTE_CMDID_FACTORY_RESET,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);

  return ret;
}

/* Asynchronous APIs */

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

int lte_get_version(get_ver_cb_t callback)
{
  printf("This API is discarded. Please use lte_get_version_sync().\n");
  return -ENOTSUP;
}
#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

