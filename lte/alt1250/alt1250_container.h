/****************************************************************************
 * apps/lte/alt1250/alt1250_container.h
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

#ifndef __APPS_LTE_ALT1250_ALT1250_CONTAINER_H
#define __APPS_LTE_ALT1250_ALT1250_CONTAINER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <nuttx/modem/alt1250.h>

#include "alt1250_postproc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONTAINER_SOCKETID(c)  ((c)->sock)
#define CONTAINER_CMDID(c)     ((c)->cmdid)
#define CONTAINER_ARGUMENT(c)  ((c)->inparam)
#define CONTAINER_ARGSIZE(c)   ((c)->inparamlen)
#define CONTAINER_RESPONSE(c)  ((c)->outparam)
#define CONTAINER_RESPSIZE(c)  ((c)->outparamlen)
#define CONTAINER_RESPRES(c)   ((c)->result)
#define CONTAINER_POSTPROC(c)  ((FAR struct postproc_s *)(c)->priv)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void init_containers(void);

FAR struct alt_container_s *container_alloc(void);

void clear_container(FAR struct alt_container_s *container);

void set_container_ids(FAR struct alt_container_s *container,
                       int16_t usockid, uint32_t cmdid);

void set_container_argument(FAR struct alt_container_s *container,
                            FAR void *inparams[], size_t paramsz);

void set_container_response(FAR struct alt_container_s *container,
                            FAR void *outparams[], size_t paramsz);

void set_container_postproc(FAR struct alt_container_s *container,
                            FAR postproc_hdlr_t func, unsigned long priv);

void container_free(FAR struct alt_container_s *container);
void container_free_all(FAR struct alt_container_s *head);

FAR struct alt_container_s *
    container_pick_listtop(FAR struct alt_container_s **head);

#endif /* __APPS_LTE_ALT1250_ALT1250_CONTAINER_H */
