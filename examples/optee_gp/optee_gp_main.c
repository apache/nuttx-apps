/****************************************************************************
 * apps/examples/optee_gp/optee_gp_main.c
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
#include <nuttx/tee.h>
#include <tee_client_api.h>
#include <teec_trace.h>
#include <uuid.h>

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

/* This UUID is taken from the OP-TEE OS built-in pseudo TA:
 * https://github.com/OP-TEE/optee_os/blob/4.6.0/
 *   lib/libutee/include/pta_device.h
 */
#define PTA_DEVICE_ENUM_UUID                               \
    {                                                      \
      0x7011a688, 0xddde, 0x4053,                          \
        {                                                  \
            0xa5, 0xa9, 0x7b, 0x3c, 0x4d, 0xdf, 0x13, 0xb8 \
        }                                                  \
    }

#define PTA_CMD_GET_DEVICES 0x0

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * optee_gp_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  TEEC_Result res;
  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  TEEC_UUID uuid = PTA_DEVICE_ENUM_UUID;
  void *buf;
  TEEC_SharedMemory io_shm;
  uint32_t err_origin;
  unsigned int count;
  const uuid_t *raw_ta_uuid;
  uuid_t ta_uuid;
  char *ta_uuid_s;

  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS)
    {
      EMSG("TEEC_InitializeContext failed with code 0x%08x\n", res);
      goto exit;
    }

  memset(&op, 0, sizeof(op));

  /* Open a session with the devices pseudo TA */

  res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL,
                         &op, &err_origin);
  if (res != TEEC_SUCCESS)
    {
      EMSG("TEEC_Opensession failed with code 0x%08x origin 0x%08x", res,
           err_origin);
      goto exit_with_ctx;
    }

  /* Invoke command with NULL buffer to get required size */

  op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
                                   TEEC_NONE, TEEC_NONE);
  op.params[0].tmpref.buffer = NULL;
  op.params[0].tmpref.size = 0;

  res = TEEC_InvokeCommand(&sess, PTA_CMD_GET_DEVICES, &op, &err_origin);
  if (err_origin != TEEC_ORIGIN_TRUSTED_APP ||
      res != TEEC_ERROR_SHORT_BUFFER)
    {
      EMSG("TEEC_InvokeCommand failed: code 0x%08x origin 0x%08x",
          res, err_origin);
      goto exit_with_session;
    }

  /* Invoke command using temporary memory */

  op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE,
                                   TEEC_NONE, TEEC_NONE);

  op.params[0].tmpref.buffer = buf = malloc(op.params[0].tmpref.size);
  if (!op.params[0].tmpref.buffer)
    {
      EMSG("Failed to allocate %zu bytes of memory to share with TEE",
           op.params[0].tmpref.size);
      goto exit_with_session;
    }

  res = TEEC_InvokeCommand(&sess, PTA_CMD_GET_DEVICES, &op, &err_origin);
  if (res != TEEC_SUCCESS)
    {
      EMSG("TEEC_InvokeCommand failed: code 0x%08x origin 0x%08x",
          res, err_origin);
      goto exit_with_buf;
    }

  /* Invoke command using pre-allocated, pre-registered memory */

  io_shm.size = op.params[0].tmpref.size;
  io_shm.flags = TEEC_MEM_OUTPUT;
  res = TEEC_AllocateSharedMemory(&ctx, &io_shm);
  if (res != TEEC_SUCCESS)
    {
      EMSG("TEEC_AllocateSharedMemory failed: code 0x%08x", res);
      goto exit_with_buf;
    }

  op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE,
                                   TEEC_NONE);
  op.params[0].memref.parent = &io_shm;

  res = TEEC_InvokeCommand(&sess, PTA_CMD_GET_DEVICES, &op, &err_origin);
  if (res != TEEC_SUCCESS)
    {
      EMSG("TEEC_InvokeCommand failed: code 0x%08x origin 0x%08x",
          res, err_origin);
      goto exit_with_shm;
    }

  /* Sanity check that both outputs are the same */

  if (memcmp(buf, io_shm.buffer, io_shm.size))
    {
      EMSG("Different results with temp vs registered memory");
      goto exit_with_shm;
    }

  /* Print results to stdout */

  IMSG("Available devices:");

  count = io_shm.size / sizeof(uuid_t);
  raw_ta_uuid = (uuid_t *)io_shm.buffer;

  while (count--)
    {
      uuid_dec_be(raw_ta_uuid, &ta_uuid);
      uuid_to_string(&ta_uuid, &ta_uuid_s, NULL);

      IMSG("  %s", ta_uuid_s);

      free(ta_uuid_s);
      raw_ta_uuid++;
    }

exit_with_shm:
  TEEC_ReleaseSharedMemory(&io_shm);
exit_with_buf:
  free(buf);
exit_with_session:
  TEEC_CloseSession(&sess);
exit_with_ctx:
  TEEC_FinalizeContext(&ctx);
exit:
  return res;
}
