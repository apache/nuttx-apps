/****************************************************************************
 * apps/examples/optee/optee_main.c
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
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <nuttx/tee.h>
#include <uuid.h>

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

#undef USE_ALLOC_IOC

#ifdef USE_ALLOC_IOC
#  define tee_shm_alloc          tee_shm_mmap
#  define tee_shm_free_buf(p, s) munmap(p, s)
#else
#  define tee_shm_alloc          tee_shm_malloc
#  define tee_shm_free_buf(p, s) ((void)s, free(p))
#endif

#define OPTEE_DEV                 "/dev/tee0"

#define PTA_DEVICE_ENUM           { 0x7011a688, 0xddde, 0x4053, \
                                    0xa5, 0xa9, \
                                    { 0x7b, 0x3c, 0x4d, 0xdf, 0x13, 0xb8 } }

#define PTA_CMD_GET_DEVICES       0x0

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct tee_shm
{
  int fd;
  size_t size;
  void *ptr;
  int32_t id;
} tee_shm_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int tee_check_version_and_caps(int fd, uint32_t *caps)
{
  struct tee_ioctl_version_data ioc_ver;
  int ret;

  ret = ioctl(fd, TEE_IOC_VERSION, (unsigned long)&ioc_ver);
  if (ret < 0)
    {
      printf("Failed to query TEE driver version and caps: %d, %s\n",
             ret, strerror(errno));
      return ret;
    }

  if (ioc_ver.impl_id != TEE_IMPL_ID_OPTEE)
    {
      printf("Not an OP-TEE implementation\n");
      return -ENOTSUP;
    }

  if (!(ioc_ver.impl_caps & TEE_OPTEE_CAP_TZ))
    {
      printf("OP-TEE TrustZone not supported\n");
      return -ENOTSUP;
    }

  if (((TEE_GEN_CAP_GP | TEE_GEN_CAP_MEMREF_NULL) & ioc_ver.gen_caps) !=
       (TEE_GEN_CAP_GP | TEE_GEN_CAP_MEMREF_NULL))
    {
      printf("CAP_GP or MEMREF_NULL not supported\n");
      return -ENOTSUP;
    }

  printf("impl id: %u, impl caps: %u, gen caps: %u\n",
         ioc_ver.impl_id, ioc_ver.impl_caps, ioc_ver.gen_caps);

  if (caps)
    {
      *caps = ioc_ver.gen_caps;
    }

  return ret;
}

static int tee_open_session(int fd, const uuid_t *uuid, uint32_t *session)
{
  struct tee_ioctl_open_session_arg ioc_opn;
  struct tee_ioctl_buf_data ioc_buf;
  int ret;

  memset(&ioc_opn, 0, sizeof(struct tee_ioctl_open_session_arg));

  uuid_enc_be(&ioc_opn.uuid, uuid);

  ioc_buf.buf_ptr = (uintptr_t)&ioc_opn;
  ioc_buf.buf_len = sizeof(struct tee_ioctl_open_session_arg);

  ret = ioctl(fd, TEE_IOC_OPEN_SESSION, (unsigned long)&ioc_buf);
  if (ret < 0)
    {
      return ret;
    }

  if (session)
    {
      *session = ioc_opn.session;
    }

  return ret;
}

static int tee_invoke(int fd, uint32_t session, uint32_t func,
                      struct tee_ioctl_param *params, size_t num_params)
{
  struct tee_ioctl_invoke_arg *ioc_args;
  struct tee_ioctl_buf_data ioc_buf;
  size_t ioc_args_len;
  int ret;

  ioc_args_len = sizeof(struct tee_ioctl_invoke_arg) +
                 TEE_IOCTL_PARAM_SIZE(num_params);

  ioc_args = (struct tee_ioctl_invoke_arg *)calloc(1, ioc_args_len);
  if (!ioc_args)
    {
      return -ENOMEM;
    }

  ioc_args->func = func;
  ioc_args->session = session;
  ioc_args->num_params = num_params;
  memcpy(&ioc_args->params, params, TEE_IOCTL_PARAM_SIZE(num_params));

  ioc_buf.buf_ptr = (uintptr_t)ioc_args;
  ioc_buf.buf_len = ioc_args_len;

  ret = ioctl(fd, TEE_IOC_INVOKE, (unsigned long)&ioc_buf);
  if (ret < 0)
    {
      goto err_with_args;
    }

  memcpy(params, &ioc_args->params, TEE_IOCTL_PARAM_SIZE(num_params));

err_with_args:
  free(ioc_args);

  return ret;
}

static int tee_shm_register(int fd, tee_shm_t *shm)
{
  struct tee_ioctl_shm_register_data ioc_reg;

  memset(&ioc_reg, 0, sizeof(struct tee_ioctl_shm_register_data));

  if (!shm)
    {
      return -EINVAL;
    }

  ioc_reg.addr = (uintptr_t)shm->ptr;
  ioc_reg.length = shm->size;

  shm->fd = ioctl(fd, TEE_IOC_SHM_REGISTER, (unsigned long)&ioc_reg);
  shm->id = ioc_reg.id;

  return shm->fd < 0 ? shm->fd : 0;
}

#ifdef USE_ALLOC_IOC
static int tee_shm_mmap(int fd, tee_shm_t *shm, bool reg)
{
  struct tee_ioctl_shm_alloc_data ioc_alloc;
  int ret = 0;

  memset(&ioc_alloc, 0, sizeof(struct tee_ioctl_shm_alloc_data));

  if (!shm)
    {
      return -EINVAL;
    }

  ioc_alloc.size = shm->size;

  shm->fd = ioctl(fd, TEE_IOC_SHM_ALLOC, (unsigned long)&ioc_alloc);
  if (shm->fd < 0)
    {
      return shm->fd;
    }

  shm->ptr = mmap(NULL, shm->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                  shm->fd, 0);
  if (shm->ptr == MAP_FAILED)
    {
      close(shm->fd);
      return -ENOMEM;
    }

  if (reg)
    {
      ret = tee_shm_register(fd, shm);
      if (ret < 0)
        {
          munmap(shm->ptr, shm->size);
          close(shm->fd);
          return ret;
        }
    }

  return ret;
}

#else /* !USE_ALLOC_IOC */
static int tee_shm_malloc(int fd, tee_shm_t *shm, bool reg)
{
  int ret = 0;

  if (!shm)
    {
      return -EINVAL;
    }

  shm->ptr = malloc(shm->size);
  if (!shm->ptr)
    {
      return -ENOMEM;
    }

  shm->fd = -1;

  if (reg)
    {
      ret = tee_shm_register(fd, shm);
      if (ret < 0)
        {
          free(shm->ptr);
        }
    }

  return ret;
}
#endif /* !USE_ALLOC_IOC */

static void tee_shm_free(tee_shm_t *shm)
{
  if (!shm)
    {
      return;
    }

  tee_shm_free_buf(shm->ptr, shm->size);

  if (shm->fd >= 0)
    {
      close(shm->fd);
    }

  shm->ptr = NULL;
  shm->fd = -1;
}

static int tee_close_session(int fd, uint32_t session)
{
  struct tee_ioctl_close_session_arg ioc_close;
  int ret;

  ioc_close.session = session;

  ret = ioctl(fd, TEE_IOC_CLOSE_SESSION, (unsigned long)&ioc_close);

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * optee_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  uint32_t caps;
  const uuid_t pta_dvc_uuid = PTA_DEVICE_ENUM;
  uint32_t session;
  struct tee_ioctl_param par0;
  tee_shm_t shm;
  unsigned int count;
  const uuid_t *raw_ta_uuid;
  uuid_t ta_uuid;
  char *ta_uuid_s;
  int ret;

  memset(&par0, 0, sizeof(struct tee_ioctl_param));
  memset(&shm, 0, sizeof(tee_shm_t));

  fd = open(OPTEE_DEV, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      printf("Failed to open " OPTEE_DEV ": %s\n", strerror(errno));
      return -errno;
    }

  ret = tee_check_version_and_caps(fd, &caps);
  if (ret < 0)
    {
      goto err;
    }

  ret = tee_open_session(fd, &pta_dvc_uuid, &session);
  if (ret < 0)
    {
      printf("Failed to open session with devices.pta: %d, %s\n",
             ret, strerror(errno));
      goto err;
    }

  par0.attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
  par0.c = TEE_MEMREF_NULL;

  ret = tee_invoke(fd, session, PTA_CMD_GET_DEVICES, &par0, 1);
  if (ret < 0)
    {
      printf("Failed to get size needed for device enumeration: %d, %s\n",
             ret, strerror(errno));
      goto err_with_session;
    }

  shm.size = par0.b;

  ret = tee_shm_alloc(fd, &shm, caps & TEE_GEN_CAP_REG_MEM);
  if (ret < 0)
    {
      printf("Failed to allocate shared memory: %d, %s\n",
             ret, strerror(errno));
      goto err_with_session;
    }

  par0.attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
  par0.a = 0;
  par0.b = shm.size;
  par0.c = shm.id;

  ret = tee_invoke(fd, session, PTA_CMD_GET_DEVICES, &par0, 1);
  if (ret < 0)
    {
      printf("Failed to enumerate devices: %d, %s\n", ret, strerror(errno));
      goto err_with_shm;
    }

  printf("Available devices:\n");

  count = par0.b / sizeof(uuid_t);
  raw_ta_uuid = (uuid_t *)shm.ptr;

  while (count--)
    {
      uuid_dec_be(raw_ta_uuid, &ta_uuid);
      uuid_to_string(&ta_uuid, &ta_uuid_s, NULL);

      printf("  %s\n", ta_uuid_s);

      free(ta_uuid_s);
      raw_ta_uuid++;
    }

err_with_shm:
  tee_shm_free(&shm);

err_with_session:
  tee_close_session(fd, session);

err:
  close(fd);

  return ret;
}
