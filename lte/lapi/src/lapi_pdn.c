/****************************************************************************
 * apps/lte/lapi/src/lapi_pdn.c
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

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int lte_activate_pdn_inparam_check(FAR lte_apn_setting_t *apn)
{
  int32_t mask = 0;

  if (!apn)
    {
      lapi_printf("apn is null.\n");
      return -EINVAL;
    }

  if ((!apn->apn) || (strnlen((char *)apn->apn, LTE_APN_LEN) >= LTE_APN_LEN))
    {
      lapi_printf("apn is length overflow.\n");
      return  -EINVAL;
    }

  if ((apn->ip_type < LTE_IPTYPE_V4) ||
      (apn->ip_type > LTE_IPTYPE_NON))
    {
      lapi_printf("ip type is invalid. iptype=%d\n", apn->ip_type);
      return -EINVAL;
    }

  if ((apn->auth_type < LTE_APN_AUTHTYPE_NONE) ||
      (apn->auth_type > LTE_APN_AUTHTYPE_CHAP))
    {
      lapi_printf("auth type is invalid. authtype=%d\n", apn->auth_type);
      return -EINVAL;
    }

  if (apn->user_name && apn->password)
    {
      if (strnlen((FAR char *)apn->user_name, LTE_APN_USER_NAME_LEN) >=
        LTE_APN_USER_NAME_LEN)
        {
          lapi_printf("username is length overflow.\n");
          return -EINVAL;
        }

      if (strnlen((FAR char *)apn->password, LTE_APN_PASSWD_LEN) >=
        LTE_APN_PASSWD_LEN)
        {
          lapi_printf("password is length overflow.\n");
          return  -EINVAL;
        }
    }
  else
    {
      if (apn->auth_type != LTE_APN_AUTHTYPE_NONE)
        {
          lapi_printf("authentication information is invalid.\n");
          return -EINVAL;
        }
    }

  mask = (LTE_APN_TYPE_DEFAULT |
    LTE_APN_TYPE_MMS | LTE_APN_TYPE_SUPL | LTE_APN_TYPE_DUN |
    LTE_APN_TYPE_HIPRI | LTE_APN_TYPE_FOTA | LTE_APN_TYPE_IMS |
    LTE_APN_TYPE_CBS | LTE_APN_TYPE_IA | LTE_APN_TYPE_EMERGENCY);
  if (0 == (apn->apn_type & mask))
    {
      lapi_printf("apn type is invalid. apntype=%08ld / mask=%08ld \n",
        apn->apn_type, mask);
      return -EINVAL;
    }

  return OK;
}

static int lte_deactivate_pdn_inparam_check(uint8_t session_id)
{
  if (LTE_SESSION_ID_MIN > session_id ||
      LTE_SESSION_ID_MAX < session_id)
    {
      lapi_printf("Invalid session id %d.\n", session_id);
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Synchronous APIs */

int lte_activate_pdn_sync(FAR lte_apn_setting_t *apn, FAR lte_pdn_t *pdn)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      apn
    };

  FAR void *outarg[] =
    {
      &result, pdn
    };

  if (lte_activate_pdn_inparam_check(apn) || pdn == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_ACTPDN,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_deactivate_pdn_sync(uint8_t session_id)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      &session_id
    };

  FAR void *outarg[] =
    {
      &result
    };

  if (lte_deactivate_pdn_inparam_check(session_id))
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_DEACTPDN,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

/* Asynchronous APIs */

int lte_activate_pdn(FAR lte_apn_setting_t *apn, activate_pdn_cb_t callback)
{
  FAR void *inarg[] =
    {
      apn
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  if (lte_activate_pdn_inparam_check(apn))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_ACTPDN | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

int lte_deactivate_pdn(uint8_t session_id, deactivate_pdn_cb_t callback)
{
  FAR void *inarg[] =
    {
      &session_id
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  if (lte_deactivate_pdn_inparam_check(session_id))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_DEACTPDN | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

int lte_activate_pdn_cancel(void)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result
    };

  ret = lapi_req(LTE_CMDID_ACTPDNCAN,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}
