/****************************************************************************
 * apps/lte/lapi/src/lapi_sim.c
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

#include <stdint.h>
#include <errno.h>
#include <sys/param.h>
#include <nuttx/wireless/lte/lte_ioctl.h>

#include "lte/lte_api.h"
#include "lte/lapi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int lte_get_siminfo_inparam_check(uint32_t option)
{
  uint32_t mask = 0;

  mask = (LTE_SIMINFO_GETOPT_MCCMNC |
          LTE_SIMINFO_GETOPT_SPN |
          LTE_SIMINFO_GETOPT_ICCID |
          LTE_SIMINFO_GETOPT_IMSI |
          LTE_SIMINFO_GETOPT_GID1 |
          LTE_SIMINFO_GETOPT_GID2);

  if (0 == (option & mask))
    {
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Synchronous APIs */

int lte_get_siminfo_sync(uint32_t option, FAR lte_siminfo_t *siminfo)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      &option
    };

  FAR void *outarg[] =
    {
      &result, siminfo
    };

  if (lte_get_siminfo_inparam_check(option) || siminfo == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETSIMINFO,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_imscap_sync(FAR bool *imscap)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, imscap
    };

  if (imscap == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_IMSCAP,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
int lte_get_imsi_sync(FAR char *imsi)
#else
int lte_get_imsi_sync(FAR char *imsi, size_t len)
#endif
{
  int ret;
  int result;
  uint8_t errcause;
  FAR void *outarg[] =
    {
#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
      &result, &errcause, imsi
#else
      &result, &errcause, imsi, &len
#endif
    };

  if (imsi == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETIMSI,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
int lte_get_imei_sync(FAR char *imei)
#else
int lte_get_imei_sync(FAR char *imei, size_t len)
#endif
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
      &result, imei
#else
      &result, imei, &len
#endif
    };

  if (imei == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETIMEI,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
int lte_get_phoneno_sync(FAR char *phoneno)
#else
int lte_get_phoneno_sync(FAR char *phoneno, size_t len)
#endif
{
  int ret;
  int result;
  uint8_t errcause;
  FAR void *outarg[] =
    {
#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
      &result, &errcause, phoneno
#else
      &result, &errcause, phoneno, &len
#endif
    };

  if (phoneno == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETPHONE,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_report_simstat(simstat_report_cb_t callback)
{
  int ret;
  int result;
  int32_t id = LTE_CMDID_REPSIMSTAT;
  FAR void *inarg[] =
    {
      callback, &id
    };

  FAR void *outarg[] =
    {
      &result
    };

  ret = lapi_req(LTE_CMDID_REPSIMSTAT,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 callback);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

/* Asynchronous APIs */

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

int lte_get_siminfo(uint32_t option, get_siminfo_cb_t callback)
{
  FAR void *inarg[] =
    {
      &option
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  if (lte_get_siminfo_inparam_check(option))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETSIMINFO | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

int lte_get_imscap(get_imscap_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_IMSCAP | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_imsi(get_imsi_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETIMSI | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_imei(get_imei_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETIMEI | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_phoneno(get_phoneno_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETPHONE | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}
#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

