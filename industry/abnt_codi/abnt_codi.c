/****************************************************************************
 * apps/industry/abnt_codi/abnt_codi.c
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <stdarg.h>

#include "industry/abnt_codi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uint8_t abnt_codi_checksum(FAR const uint8_t *data)
{
  uint8_t checksum = 0x00;
  int i;

  for (i = 0; i < 7; i++)
    {
      checksum ^= *data++;
    }

  /* Return the complement of the resulting checksum */

  checksum ^= 0xff;

  return checksum;
}

bool abnt_codi_parse(FAR const uint8_t *data,
                     FAR struct abnt_codi_proto_s *proto)
{
  uint8_t checksum;

  /* Checksum is the last byte in the CODI byte sequence */

  checksum = data[7];

  /* Verify if the checksum is correct */

  if (abnt_codi_checksum(data) == checksum)
    {
      printf("Checksum is correct, this is a valid CODI sequence!\n");
    }
  else
    {
      printf("Invalid checksum! Received: 0x%02x Expected: 0x%02x!\n",
             checksum, abnt_codi_checksum(data));

      return false;
    }

  /* Seconds missing to the end of current active demand */

  proto->end_act_dem = ((data[1] & END_ACT_DEM_MASK) << 8) | data[0];

  /* Bill indicator, it is inverted at end of active current demand */

  proto->bill_indicator = (data[1] & BILL_INDICADOR) == BILL_INDICADOR ?
                          true : false;

  /* Reactive indicator, it is invered at end of reactive interval */

  proto->react_interval = (data[1] & REACT_INTERVAL) == REACT_INTERVAL ?
                          true : false;

  /* Verify if capacitive pulses are used to calculate consumption */

  proto->react_cap_pulse = (data[1] & REACT_CAP_PULSE) == REACT_CAP_PULSE ?
                           true : false;

  /* Verify if inductive pulses are used to calculate consumption */

  proto->react_ind_pulse = (data[1] & REACT_IND_PULSE) == REACT_IND_PULSE ?
                           true : false;

  /* Seasonal segment type */

  proto->segment_type = data[2] & CHARGES_MASK;

  /* Tariff type */

  proto->charge_type = data[2] & CHARGES_MASK;

  /* Verify if Reactive Tariff is enable */

  proto->react_charge_en = (data[2] & CHARGES_REACT_EN) == CHARGES_REACT_EN ?
                           true : false;

  /* Number of active pulses since the beginning of current demand */

  proto->pulses_act_dem = ((data[4] & ~NOT_USED_BIT)) << 8 | data[3];

  /* Number of reactive pulses since the beginning of current demand */

  proto->pulses_react_dem = ((data[6] & ~NOT_USED_BIT)) << 8 | data[5];

  /* Save the checksum too */

  proto->checksum = checksum;

  return true;
}
