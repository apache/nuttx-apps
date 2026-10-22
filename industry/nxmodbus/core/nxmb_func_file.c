/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_func_file.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/compiler.h>

#include <errno.h>
#include <stdint.h>

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Reference type of every sub-request (Modbus Application Protocol
 * Specification V1.1b3, sections 6.14 and 6.15).
 */

#define NXMB_FILE_REF_TYPE        0x06

/* A sub-request has the reference type, the file number, the starting record
 * and the record length: 1 + 2 + 2 + 2 bytes.
 */

#define NXMB_FILE_SUBREQ_LEN      7
#define NXMB_FILE_REF_OFF         0
#define NXMB_FILE_FILE_OFF        1
#define NXMB_FILE_RECORD_OFF      3
#define NXMB_FILE_LENGTH_OFF      5

/* A file has 10000 records (0 to 9999, 0x270f), each one a 16-bit register */

#define NXMB_FILE_RECORD_MAX      0x270f
#define NXMB_FILE_RECORDS         10000

/* FC20: byte count 0x07 to 0xf5, a group of up to 35 sub-requests */

#define NXMB_FC20_BCNT_MIN        0x07
#define NXMB_FC20_BCNT_MAX        0xf5
#define NXMB_FC20_SUBREQ_MAX      (NXMB_FC20_BCNT_MAX / NXMB_FILE_SUBREQ_LEN)

/* FC21: request data length 0x09 to 0xfb */

#define NXMB_FC21_BCNT_MIN        0x09
#define NXMB_FC21_BCNT_MAX        0xfb

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* One sub-request of a read, parsed before the response overwrites it */

struct nxmb_file_group_s
{
  uint16_t file;
  uint16_t record;
  uint16_t nregs;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_file_check_group
 *
 * Description:
 *   Check the file, record and length fields of a sub-request.  The
 *   reference type must be 6, the file number 1 to 0xffff, and the group
 *   must stay inside the 10000 records of the file.
 *
 * Input Parameters:
 *   sub   - The sub-request: reference type, file, record and length
 *   group - Receives the parsed file, record and length on success
 *
 * Returned Value:
 *   NXMB_EX_NONE on success; NXMB_EX_ILLEGAL_DATA_ADDRESS otherwise.
 *
 ****************************************************************************/

static enum nxmb_exception_e
nxmb_file_check_group(FAR const uint8_t *sub,
                      FAR struct nxmb_file_group_s *group)
{
  group->file   = nxmb_util_get_u16_be(&sub[NXMB_FILE_FILE_OFF]);
  group->record = nxmb_util_get_u16_be(&sub[NXMB_FILE_RECORD_OFF]);
  group->nregs  = nxmb_util_get_u16_be(&sub[NXMB_FILE_LENGTH_OFF]);

  if (sub[NXMB_FILE_REF_OFF] != NXMB_FILE_REF_TYPE || group->file == 0 ||
      group->record > NXMB_FILE_RECORD_MAX || group->nregs == 0 ||
      (uint32_t)group->record + group->nregs > NXMB_FILE_RECORDS)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  return NXMB_EX_NONE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_fc20_read_file
 *
 * Description:
 *   Handle FC20 (0x14) Read File Record.  Every sub-request is a group of
 *   records of one file; the response carries one sub-response (its length,
 *   the reference type and the registers) per sub-request.  All sub-requests
 *   are checked and parsed before the first callback, because the response
 *   overwrites the request in ctx->adu.
 *
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc20_read_file(nxmb_handle_t ctx)
{
  struct nxmb_file_group_s group[NXMB_FC20_SUBREQ_MAX];
  enum nxmb_exception_e    ex;
  uint16_t                 data_len = ctx->adu.length - 2;
  uint16_t                 resp_len;
  uint16_t                 pos;
  uint8_t                  bcnt;
  uint8_t                  nsub;
  uint8_t                  i;
  int                      ret;

  if (data_len < 1)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  bcnt = ctx->adu.data[0];

  if (bcnt < NXMB_FC20_BCNT_MIN || bcnt > NXMB_FC20_BCNT_MAX ||
      (bcnt % NXMB_FILE_SUBREQ_LEN) != 0 || data_len != 1 + bcnt)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->file_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  nsub     = bcnt / NXMB_FILE_SUBREQ_LEN;
  resp_len = 0;

  for (i = 0; i < nsub; i++)
    {
      pos = 1 + i * NXMB_FILE_SUBREQ_LEN;
      ex  = nxmb_file_check_group(&ctx->adu.data[pos], &group[i]);
      if (ex != NXMB_EX_NONE)
        {
          return ex;
        }

      /* Sub-response: its length byte, the reference type, the registers */

      resp_len += 2 + group[i].nregs * 2;
      if (resp_len > NXMB_FC20_BCNT_MAX)
        {
          return NXMB_EX_ILLEGAL_DATA_VALUE;
        }
    }

  /* The response data length byte and the sub-responses must fit */

  if (1 + resp_len > NXMB_ADU_DATA_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  ctx->adu.fc      = NXMB_FC_READ_FILE;
  ctx->adu.data[0] = (uint8_t)resp_len;
  pos              = 1;

  for (i = 0; i < nsub; i++)
    {
      ctx->adu.data[pos]     = (uint8_t)(1 + group[i].nregs * 2);
      ctx->adu.data[pos + 1] = NXMB_FILE_REF_TYPE;

      ret = ctx->callbacks->file_cb(&ctx->adu.data[pos + 2], group[i].file,
                                    group[i].record, group[i].nregs,
                                    NXMB_REG_READ, ctx->callbacks->priv);
      if (ret != 0)
        {
          return nxmb_cb_ret_to_exception(ret);
        }

      pos += 2 + group[i].nregs * 2;
    }

  ctx->adu.length = (uint16_t)(2 + 1 + resp_len);
  return NXMB_EX_NONE;
}

/****************************************************************************
 * Name: nxmb_fc21_write_file
 *
 * Description:
 *   Handle FC21 (0x15) Write File Record.  The request is checked to the
 *   last byte before the first callback; the groups are then written in
 *   order, and the response is an echo of the request.  The specification
 *   does not make the groups atomic: a callback failure leaves the earlier
 *   groups written.
 *
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc21_write_file(nxmb_handle_t ctx)
{
  struct nxmb_file_group_s group;
  enum nxmb_exception_e    ex;
  uint16_t                 data_len = ctx->adu.length - 2;
  uint16_t                 pos;
  uint8_t                  bcnt;
  int                      ret;

  if (data_len < 1)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  bcnt = ctx->adu.data[0];

  if (bcnt < NXMB_FC21_BCNT_MIN || bcnt > NXMB_FC21_BCNT_MAX ||
      data_len != 1 + bcnt)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->file_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  /* First pass: every sub-request must be complete and valid, and they must
   * fill the request data exactly.
   */

  for (pos = 1; pos < data_len; )
    {
      if (data_len - pos < NXMB_FILE_SUBREQ_LEN)
        {
          return NXMB_EX_ILLEGAL_DATA_VALUE;
        }

      ex = nxmb_file_check_group(&ctx->adu.data[pos], &group);
      if (ex != NXMB_EX_NONE)
        {
          return ex;
        }

      pos += NXMB_FILE_SUBREQ_LEN;

      if (data_len - pos < group.nregs * 2)
        {
          return NXMB_EX_ILLEGAL_DATA_VALUE;
        }

      pos += group.nregs * 2;
    }

  /* Second pass: write the groups, the response echoes the request */

  for (pos = 1; pos < data_len; )
    {
      (void)nxmb_file_check_group(&ctx->adu.data[pos], &group);
      pos += NXMB_FILE_SUBREQ_LEN;

      ret = ctx->callbacks->file_cb(&ctx->adu.data[pos], group.file,
                                    group.record, group.nregs,
                                    NXMB_REG_WRITE, ctx->callbacks->priv);
      if (ret != 0)
        {
          return nxmb_cb_ret_to_exception(ret);
        }

      pos += group.nregs * 2;
    }

  return NXMB_EX_NONE;
}
