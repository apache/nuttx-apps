/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_subhdlr.h
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

#ifndef __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_IOCTL_SUBHDLR_H
#define __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_IOCTL_SUBHDLR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "alt1250_daemon.h"
#include "alt1250_usockif.h"
#include "alt1250_usockevent.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int usockreq_ioctl_ltecmd(FAR struct alt1250_s *dev,
                          FAR struct usrsock_request_buff_s *req,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl_ifreq(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl_normal(FAR struct alt1250_s *dev,
                          FAR struct usrsock_request_buff_s *req,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl_event(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl_other(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl_power(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl_fwupdate(FAR struct alt1250_s *dev,
                            FAR struct usrsock_request_buff_s *req,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl_denyinetsock(FAR struct alt1250_s *dev,
                                FAR struct usrsock_request_buff_s *req,
                                FAR int32_t *usock_result,
                                FAR uint32_t *usock_xid,
                                FAR struct usock_ackinfo_s *ackinfo);

int send_m2mnotice_command(uint32_t cmdid,
                           FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           FAR struct usock_s *usock,
                           FAR struct lte_ioctl_data_s *ltecmd,
                           FAR int32_t *ures);

int usockreq_ioctl_lwm2m(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo);

#endif  /* __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_IOCTL_SUBHDLR_H */
