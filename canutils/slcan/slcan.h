/****************************************************************************
 * apps/canutils/slcan/slcan.h
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

#ifndef SLCAN_H
#define SLCAN_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* S6   - CAN speed 500 kBit/s
 * S8   - CAN speed 1   Mbit/s
 * O    - open channel
 * C    - close channel
 * C
 * S8
 * O
 * F  -> return status flags
 */
#define SLCAN_REC_FIFO_FULL    (1 << 0)
#define SLCAN_SND_FIFO_FULL    (1 << 1)
#define SLCAN_ERROR_WARN       (1 << 2)
#define SLCAN_DATA_OVERRUN     (1 << 3)
#define SLCAN_ERROR_PASSIVE    (1 << 5)
#define SLCAN_ARBITRATION_LOST (1 << 6)
#define SLCAN_BUS_ERROR        (1 << 7)

#endif /* SLCAN_H */
