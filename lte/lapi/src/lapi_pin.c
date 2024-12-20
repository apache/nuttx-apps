/****************************************************************************
 * apps/lte/lapi/src/lapi_pin.c
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

#define SETPIN_TARGETPIN_MIN LTE_TARGET_PIN
#define SETPIN_TARGETPIN_MAX LTE_TARGET_PIN2

#define APICMD_SETPINLOCK_PINCODE_LEN    9

#define SETPIN_MIN_PIN_LEN (4)
#define SETPIN_MAX_PIN_LEN ((APICMD_SETPINLOCK_PINCODE_LEN) - 1)

#define APICMD_ENTERPIN_PINCODE_LEN              9
#define ENTERPIN_MIN_PIN_LEN (4)
#define ENTERPIN_MAX_PIN_LEN ((APICMD_ENTERPIN_PINCODE_LEN) - 1)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int lte_change_pin_inparam_check(int8_t target_pin, FAR char *pincode,
  FAR char *new_pincode)
{
  uint8_t pinlen = 0;

  if (!pincode || !new_pincode)
    {
      lapi_printf("Input argument is NULL.\n");
      return -EINVAL;
    }

  if (SETPIN_TARGETPIN_MIN > target_pin || SETPIN_TARGETPIN_MAX < target_pin)
    {
      lapi_printf("Unsupport change type. type:%d\n", target_pin);
      return -EINVAL;
    }

  pinlen = strnlen(pincode, APICMD_SETPINLOCK_PINCODE_LEN);
  if (pinlen < SETPIN_MIN_PIN_LEN || SETPIN_MAX_PIN_LEN < pinlen)
    {
      return -EINVAL;
    }

  pinlen = strnlen(new_pincode, APICMD_SETPINLOCK_PINCODE_LEN);
  if (pinlen < SETPIN_MIN_PIN_LEN || SETPIN_MAX_PIN_LEN < pinlen)
    {
      return -EINVAL;
    }

  return OK;
}

static int lte_enter_pin_inparam_check(FAR char *pincode,
                                       FAR char *new_pincode)
{
  uint8_t pinlen = 0;

  if (!pincode)
    {
      lapi_printf("Input argument is NULL.\n");
      return -EINVAL;
    }

  pinlen = strnlen(pincode, APICMD_SETPINLOCK_PINCODE_LEN);
  if (pinlen < ENTERPIN_MIN_PIN_LEN || ENTERPIN_MAX_PIN_LEN < pinlen)
    {
      lapi_printf("Invalid PIN code length.length:%d\n", pinlen);
      return -EINVAL;
    }

  if (new_pincode)
    {
      lapi_printf("lte_enter_pin() doesn't support entering PUK code.\n");
      lapi_printf("lte_enter_pin_sync() doesn't support entering"
                  " PUK code.\n");
      return -EINVAL;
    }

  return OK;
}

static int lte_set_pinenable_inparam_check(bool enable, FAR char *pincode)
{
  uint8_t pinlen = 0;

  if (!pincode)
    {
      lapi_printf("Input argument is NULL.\n");
      return -EINVAL;
    }

  pinlen = strnlen(pincode, APICMD_SETPINLOCK_PINCODE_LEN);
  if (pinlen < SETPIN_MIN_PIN_LEN || SETPIN_MAX_PIN_LEN < pinlen)
    {
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Synchronous APIs */

int lte_get_pinset_sync(FAR lte_getpin_t *pinset)
{
  int ret;
  int result;
  FAR void *outarg[] =
    {
      &result, pinset
    };

  if (pinset == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETPINSET,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_set_pinenable_sync(bool enable, FAR char *pincode,
  FAR uint8_t *attemptsleft)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      &enable, pincode
    };

  FAR void *outarg[] =
    {
      &result, attemptsleft
    };

  if (lte_set_pinenable_inparam_check(enable, pincode) ||
    attemptsleft == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_PINENABLE,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_change_pin_sync(int8_t target_pin, FAR char *pincode,
  FAR char *new_pincode, FAR uint8_t *attemptsleft)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      &target_pin, pincode, new_pincode
    };

  FAR void *outarg[] =
    {
      &result, attemptsleft
    };

  if (lte_change_pin_inparam_check(target_pin, pincode, new_pincode) ||
    attemptsleft == NULL)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_CHANGEPIN,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);
  if (ret == 0)
    {
      ret = result;
    }

  return ret;
}

int lte_enter_pin_sync(FAR char *pincode, FAR char *new_pincode,
  FAR uint8_t *simstat, FAR uint8_t *attemptsleft)
{
  int ret;
  int result;
  FAR void *inarg[] =
    {
      pincode, new_pincode
    };

  FAR void *outarg[] =
    {
      &result, simstat, attemptsleft
    };

  lte_getpin_t pinset =
    {
      0
    };

  if (lte_enter_pin_inparam_check(pincode, new_pincode) || simstat == NULL ||
    attemptsleft == NULL)
    {
      return -EINVAL;
    }

  ret = lte_get_pinset_sync(&pinset);
  if (ret < 0)
    {
      lapi_printf("Failed to get pinset.%d\n", ret);
      return ret;
    }

  if (simstat)
    {
      *simstat = pinset.status;
    }

  if (attemptsleft)
    {
      if (pinset.status == LTE_PINSTAT_SIM_PUK)
        {
          *attemptsleft = pinset.puk_attemptsleft;
        }
      else
        {
          *attemptsleft = pinset.pin_attemptsleft;
        }
    }

  if (pinset.enable == LTE_DISABLE)
    {
      lapi_printf(
        "PIN lock is disable. Don't need to run lte_enter_pin_sync().\n");
      return -EPERM;
    }
  else if (pinset.status != LTE_PINSTAT_SIM_PIN)
    {
      if (pinset.status == LTE_PINSTAT_SIM_PUK)
        {
          lapi_printf(
            "This SIM is PUK locked. lte_enter_pin_sync() can't be used.\n");
        }
      else
        {
          lapi_printf("PIN is already unlocked. "
            "Don't need to run lte_enter_pin_sync(). status:%d\n",
            pinset.status);
        }

      return -EPERM;
    }

  ret = lapi_req(LTE_CMDID_ENTERPIN,
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

#ifdef CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API

int lte_get_pinset(get_pinset_cb_t callback)
{
  if (callback == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_GETPINSET | LTE_CMDOPT_ASYNC_BIT,
                  NULL, 0, NULL, 0, callback);
}

int lte_set_pinenable(bool enable, FAR char *pincode,
  set_pinenable_cb_t callback)
{
  FAR void *inarg[] =
    {
      &enable, pincode
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  if (lte_set_pinenable_inparam_check(enable, pincode))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_PINENABLE | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

int lte_change_pin(int8_t target_pin, FAR char *pincode,
  FAR char *new_pincode, change_pin_cb_t callback)
{
  FAR void *inarg[] =
    {
      &target_pin, pincode, new_pincode
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  if (lte_change_pin_inparam_check(target_pin, pincode, new_pincode))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_CHANGEPIN | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}

int lte_enter_pin(FAR char *pincode, FAR char *new_pincode,
  enter_pin_cb_t callback)
{
  FAR void *inarg[] =
    {
      pincode, new_pincode
    };

  if (callback == NULL)
    {
      return -EINVAL;
    }

  if (lte_enter_pin_inparam_check(pincode, new_pincode))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_ENTERPIN | LTE_CMDOPT_ASYNC_BIT,
                  (FAR void *)inarg, nitems(inarg),
                  NULL, 0, callback);
}
#endif /* CONFIG_LTE_LAPI_ENABLE_DEPRECATED_API */

