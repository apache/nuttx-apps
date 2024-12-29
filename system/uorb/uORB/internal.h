/****************************************************************************
 * apps/system/uorb/uORB/internal.h
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

#ifndef __APP_SYSTEM_UORB_UORB_INTERNAL_H
#define __APP_SYSTEM_UORB_UORB_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <uORB/uORB.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const struct orb_loop_ops_s g_orb_loop_epoll_ops;

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct orb_loop_ops_s
{
  CODE int (*init)(FAR struct orb_loop_s *loop);
  CODE int (*run)(FAR struct orb_loop_s *loop);
  CODE int (*uninit)(FAR struct orb_loop_s *loop);
  CODE int (*enable)(FAR struct orb_loop_s *loop,
                     FAR struct orb_handle_s *handle, bool en);
};

#endif /* __APP_SYSTEM_UORB_UORB_INTERNAL_H */
