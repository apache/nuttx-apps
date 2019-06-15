/****************************************************************************
 * apps/industry/abnt_codi/abnt_codi.c
 *
 * Copyright (C) 2019 Alan Carvalho de Assis <acassis@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
