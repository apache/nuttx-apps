/****************************************************************************
 * apps/testing/crypto/psa-arch-tests/include/pal_storage_config.h
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
 ****************************************************************************/

#ifndef _PAL_STORAGE_CONFIG_H_
#define _PAL_STORAGE_CONFIG_H_

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Platform specific max UID's size */

#define ARCH_TEST_STORAGE_UID_MAX_SIZE        512
#define PSA_STORAGE_FLAG_NO_CONFIDENTIALITY   (1u << 1)
#define PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION (1u << 2)

#endif /* _PAL_STORAGE_CONFIG_H_ */
