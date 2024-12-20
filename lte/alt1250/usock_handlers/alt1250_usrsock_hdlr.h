/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_usrsock_hdlr.h
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

#ifndef __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_SOCKETHDLR_H
#define __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_SOCKETHDLR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "alt1250_daemon.h"
#include "alt1250_usockif.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define COMBINE_ERRCODE(res, ecode) (((res) < 0) ? -(ecode) : (res))

#define ALT1250_NETIF_READY_DELAY (150 * 1000)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void reset_fwupdate_info(FAR struct alt1250_s *dev);

int usockreq_socket(FAR struct alt1250_s *dev,
                    FAR struct usrsock_request_buff_s *req,
                    FAR int32_t *usock_result,
                    FAR uint32_t *usock_xid,
                    FAR struct usock_ackinfo_s *ackinfo);

int usockreq_close(FAR struct alt1250_s *dev,
                   FAR struct usrsock_request_buff_s *req,
                   FAR int32_t *usock_result,
                   FAR uint32_t *usock_xid,
                   FAR struct usock_ackinfo_s *ackinfo);

int usockreq_connect(FAR struct alt1250_s *dev,
                     FAR struct usrsock_request_buff_s *req,
                     FAR int32_t *usock_result,
                     FAR uint32_t *usock_xid,
                     FAR struct usock_ackinfo_s *ackinfo);

int usockreq_sendto(FAR struct alt1250_s *dev,
                    FAR struct usrsock_request_buff_s *req,
                    FAR int32_t *usock_result,
                    FAR uint32_t *usock_xid,
                    FAR struct usock_ackinfo_s *ackinfo);

int usockreq_recvfrom(FAR struct alt1250_s *dev,
                      FAR struct usrsock_request_buff_s *req,
                      FAR int32_t *usock_result,
                      FAR uint32_t *usock_xid,
                      FAR struct usock_ackinfo_s *ackinfo);

int usockreq_setsockopt(FAR struct alt1250_s *dev,
                        FAR struct usrsock_request_buff_s *req,
                        FAR int32_t *usock_result,
                        FAR uint32_t *usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo);

int usockreq_getsockopt(FAR struct alt1250_s *dev,
                        FAR struct usrsock_request_buff_s *req,
                        FAR int32_t *usock_result,
                        FAR uint32_t *usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo);

int usockreq_getsockname(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo);

int usockreq_getpeername(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo);

int usockreq_bind(FAR struct alt1250_s *dev,
                  FAR struct usrsock_request_buff_s *req,
                  FAR int32_t *usock_result,
                  FAR uint32_t *usock_xid,
                  FAR struct usock_ackinfo_s *ackinfo);

int usockreq_listen(FAR struct alt1250_s *dev,
                    FAR struct usrsock_request_buff_s *req,
                    FAR int32_t *usock_result,
                    FAR uint32_t *usock_xid,
                    FAR struct usock_ackinfo_s *ackinfo);

int usockreq_accept(FAR struct alt1250_s *dev,
                    FAR struct usrsock_request_buff_s *req,
                    FAR int32_t *usock_result,
                    FAR uint32_t *usock_xid,
                    FAR struct usock_ackinfo_s *ackinfo);

int usockreq_ioctl(FAR struct alt1250_s *dev,
                   FAR struct usrsock_request_buff_s *req,
                   FAR int32_t *usock_result,
                   FAR uint32_t *usock_xid,
                   FAR struct usock_ackinfo_s *ackinfo);

int usockreq_shutdown(FAR struct alt1250_s *dev,
                      FAR struct usrsock_request_buff_s *req,
                      FAR int32_t *usock_result,
                      FAR uint32_t *usock_xid,
                      FAR struct usock_ackinfo_s *ackinfo);

int open_altsocket(FAR struct alt1250_s *dev,
                    FAR struct alt_container_s *container,
                    FAR struct usock_s *usock,
                    FAR int32_t *usock_result);

int nextstep_connect(FAR struct alt1250_s *dev,
                     FAR struct alt_container_s *reply,
                     FAR struct usock_s *usock,
                     FAR int32_t *usock_result,
                     FAR uint32_t *usock_xid,
                     FAR struct usock_ackinfo_s *ackinfo,
                     unsigned long arg);

int nextstep_check_connectres(FAR struct alt1250_s *dev,
                              FAR struct usock_s *usock);

int nextstep_bind(FAR struct alt1250_s *dev,
                  FAR struct alt_container_s *reply,
                  FAR struct usock_s *usock,
                  FAR int32_t *usock_result,
                  FAR uint32_t *usock_xid,
                  FAR struct usock_ackinfo_s *ackinfo,
                  unsigned long arg);

int nextstep_listen(FAR struct alt1250_s *dev,
                    FAR struct alt_container_s *reply,
                    FAR struct usock_s *usock,
                    FAR int32_t *usock_result,
                    FAR uint32_t *usock_xid,
                    FAR struct usock_ackinfo_s *ackinfo,
                    unsigned long arg);

int nextstep_setsockopt(FAR struct alt1250_s *dev,
                        FAR struct alt_container_s *reply,
                        FAR struct usock_s *usock,
                        FAR int32_t *usock_result,
                        FAR uint32_t *usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo,
                        unsigned long arg);

int nextstep_getsockopt(FAR struct alt1250_s *dev,
                        FAR struct alt_container_s *reply,
                        FAR struct usock_s *usock,
                        FAR int32_t *usock_result,
                        FAR uint32_t *usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo,
                        unsigned long arg);

int nextstep_getsockname(FAR struct alt1250_s *dev,
                         FAR struct alt_container_s *reply,
                         FAR struct usock_s *usock,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo,
                         unsigned long arg);

int postproc_sockcommon(FAR struct alt1250_s *dev,
                        FAR struct alt_container_s *reply,
                        FAR struct usock_s *usock,
                        FAR int32_t *usock_result,
                        FAR uint32_t *usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo,
                        unsigned long arg);

int send_getsockopt_command(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *container,
                            FAR struct usock_s *usock,
                            int16_t level,
                            int16_t option,
                            uint16_t valuelen,
                            FAR int16_t *requested_level,
                            FAR int16_t *requested_option,
                            FAR postproc_hdlr_t func,
                            unsigned long priv,
                            FAR int32_t *usock_result);

int send_reportnet_command(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           FAR struct usock_s *usock,
                           FAR postproc_hdlr_t func,
                           unsigned long priv,
                           FAR int32_t *usock_result);

int send_lapi_command(FAR struct alt1250_s *dev,
                      FAR struct alt_container_s *container,
                      FAR struct usock_s *usock,
                      FAR struct lte_ioctl_data_s *ltecmd,
                      FAR postproc_hdlr_t hdlr,
                      unsigned long priv,
                      FAR int32_t *usock_result);

int postproc_fwgetversion(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *reply,
                          FAR struct usock_s *usock,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo,
                          unsigned long arg);

void alt1250_geterrinfo(FAR lte_errinfo_t *errinfo);

#endif  /* __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_SOCKETHDLR_H */
