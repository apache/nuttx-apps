/****************************************************************************
 * apps/lte/alt1250/alt1250_atcmd.h
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

#ifndef __APPS_LTE_ALT1250_ALT1250_ATCMD_H
#define __APPS_LTE_ALT1250_ALT1250_ATCMD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

#include <nuttx/net/usrsock.h>

#include "alt1250_dbg.h"
#include "alt1250_devif.h"
#include "alt1250_container.h"

#include <lte/lte_lwm2m.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef int (*atreply_parser_t)(FAR char *reply, int len, void *arg);
typedef int (*atcmd_postproc_t)(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                FAR char *rdata, int len, unsigned long arg,
                                FAR int32_t *usock_result);

struct atreply_truefalse_s
{
  FAR const char *target_str;
  bool result;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int atcmdreply_ok_error(FAR struct alt1250_s *dev,
                        FAR struct alt_container_s *reply,
                        FAR char *rdata, int len, unsigned long arg,
                        FAR int32_t *usock_result);

int check_atreply_ok(FAR char *reply, int len, void *arg);
int check_atreply_truefalse(FAR char *reply, int len, void *arg);

int lwm2mstub_send_reset(FAR struct alt1250_s *dev,
                         FAR struct alt_container_s *container);

int lwm2mstub_send_getenable(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             FAR int32_t *usock_result);

int lwm2mstub_send_setenable(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container, bool en);

int lwm2mstub_send_getnamemode(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container);

int lwm2mstub_send_setnamemode(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               int mode);

int lwm2mstub_send_getversion(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container);

int lwm2mstub_send_setversion(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              bool is_v1_1);

int lwm2mstub_send_getwriteattr(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container);

int lwm2mstub_send_setwriteattr(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                bool en);

int lwm2mstub_send_getautoconnect(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *container);

int lwm2mstub_send_setautoconnect(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *container,
                                  bool en);

int ltenwop_send_getnwop(FAR struct alt1250_s *dev,
                         FAR struct alt_container_s *container);

int ltenwop_send_setnwoptp(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container);

int ltesp_send_getscanplan(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container);

int ltesp_send_setscanplan(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           bool enable);

int lwm2mstub_send_getqueuemode(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures);

int lwm2mstub_send_setqueuemode(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures, int en);

int lwm2mstub_send_m2mopev(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           int16_t usockid, FAR int32_t *ures, bool en);

int lwm2mstub_send_m2mobjcmd(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures, bool en);

int lwm2mstub_send_m2mev(FAR struct alt1250_s *dev,
                         FAR struct alt_container_s *container,
                         int16_t usockid, FAR int32_t *ures, bool en);

int lwm2mstub_send_getepname(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, atcmd_postproc_t proc,
                             unsigned long arg, FAR int32_t *ures);

int lwm2mstub_send_getsrvinfo(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, atcmd_postproc_t proc,
                              unsigned long arg, FAR int32_t *ures);

int lwm2mstub_send_getresource(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               int16_t usockid, atcmd_postproc_t proc,
                               unsigned long arg, FAR int32_t *ures,
                               FAR char *resource);

int lwm2mstub_send_getsupobjs(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, atcmd_postproc_t proc,
                              unsigned long arg, FAR int32_t *ures);

int lwm2mstub_send_getobjdef(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, atcmd_postproc_t proc,
                             unsigned long arg, FAR int32_t *ures,
                             uint16_t objid);

int lwm2mstub_send_setepname(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures,
                             FAR const char *epname);

int lwm2mstub_send_bsstart(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           int16_t usockid, atcmd_postproc_t proc,
                           unsigned long arg, FAR int32_t *ures);

int lwm2mstub_send_bsdelete(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *container,
                            int16_t usockid, atcmd_postproc_t proc,
                            unsigned long arg, FAR int32_t *ures);

int lwm2mstub_send_bscreateobj0(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, atcmd_postproc_t proc,
                                unsigned long arg, FAR int32_t *ures,
                                FAR struct lwm2mstub_serverinfo_s *info);

int lwm2mstub_send_bscreateobj1(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, atcmd_postproc_t proc,
                                unsigned long arg, FAR int32_t *ures,
                                FAR struct lwm2mstub_serverinfo_s *info);

int lwm2mstub_send_bsdone(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *container,
                          int16_t usockid, FAR int32_t *ures);

int lwm2mstub_send_setsupobjs(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, FAR int32_t *ures,
                              FAR uint16_t *objids, int objnum);

int lwm2mstub_send_setobjdef(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures,
                             uint16_t objid, int resnum,
                             FAR struct lwm2mstub_resource_s *resucs);

int lwm2mstub_send_registration(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures, int cmd);

int lwm2mstub_send_evrespwvalue(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures,
                                int seq_no, int resp,
                                FAR struct lwm2mstub_instance_s *inst,
                                FAR char *retval);

int lwm2mstub_send_evresponse(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, FAR int32_t *ures, int seq_no,
                              int resp,
                              FAR struct lwm2mstub_instance_s *inst);

int lwm2mstub_send_evrespwoinst(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures,
                                int seq_no, int resp);

int lwm2mstub_send_objevent(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *container,
                            int16_t usockid, FAR int32_t *ures,
                            FAR char *token,
                            FAR struct lwm2mstub_instance_s *inst,
                            FAR char *retval);

int lwm2mstub_send_changerat(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures, int rat);

int lwm2mstub_send_getrat(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *container,
                          int16_t usockid, atcmd_postproc_t proc,
                          unsigned long arg, FAR int32_t *ures);

#endif  /* __APPS_LTE_ALT1250_ALT1250_ATCMD_H */
