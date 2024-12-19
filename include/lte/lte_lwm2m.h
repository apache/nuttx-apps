/****************************************************************************
 * apps/include/lte/lte_lwm2m.h
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

#ifndef __APPS_INCLUDE_LTE_LTE_LWM2M_H
#define __APPS_INCLUDE_LTE_LTE_LWM2M_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <nuttx/wireless/lte/lte.h>

#define LWM2MSTUB_RESOURCE_HANDLENOCARE (0)
#define LWM2MSTUB_RESOURCE_HANDLEHOST   (1)
#define LWM2MSTUB_RESOURCE_HANDLEMODEMH (2)

#define LWM2MSTUB_MAX_WRITE_SIZE (1500)
#define LWM2MSTUB_MAX_TOKEN_SIZE (8 * 2 + 1)

#define LWM2MSTUB_MAX_SERVER_NAME (256)
#define LWM2MSTUB_MAX_DEVID       (256)
#define LWM2MSTUB_MAX_SEQKEY      (256)

#define LWM2MSTUB_CONDVALID_MINPERIOD  (1<<0)
#define LWM2MSTUB_CONDVALID_MAXPERIOD  (1<<1)
#define LWM2MSTUB_CONDVALID_GRATERTHAN (1<<2)
#define LWM2MSTUB_CONDVALID_LESSTHAN   (1<<3)
#define LWM2MSTUB_CONDVALID_STEP       (1<<4)

#define LWM2MSTUB_FWUP_PEND_DL  (0)
#define LWM2MSTUB_FWUP_PEND_UPD (1)
#define LWM2MSTUB_FWUP_COMP_DL  (2)
#define LWM2MSTUB_FWUP_FAIL_DL  (3)
#define LWM2MSTUB_FWUP_CANCELED (4)

#define LWM2MSTUB_CMD_REGISTER      (0)
#define LWM2MSTUB_CMD_DEREGISTER    (1)
#define LWM2MSTUB_CMD_UPDATERESIGER (2)

#define LWM2MSTUB_STATE_NOTREGISTERD    (0)
#define LWM2MSTUB_STATE_REGISTPENDING   (1)
#define LWM2MSTUB_STATE_REGISTERD       (2)
#define LWM2MSTUB_STATE_REGISTERFAILED  (3)
#define LWM2MSTUB_STATE_UPDATEPENDING   (4)
#define LWM2MSTUB_STATE_DEREGISTPENDING (5)
#define LWM2MSTUB_STATE_BSHOLDOFF       (6)
#define LWM2MSTUB_STATE_BSREQUESTED     (7)
#define LWM2MSTUB_STATE_BSONGOING       (8)
#define LWM2MSTUB_STATE_BSDONE          (9)
#define LWM2MSTUB_STATE_BSFAILED        (10)

#define LWM2MSTUB_RESOP_READ  (0)
#define LWM2MSTUB_RESOP_WRITE (1)
#define LWM2MSTUB_RESOP_RW    (2)
#define LWM2MSTUB_RESOP_EXEC  (3)

#define LWM2MSTUB_RESINST_SINGLE (0)
#define LWM2MSTUB_RESINST_MULTI  (1)

#define LWM2MSTUB_RESDATA_NONE     (0)
#define LWM2MSTUB_RESDATA_STRING   (1)
#define LWM2MSTUB_RESDATA_INT      (2)
#define LWM2MSTUB_RESDATA_UNSIGNED (3)
#define LWM2MSTUB_RESDATA_FLOAT    (4)
#define LWM2MSTUB_RESDATA_BOOL     (5)
#define LWM2MSTUB_RESDATA_OPAQUE   (6)
#define LWM2MSTUB_RESDATA_TIME     (7)
#define LWM2MSTUB_RESDATA_OBJLINK  (8)

#define LWM2MSTUB_SECUREMODE_PSK     (0)
#define LWM2MSTUB_SECUREMODE_RPK     (1)
#define LWM2MSTUB_SECUREMODE_CERT    (2)
#define LWM2MSTUB_SECUREMODE_NOSEC   (3)
#define LWM2MSTUB_SECUREMODE_CERTEST (4)

#define LWM2MSTUB_CONNECT_REGISTER   (0)
#define LWM2MSTUB_CONNECT_DEREGISTER (1)
#define LWM2MSTUB_CONNECT_REREGISTER (2)
#define LWM2MSTUB_CONNECT_BOOTSTRAP  (3)

#define LWM2MSTUB_RESP_CHANGED       (0)
#define LWM2MSTUB_RESP_CONTENT       (1)
#define LWM2MSTUB_RESP_BADREQ        (2)
#define LWM2MSTUB_RESP_UNAUTH        (3)
#define LWM2MSTUB_RESP_NOURI         (4)
#define LWM2MSTUB_RESP_NOTALLOW      (5)
#define LWM2MSTUB_RESP_NOTACCEPT     (6)
#define LWM2MSTUB_RESP_UNSUPPORT     (7)
#define LWM2MSTUB_RESP_INTERNALERROR (8)

/* Client received "Write" operation */

#define LWM2MSTUB_OP_WRITE      (0)

/* Client received "Execute" operation */

#define LWM2MSTUB_OP_EXEC       (1)

/* Client received "Write Attributes" operation */

#define LWM2MSTUB_OP_WATTR      (4)

/* Client received "Discover" operation */

#define LWM2MSTUB_OP_DISCOVER   (5)

/* Client received "Read" operation */

#define LWM2MSTUB_OP_READ       (6)

/* Client received "Observe" operation */

#define LWM2MSTUB_OP_OBSERVE    (7)

/* Client received "Cancel observation" operation */

#define LWM2MSTUB_OP_CANCELOBS  (8)

/* Client is offline now. */

#define LWM2MSTUB_OP_OFFLINE    (9)

/* Client is online now. */

#define LWM2MSTUB_OP_ONLINE     (10)

/* Client sent observation notification to a server. */

#define LWM2MSTUB_OP_SENDNOTICE (11)

/* Client received wakeup SMS. */

#define LWM2MSTUB_OP_RCVWUP     (12)

/* Client received notification acknowledge. */

#define LWM2MSTUB_OP_RCVOBSACK  (13)

/* Client ON: LMM2M client exits Client OFF state
 * and tries to re-connect server due to explicitly
 * AT Command registration request.
 */

#define LWM2MSTUB_OP_CLIENTON   (14)

/* Client OFF: LWM2M client has exhausted server connection retries. */

#define LWM2MSTUB_OP_CLIENTOFF  (15)

/* Confirmable NOTIFY failed. */

#define LWM2MSTUB_OP_FAILNOTIFY  (16)

/* Bootstrap finished and completed successfully. */

#define LWM2MSTUB_OP_BSFINISH  (20)

/* Registration finished and completed successfully.
 * all server observation requests are cleaned,
 * the host should clean host objects observation rules too.
 */

#define LWM2MSTUB_OP_REGSUCCESS  (21)

/* Register update finished and completed successfully. */

#define LWM2MSTUB_OP_REGUPDATED  (22)

/* De-register finished and completed successfully. */

#define LWM2MSTUB_OP_DEREGSUCCESS  (23)

/* Notification was not saved and not sent to server */

#define LWM2MSTUB_OP_NOSENDNOTICE  (24)

struct lwm2mstub_resource_s
{
  int res_id;
  int operation;
  int inst_type;
  int data_type;
  int handl;
};

struct lwm2mstub_instance_s
{
  int object_id;
  int object_inst;
  int res_id;
  int res_inst;
};

struct lwm2mstub_ovcondition_s
{
    uint8_t valid_mask;
    unsigned int min_period;
    unsigned int max_period;
    double gt_cond;
    double lt_cond;
    double step_val;
};

struct lwm2mstub_serverinfo_s
{
  int object_inst;
  int state;
  bool bootstrap;
  bool nonip;
  int security_mode;
  uint32_t lifetime;
  char server_uri[LWM2MSTUB_MAX_SERVER_NAME];
  char device_id[LWM2MSTUB_MAX_DEVID];
  char security_key[LWM2MSTUB_MAX_SEQKEY];
};

typedef CODE void (*lwm2mstub_write_cb_t)(int seq_no, int srv_id,
              FAR struct lwm2mstub_instance_s *inst, FAR char *value,
              int len);

typedef CODE void (*lwm2mstub_read_cb_t)(int seq_no, int srv_id,
              FAR struct lwm2mstub_instance_s *inst);

typedef CODE void (*lwm2mstub_exec_cb_t)(int seq_no, int srv_id,
              FAR struct lwm2mstub_instance_s *inst);

typedef CODE void (*lwm2mstub_ovstart_cb_t)(int seq_no, int srv_id,
              FAR struct lwm2mstub_instance_s *inst, FAR char *token,
              FAR struct lwm2mstub_ovcondition_s *cond);

typedef CODE void (*lwm2mstub_ovstop_cb_t)(int seq_no, int srv_id,
              FAR struct lwm2mstub_instance_s *inst, FAR char *token);

typedef CODE void (*lwm2mstub_operation_cb_t)(int event, int srv_id,
                   FAR struct lwm2mstub_instance_s *inst);

typedef CODE void (*lwm2mstub_fwupstate_cb_t)(int event);

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* On powe on state */

int lte_setm2m_endpointname(FAR char *name);
int lte_getm2m_endpointname(FAR char *name, int len);

int lte_getm2m_servernum(void);
int lte_setm2m_serverinfo(FAR struct lwm2mstub_serverinfo_s *info, int id);
int lte_getm2m_serverinfo(FAR struct lwm2mstub_serverinfo_s *info, int id);

int lte_getm2m_enabled_objectnum(void);
int lte_getm2m_enabled_objects(FAR uint16_t *objids, int objnum);
int lte_enablem2m_objects(FAR uint16_t *objids, int objnum);

int lte_getm2m_objresourcenum(uint16_t objid);
int lte_getm2m_objresourceinfo(uint16_t objids, int res_num,
                                FAR struct lwm2mstub_resource_s *reses);
int lte_setm2m_objectdefinition(uint16_t objids, int res_num,
                                FAR struct lwm2mstub_resource_s *reses);
bool lte_getm2m_qmode(void);
int lte_setm2m_qmode(bool en);

int lte_apply_m2msetting(void);

/* After attached */

int lte_m2m_connection(int cmd);

int lte_set_report_m2mwrite(lwm2mstub_write_cb_t cb);
int lte_set_report_m2mread(lwm2mstub_read_cb_t cb);
int lte_set_report_m2mexec(lwm2mstub_exec_cb_t cb);
int lte_set_report_m2movstart(lwm2mstub_ovstart_cb_t cb);
int lte_set_report_m2movstop(lwm2mstub_ovstop_cb_t cb);
int lte_set_report_m2moperation(lwm2mstub_operation_cb_t cb);
int lte_set_report_m2mfwupdate(lwm2mstub_fwupstate_cb_t cb);

int lte_m2m_readresponse(int seq_no,
                         FAR struct lwm2mstub_instance_s *inst,
                         int resp, FAR char *readvalue, int len);
int lte_m2m_writeresponse(int seq_no,
                          FAR struct lwm2mstub_instance_s *inst,
                          int resp);
int lte_m2m_executeresp(int seq_no,
                        FAR struct lwm2mstub_instance_s *inst,
                        int resp);
int lte_m2m_observeresp(int seq_no, int resp);

int lte_m2m_observeupdate(FAR char *token,
                          FAR struct lwm2mstub_instance_s *inst,
                          FAR char *value, int len);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif  /* __APPS_INCLUDE_LTE_LTE_LWM2M_H */
