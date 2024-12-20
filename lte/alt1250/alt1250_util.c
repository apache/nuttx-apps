/****************************************************************************
 * apps/lte/alt1250/alt1250_util.c
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

#include <string.h>
#include <nuttx/modem/alt1250.h>

#include "alt1250_daemon.h"
#include "alt1250_util.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: alt1250_saveapn
 ****************************************************************************/

void alt1250_saveapn(FAR struct alt1250_s *dev, FAR lte_apn_setting_t *apn)
{
  memcpy(&dev->apn, apn, sizeof(lte_apn_setting_t));
  strncpy(dev->apn_name, (FAR const char *)apn->apn, LTE_APN_LEN);
  if ((apn->auth_type != LTE_APN_AUTHTYPE_NONE) && (apn->user_name))
    {
      strncpy(dev->user_name, (FAR const char *)apn->user_name,
              LTE_APN_USER_NAME_LEN);
    }

  if ((apn->auth_type != LTE_APN_AUTHTYPE_NONE) && (apn->password))
    {
      strncpy(dev->pass, (FAR const char *)apn->password,
              LTE_APN_PASSWD_LEN);
    }

  dev->apn.apn = dev->apn_name;
  dev->apn.user_name = dev->user_name;
  dev->apn.password = dev->pass;
}

/****************************************************************************
 * Name: alt1250_getapn
 ****************************************************************************/

void alt1250_getapn(FAR struct alt1250_s *dev, FAR lte_apn_setting_t *apn)
{
  memcpy(apn, &dev->apn, sizeof(lte_apn_setting_t));
}
