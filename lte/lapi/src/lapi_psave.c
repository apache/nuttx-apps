/****************************************************************************
 * apps/lte/lapi/src/lapi_psave.c
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

#define ALTCOMBS_EDRX_CYCLE_WBS1_MIN      (LTE_EDRX_CYC_512)
#define ALTCOMBS_EDRX_CYCLE_WBS1_MAX      (LTE_EDRX_CYC_262144)
#define ALTCOMBS_EDRX_CYCLE_NBS1_MIN      (LTE_EDRX_CYC_2048)
#define ALTCOMBS_EDRX_CYCLE_NBS1_MAX      (LTE_EDRX_CYC_1048576)
#define ALTCOMBS_EDRX_PTW_WBS1_MIN        (LTE_EDRX_PTW_128)
#define ALTCOMBS_EDRX_PTW_WBS1_MAX        (LTE_EDRX_PTW_2048)
#define ALTCOMBS_EDRX_PTW_NBS1_MIN        (LTE_EDRX_PTW_256)
#define ALTCOMBS_EDRX_PTW_NBS1_MAX        (LTE_EDRX_PTW_4096)
#define ALTCOMBS_PSM_UNIT_T3324_MIN       (LTE_PSM_T3324_UNIT_2SEC)
#define ALTCOMBS_PSM_UNIT_T3324_MAX       (LTE_PSM_T3324_UNIT_6MIN)
#define ALTCOMBS_PSM_UNIT_T3412_MIN       (LTE_PSM_T3412_UNIT_2SEC)
#define ALTCOMBS_PSM_UNIT_T3412_MAX       (LTE_PSM_T3412_UNIT_320HOUR)
#define ALTCOMBS_EDRX_INVALID             (255)

#define APICMD_EDRX_ACTTYPE_NOTUSE   (0) /* eDRX is not running */
#define APICMD_EDRX_ACTTYPE_ECGSMIOT (1) /* EC-GSM-IoT (A/Gb mode) */
#define APICMD_EDRX_ACTTYPE_GSM      (2) /* GSM (A/Gb mode) */
#define APICMD_EDRX_ACTTYPE_IU       (3) /* UTRAN (Iu mode) */
#define APICMD_EDRX_ACTTYPE_WBS1     (4) /* E-UTRAN (WB-S1 mode) */
#define APICMD_EDRX_ACTTYPE_NBS1     (5) /* E-UTRAN (NB-S1 mode) */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int lte_set_edrx_inparam_check(FAR lte_edrx_setting_t *settings)
{
  int32_t ret = 0;

  if (!settings)
    {
      lapi_printf("Input argument is NULL.\n");
      return -EINVAL;
    }

  if (settings->act_type != LTE_EDRX_ACTTYPE_WBS1 &&
      settings->act_type != LTE_EDRX_ACTTYPE_NBS1 &&
      settings->act_type != LTE_EDRX_ACTTYPE_ECGSMIOT &&
      settings->act_type != LTE_EDRX_ACTTYPE_GSM &&
      settings->act_type != LTE_EDRX_ACTTYPE_IU &&
      settings->act_type != LTE_EDRX_ACTTYPE_NOTUSE)
    {
      lapi_printf("Input argument act_type is invalid.\n");
      return -EINVAL;
    }

  ret = lte_get_rat_sync();
  if (ret < 0 && ret != -ENOTSUP)
    {
      lapi_printf("Unable to read RAT setting from the device."
                  " ret: [%ld].\n",
                  ret);
      return ret;
    }
  else if (ret == -ENOTSUP)
    {
      /* act_type check for protocol version V1 */

      if (LTE_EDRX_ACTTYPE_NOTUSE != settings->act_type &&
          LTE_EDRX_ACTTYPE_WBS1   != settings->act_type)
        {
          lapi_printf("Operation is not allowed[act_type : %d].\n",
            settings->act_type);
          return -EPERM;
        }
    }
  else
    {
      /* act_type check for version V4 or later */

      if (!((ret == LTE_RAT_CATM
             && settings->act_type == LTE_EDRX_ACTTYPE_WBS1) ||
            (ret == LTE_RAT_NBIOT
             && settings->act_type == LTE_EDRX_ACTTYPE_NBS1) ||
            (settings->act_type == LTE_EDRX_ACTTYPE_NOTUSE)))
        {
          lapi_printf("Operation is not allowed[act_type : %d,"
                      " RAT : %ld].\n",
                      settings->act_type, ret);
          return -EPERM;
        }
    }

  if (settings->enable)
    {
      if (settings->act_type == LTE_EDRX_ACTTYPE_WBS1)
        {
          if (!(ALTCOMBS_EDRX_CYCLE_WBS1_MIN <= settings->edrx_cycle &&
            settings->edrx_cycle <= ALTCOMBS_EDRX_CYCLE_WBS1_MAX))
            {
              lapi_printf("Input argument edrx_cycle is invalid.\n");
              return -EINVAL;
            }

          if (!(ALTCOMBS_EDRX_PTW_WBS1_MIN <= settings->ptw_val &&
            settings->ptw_val <= ALTCOMBS_EDRX_PTW_WBS1_MAX))
            {
              lapi_printf("Input argument ptw is invalid.\n");
              return -EINVAL;
            }
        }

      if (settings->act_type == LTE_EDRX_ACTTYPE_NBS1)
        {
          if (!(ALTCOMBS_EDRX_CYCLE_NBS1_MIN <= settings->edrx_cycle &&
              settings->edrx_cycle <= ALTCOMBS_EDRX_CYCLE_NBS1_MAX))
            {
              lapi_printf("Input argument edrx_cycle is invalid.\n");
              return -EINVAL;
            }

          if (!(ALTCOMBS_EDRX_PTW_NBS1_MIN <= settings->ptw_val &&
              settings->ptw_val <= ALTCOMBS_EDRX_PTW_NBS1_MAX))
            {
              lapi_printf("Input argument ptw is invalid.\n");
              return -EINVAL;
            }
        }
    }

  return OK;
}

static int lte_set_psm_inparam_check(FAR lte_psm_setting_t *settings)
{
  if (!settings)
    {
      lapi_printf("Input argument is NULL.\n");
      return -EINVAL;
    }

  if (LTE_ENABLE == settings->enable)
    {
      if (settings->req_active_time.unit < LTE_PSM_T3324_UNIT_2SEC ||
          settings->req_active_time.unit > LTE_PSM_T3324_UNIT_DEACT)
        {
          lapi_printf("Invalid rat_time unit :%d\n",
            settings->req_active_time.unit);
          return -EINVAL;
        }

      if (settings->req_active_time.time_val < LTE_PSM_TIMEVAL_MIN ||
          settings->req_active_time.time_val > LTE_PSM_TIMEVAL_MAX)
        {
          lapi_printf("Invalid rat_time time_val :%d\n",
            settings->req_active_time.time_val);
          return -EINVAL;
        }

      if (settings->ext_periodic_tau_time.unit < LTE_PSM_T3412_UNIT_2SEC ||
          settings->ext_periodic_tau_time.unit > LTE_PSM_T3412_UNIT_DEACT)
        {
          lapi_printf("Invalid tau_time unit :%d\n",
            settings->ext_periodic_tau_time.unit);
          return -EINVAL;
        }

      if (settings->ext_periodic_tau_time.time_val < LTE_PSM_TIMEVAL_MIN ||
          settings->ext_periodic_tau_time.time_val > LTE_PSM_TIMEVAL_MAX)
        {
          lapi_printf("Invalid tau_time time_val :%d\n",
            settings->ext_periodic_tau_time.time_val);
          return -EINVAL;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Synchronous APIs */

int lte_get_edrx_sync(FAR lte_edrx_setting_t *settings)
{
  int ret;
  int result;
  bool    is_edrxevt;
  FAR void *outarg[] =
    {
      &result, settings, &is_edrxevt
    };

  if (settings == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETEDRX,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_edrx_sync(FAR lte_edrx_setting_t *settings)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      settings
    };

  FAR void *outarg[] =
    {
      &result
    };

  ret = lte_set_edrx_inparam_check(settings);
  if (ret < 0)
    {
      return ret;
    }

  ret = lapi_req(LTE_CMDID_SETEDRX,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_psm_sync(FAR lte_psm_setting_t *settings)
{
  int ret;
  int result;
  int32_t id = LTE_CMDID_GETPSM;
  bool    is_psmevt;
  FAR void *inarg[] =
    {
      &id
    };

  FAR void *outarg[] =
    {
      &result, settings, &is_psmevt
    };

  if (settings == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETPSM,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_psm_sync(FAR lte_psm_setting_t *settings)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      settings
    };

  FAR void *outarg[] =
    {
      &result
    };

  if (lte_set_psm_inparam_check(settings))
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_SETPSM,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_ce_sync(FAR lte_ce_setting_t *settings)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, settings
    };

  if (settings == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETCE,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_ce_sync(FAR lte_ce_setting_t *settings)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      settings
    };

  FAR void *outarg[] =
    {
      &result
    };

  if (settings == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_SETCE,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_current_edrx_sync(FAR lte_edrx_setting_t *settings)
{
  int ret;
  int result;
  bool    is_getcedrxevt;
  FAR void *outarg[] =
    {
      &result, settings, &is_getcedrxevt
    };

  if (settings == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETCEDRX,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_get_current_psm_sync(FAR lte_psm_setting_t *settings)
{
  int ret;
  int result;
  bool    is_getcpsmevt;
  FAR void *outarg[] =
    {
      &result, settings, &is_getcpsmevt
    };

  if (settings == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETCPSM,
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

int lte_get_edrx(get_edrx_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETEDRX | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_set_edrx(FAR lte_edrx_setting_t *settings, set_edrx_cb_t callback)
{
  int ret;
  FAR void *inarg[] =
    {
      settings
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  ret = lte_set_edrx_inparam_check(settings);
  if (ret < 0)
    {
      return ret;
    }

  return lapi_req(LTE_CMDID_SETEDRX | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

int lte_get_psm(get_psm_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETPSM | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_set_psm(FAR lte_psm_setting_t *settings, set_psm_cb_t callback)
{
  FAR void *inarg[] =
    {
      settings
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  if (lte_set_psm_inparam_check(settings))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_SETPSM | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

int lte_get_ce(get_ce_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETCE | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_set_ce(FAR lte_ce_setting_t *settings, set_ce_cb_t callback)
{
  FAR void *inarg[] =
    {
      settings
    };

  if (settings == NULL || callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_SETCE | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

int lte_get_current_edrx(get_current_edrx_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETCEDRX | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_dynamic_edrx_param(get_dynamic_edrx_param_cb_t callback)
{
  return lte_get_current_edrx(callback);
}

int lte_get_current_psm(get_current_psm_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETCPSM | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_get_dynamic_psm_param(get_dynamic_psm_param_cb_t callback)
{
  return lte_get_current_psm(callback);
}
#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

