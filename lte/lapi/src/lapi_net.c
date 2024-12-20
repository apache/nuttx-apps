/****************************************************************************
 * apps/lte/lapi/src/lapi_net.c
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
#include <errno.h>
#include <sys/param.h>
#include <nuttx/wireless/lte/lte_ioctl.h>

#include "lte/lte_api.h"
#include "lte/lapi.h"

#include "lapi_dbg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CELLINFO_PERIOD_MIN (1)
#define CELLINFO_PERIOD_MAX (4233600)

#define QUALITY_PERIOD_MIN (1)
#define QUALITY_PERIOD_MAX (4233600)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int lte_get_netinfo_inparam_check(uint8_t pdn_num)
{
  if (LTE_SESSION_ID_MIN > pdn_num || LTE_SESSION_ID_MAX < pdn_num)
    {
      return -EINVAL;
    }

  return OK;
}

static int lte_set_rat_inparam_check(uint8_t rat, bool persistent)
{
  if (rat != LTE_RAT_CATM &&
      rat != LTE_RAT_NBIOT)
    {
      lapi_printf("RAT type is invalid [%d].\n", rat);
      return -EINVAL;
    }

  if (persistent != LTE_ENABLE &&
      persistent != LTE_DISABLE)
    {
      lapi_printf("persistent is invalid [%d].\n", persistent);
      return -EINVAL;
    }

  return OK;
}

static int lte_set_report_cellinfo_inparam_check(
  cellinfo_report_cb_t callback, uint32_t period)
{
  if (callback)
    {
      if (CELLINFO_PERIOD_MIN > period || CELLINFO_PERIOD_MAX < period)
        {
          lapi_printf("Invalid parameter.\n");
          return -EINVAL;
        }
    }

  return OK;
}

static int lte_set_report_quality_inparam_check(quality_report_cb_t callback,
  uint32_t period)
{
  if (callback)
    {
      if (QUALITY_PERIOD_MIN > period || QUALITY_PERIOD_MAX < period)
        {
          lapi_printf("Invalid parameter.\n");
          return -EINVAL;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Synchronous APIs */

int lte_get_netinfo_sync(uint8_t pdn_num, FAR lte_netinfo_t *info)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      &pdn_num
    };

  FAR void *outarg[] =
    {
      &result, info, &pdn_num
    };

  if (lte_get_netinfo_inparam_check(pdn_num) || info == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETNETINFO,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_localtime_sync(FAR lte_localtime_t *localtime)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, localtime
    };

  if (localtime == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETLTIME,
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
int lte_get_operator_sync(FAR char *oper)
#else
int lte_get_operator_sync(FAR char *oper, size_t len)
#endif
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
#ifdef CONFIG_LTE_LAPI_KEEP_COMPATIBILITY
      &result, oper
#else
      &result, oper, &len
#endif
    };

  if (oper == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETOPER,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_quality_sync(FAR lte_quality_t *quality)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, quality
    };

  if (quality == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETQUAL,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_cellinfo_sync(FAR lte_cellinfo_t *cellinfo)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, cellinfo
    };

  if (cellinfo == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETCELL,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_rat_sync(void)
{
  int ret;
  int result;
  lte_ratinfo_t ratinfo;
  FAR void *outarg[] =
    {
      &result, &ratinfo
    };

  ret = lapi_req(LTE_CMDID_GETRAT,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = ratinfo.rat;
    }

  return ret;
}

int lte_set_rat_sync(uint8_t rat, bool persistent)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      &rat, &persistent
    };

  FAR void *outarg[] =
    {
      &result
    };

  if (lte_set_rat_inparam_check(rat, persistent))
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_SETRAT,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_ratinfo_sync(FAR lte_ratinfo_t *info)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, info
    };

  if (info == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETRATINFO,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

/* Asynchronous APIs */

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

int lte_get_netinfo(get_netinfo_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETNETINFO | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_localtime(get_localtime_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETLTIME | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_operator(get_operator_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETOPER | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_quality(get_quality_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETQUAL | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

int lte_set_report_netinfo(netinfo_report_cb_t callback)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      callback
    };

  FAR void *outarg[] =
    {
      &result
    };

  ret = lapi_req(LTE_CMDID_REPNETINFO,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 callback);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_report_localtime(localtime_report_cb_t callback)
{
  int ret;
  int result;
  int32_t id = LTE_CMDID_REPLTIME;
  FAR void *inarg[] =
    {
      callback, &id
    };

  FAR void *outarg[] =
    {
      &result
    };

  ret = lapi_req(LTE_CMDID_REPLTIME,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 callback);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_report_quality(quality_report_cb_t callback, uint32_t period)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      callback, &period
    };

  FAR void *outarg[] =
    {
      &result
    };

  if (lte_set_report_quality_inparam_check(callback, period))
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_REPQUAL,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 callback);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_report_cellinfo(cellinfo_report_cb_t callback,
  uint32_t period)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      callback, &period
    };

  FAR void *outarg[] =
    {
      &result
    };

  if (lte_set_report_cellinfo_inparam_check(callback, period))
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_REPCELL,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 callback);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}
