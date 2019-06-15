/****************************************************************************
 * apps/include/industry/abnt_codi.h
 *
 * Copyright © 2019 Alan Carvalho de Assis
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

#ifndef __APPS_INCLUDE_INDUSTRY_ABNTCODI_H
#define __APPS_INCLUDE_INDUSTRY_ABNTCODI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define boolstr(s) ((s) ? "true" : "false")

#define END_ACT_DEM_MASK   0x0f
#define BILL_INDICADOR     (1 << 4) /* This bit is inverted at each
                                     * replenishment of demand */
#define REACT_INTERVAL     (1 << 5) /* Reactive Interval Indicator, inverted
                                     * at end of reactive interval
                                     * consumption */
#define REACT_CAP_PULSE    (1 << 6) /* If equal 1 means that reactive
                                     * capacitive pulses are computed to
                                     * calculate UFER and DMCR */
#define REACT_IND_PULSE    (1 << 7) /* If equal 1 means that reactive
                                     * inductive pulses are computed to
                                     * calculate UFER and DMCR */

#define SEGMENT_MASK       0x0f
#define SEGMENT_PEEK       1        /* Current segment is in the peek */
#define SEGMENT_OUT_PEEK   2        /* Current segment is out of the peek */
#define SEGMENT_RESERVED   8        /* Reserved */

#define CHARGES_MASK       (3 << 4)
#define CHARGES_BLUE       (0 << 4) /* The charges is blue */
#define CHARGES_GREEN      (1 << 4) /* The charges is green */
#define CHARGES_IRRIGATORS (2 << 4) /* The charges is irrigators */
#define CHARGES_OTHER      (3 << 4) /* The is other */
                                    /* bit 6 not used */
#define CHARGES_REACT_EN   (1 << 7) /* Reactive charging is enabled */

#define NOT_USED_BIT       (1 << 7) /* Bit 7 of 5th and 7th octet are not used */

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct abnt_codi_proto_s
{
  uint16_t end_act_dem;      /* Seconds missing to end of the active demand */
  bool     bill_indicator;   /* Bill indicator alternating bit */
  bool     react_interval;   /* Reactive indicator alternating bit */
  bool     react_cap_pulse;  /* Indicates if capacitive pulses are used to
                              * calculate consumption */
  bool     react_ind_pulse;  /* Indicates if inductive pulses are used to
                              * calculate consumption */
  uint8_t  segment_type;     /* Segment type */
  uint8_t  charge_type;      /* Charging type */
  bool     react_charge_en;  /* Reactive charging enable */
  uint16_t pulses_act_dem;   /* Number of active pulses since the beginning
                              * of current demand */
  uint16_t pulses_react_dem; /* Number of reactive pulses since the beginning
                              * of current demand */
  uint8_t  checksum;         /* Checksum: xor of previous 7 bytes and
                              * complement (bits inversion) the result */
};

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Calculate raw sentence checksum. Does not check sentence integrity. */

uint8_t abnt_codi_checksum(FAR const uint8_t *data);

/* Parse a specific ABNT CODI sequence. */

bool abnt_codi_parse(FAR const uint8_t *data, FAR struct abnt_codi_proto_s *proto);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_INDUSTRY_ABNTCODI_H */
