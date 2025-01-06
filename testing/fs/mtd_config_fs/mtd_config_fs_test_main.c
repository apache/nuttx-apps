/****************************************************************************
 * apps/testing/fs/mtd_config_fs/mtd_config_fs_test_main.c
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
#include <nuttx/mtd/mtd.h>
#include <nuttx/mtd/configdata.h>

#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <nuttx/crc8.h>
#include <debug.h>
#include <assert.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/
#define NVS_ALIGN_SIZE                  CONFIG_MTD_WRITE_ALIGN_SIZE
#define NVS_ALIGN_UP(x)                 (((x) + NVS_ALIGN_SIZE - 1) & ~(NVS_ALIGN_SIZE - 1))

#define TEST_KEY1       "testkey1"
#define TEST_KEY2       "testkey2"
#define TEST_DATA1      "abc"
#define TEST_DATA2      "def"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Allocation Table Entry */

begin_packed_struct struct nvs_ate
{
  uint32_t id;           /* Data id */
  uint16_t offset;       /* Data offset within block */
  uint16_t len;          /* Data len within block */
  uint16_t key_len;      /* Key string len */
  uint8_t  part;         /* Part of a multipart data - future extension */
  uint8_t  crc8;         /* Crc8 check of the ate entry */
#if CONFIG_MTD_WRITE_ALIGN_SIZE <= 4
  /* stay compatible with situation which align byte be 1 */

  uint8_t  expired[NVS_ALIGN_SIZE];
  uint8_t  reserved[4 - NVS_ALIGN_SIZE];
#else
  uint8_t  padding[NVS_ALIGN_UP(12) - 12];
  uint8_t  expired[NVS_ALIGN_SIZE];
#endif
} end_packed_struct;

/* Pre-allocated simulated flash */

struct mtdnvs_ctx_s
{
  char mountdir[CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_MAXNAME];
  struct mallinfo mmbefore;
  struct mallinfo mmprevious;
  struct mallinfo mmafter;
  uint8_t         erasestate;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fnv_hash_32
 ****************************************************************************/

static uint32_t nvs_fnv_hash(FAR const uint8_t *input, uint32_t len)
{
  uint32_t i = 0;
  uint32_t hval = 2166136261;

  /* FNV-1 hash each octet in the buffer */

  while (i++ < len)
    {
      /* multiply by the 32 bit FNV magic prime mod 2^32 */

      hval *= 0x01000193;

      /* xor the bottom with the current octet */

      hval ^= *input++;
    }

  return hval;
}

/****************************************************************************
 * Name: fill_crc8_update
 ****************************************************************************/

static void fill_crc8_update(FAR struct nvs_ate *entry)
{
  uint8_t ate_crc;

  ate_crc = crc8part((FAR const uint8_t *)entry,
                     offsetof(struct nvs_ate, crc8), 0xff);
  entry->crc8 = ate_crc;
}

/****************************************************************************
 * Name: fill_gc_done_ate
 ****************************************************************************/

static void fill_corrupted_close_ate(FAR struct mtdnvs_ctx_s *ctx,
                                     FAR struct nvs_ate *close_ate)
{
  memset((FAR void *)close_ate, ctx->erasestate,
    sizeof(struct nvs_ate));
  close_ate->id = 0xffffffff;
  close_ate->len = 0U;
}

/****************************************************************************
 * Name: fill_close_ate
 ****************************************************************************/

static void fill_close_ate(FAR struct mtdnvs_ctx_s *ctx,
                           FAR struct nvs_ate *close_ate, int offset)
{
  memset((FAR void *)close_ate, ctx->erasestate,
    sizeof(struct nvs_ate));
  close_ate->id = 0xffffffff;
  close_ate->len = 0U;
  close_ate->offset = offset;
  fill_crc8_update(close_ate);
}

/****************************************************************************
 * Name: fill_gc_done_ate
 ****************************************************************************/

static void fill_gc_done_ate(FAR struct mtdnvs_ctx_s *ctx,
                             FAR struct nvs_ate *gc_done_ate)
{
  memset((FAR void *)gc_done_ate, ctx->erasestate,
    sizeof(struct nvs_ate));
  gc_done_ate->id = 0xffffffff;
  gc_done_ate->len = 0U;
  fill_crc8_update(gc_done_ate);
}

/****************************************************************************
 * Name: fill_ate
 ****************************************************************************/

static void fill_ate(FAR struct mtdnvs_ctx_s *ctx, FAR struct nvs_ate *ate,
                     FAR const char *key, uint16_t len, uint16_t offset,
                     bool expired)
{
  memset((FAR void *)ate, ctx->erasestate,
    sizeof(struct nvs_ate));
  ate->id = nvs_fnv_hash((FAR const uint8_t *)key, strlen(key) + 1)
    % 0xfffffffd + 1;
  ate->len = len;
  ate->offset = offset;
  ate->key_len = strlen(key) + 1;
  fill_crc8_update(ate);
  ate->expired[0] = expired ? 0x7f : 0xff;
}

/****************************************************************************
 * Name: fill_corrupted_ate
 ****************************************************************************/

static void fill_corrupted_ate(FAR struct mtdnvs_ctx_s *ctx,
                               FAR struct nvs_ate *ate, FAR const char *key,
                               uint16_t len, uint16_t offset)
{
  memset((FAR void *)ate, ctx->erasestate,
    sizeof(struct nvs_ate));
  ate->id = nvs_fnv_hash((FAR const uint8_t *)key, strlen(key) + 1)
    % 0xfffffffd + 1;
  ate->len = len;
  ate->offset = offset;
  ate->key_len = strlen(key) + 1;
}

/****************************************************************************
 * Name: mtdnvs_showmemusage
 ****************************************************************************/

static void mtdnvs_showmemusage(struct mallinfo *mmbefore,
                                struct mallinfo *mmafter)
{
  printf("VARIABLE  BEFORE   AFTER    DELTA\n");
  printf("======== ======== ======== ========\n");
  printf("arena    %8x %8x %8x\n", mmbefore->arena   , mmafter->arena,
                                   mmafter->arena    - mmbefore->arena);
  printf("ordblks  %8d %8d %8d\n", mmbefore->ordblks , mmafter->ordblks,
                                   mmafter->ordblks  - mmbefore->ordblks);
  printf("mxordblk %8x %8x %8x\n", mmbefore->mxordblk, mmafter->mxordblk,
                                   mmafter->mxordblk - mmbefore->mxordblk);
  printf("uordblks %8x %8x %8x\n", mmbefore->uordblks, mmafter->uordblks,
                                   mmafter->uordblks - mmbefore->uordblks);
  printf("fordblks %8x %8x %8x\n", mmbefore->fordblks, mmafter->fordblks,
                                   mmafter->fordblks - mmbefore->fordblks);
}

/****************************************************************************
 * Name: mtdnvs_endmemusage
 ****************************************************************************/

static void mtdnvs_endmemusage(FAR struct mtdnvs_ctx_s *ctx)
{
  ctx->mmafter = mallinfo();

  printf("\nFinal memory usage:\n");
  mtdnvs_showmemusage(&ctx->mmbefore, &ctx->mmafter);
}

/****************************************************************************
 * Name: show_useage
 ****************************************************************************/

static void show_useage(void)
{
  printf("Usage : mtdconfig_fs_test [OPTION [ARG]] ...\n");
  printf("-h    show this help statement\n");
  printf("-m    mount point to be tested e.g. [%s]\n",
          CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME);
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static int teardown(void)
{
  int fd;
  int ret;

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      return -errno;
    }

  ret = ioctl(fd, MTDIOC_BULKERASE, NULL);
  if (ret < 0)
    {
      printf("%s:clear failed, ret=%d\n", __func__, ret);
      return -errno;
    }

  close(fd);

  ret = mtdconfig_unregister();
  if (ret < 0)
    {
      printf("%s:mtdconfig_unregister failed, ret=%d\n", __func__, ret);
      return ret;
    }

  return ret;
}

extern int find_mtddriver(FAR const char *pathname,
                        FAR struct inode **ppinode);

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static int setup(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  FAR struct inode *sys_node;

  ret = find_mtddriver(CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME,
    &sys_node);
  if (ret < 0)
    {
      printf("ERROR: open %s failed: %d\n",
        CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME, ret);
      return -errno;
    }

  ret = MTD_IOCTL(sys_node->u.i_mtd, MTDIOC_ERASESTATE,
                 (unsigned long)((uintptr_t)&ctx->erasestate));
  if (ret < 0)
    {
      printf("ERROR: MTD ioctl(MTDIOC_ERASESTATE) failed: %d\n", ret);
      return ret;
    }

  ret = mtdconfig_register(sys_node->u.i_mtd);
  if (ret < 0)
    {
      printf("%s:mtdnvs_register failed, ret=%d\n", __func__, ret);
      return ret;
    }

  return ret;
}

/****************************************************************************
 * Name: test_nvs_mount
 ****************************************************************************/

static void test_nvs_mount(struct mtdnvs_ctx_s *ctx)
{
  int ret;

  printf("%s: test begin\n", __func__);

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:mtdconfig_register failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: execute_long_pattern_write
 ****************************************************************************/

static int execute_long_pattern_write(const char *key)
{
  struct config_data_s data;
  int i;
  int fd;
  int ret;
  uint8_t rd_buf[512];
  uint8_t wr_buf[512];
  char pattern[4];

  pattern[0] = 0xde;
  pattern[1] = 0xad;
  pattern[2] = 0xbe;
  pattern[3] = 0xef;

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      return -errno;
    }

  strlcpy(data.name, key, sizeof(data.name));
  data.configdata = rd_buf;
  data.len = sizeof(rd_buf);
  ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);
  if (ret != -1 || errno != ENOENT)
    {
      printf("%s:CFGDIOC_GETCONFIG unexpected failure: %d\n",
        __func__, ret);
      goto err_fd;
    }

  for (i = 0; i < sizeof(wr_buf); i += sizeof(pattern))
    {
      memcpy(wr_buf + i, pattern, sizeof(pattern));
    }

  data.configdata = wr_buf;
  data.len = sizeof(wr_buf);

  ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto err_fd;
    }

  data.configdata = rd_buf;
  data.len = sizeof(rd_buf);
  ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto err_fd;
    }

  if (memcmp(wr_buf, rd_buf, sizeof(rd_buf)) != 0)
    {
      printf("%s:RD buff should be equal to the WR buff\n", __func__);
      ret = -EIO;
      goto err_fd;
    }

err_fd:
  close(fd);

  return ret;
}

/****************************************************************************
 * Name: test_nvs_write
 ****************************************************************************/

static void test_nvs_write(struct mtdnvs_ctx_s *ctx)
{
  int ret;

  printf("%s: test begin\n", __func__);

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:mtdconfig_register failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = execute_long_pattern_write(TEST_KEY1);
  if (ret < 0)
    {
      printf("%s:execute_long_pattern_write failed, ret=%d\n",
        __func__, ret);
      goto test_fail;
    }

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_corrupt_expire
 * Description: Test that startup correctly handles invalid expire.
 ****************************************************************************/

static void test_nvs_corrupt_expire(struct mtdnvs_ctx_s *ctx)
{
  struct nvs_ate ate;
  int mtd_fd = -1;
  int nvs_fd = -1;
  int ret;
  int i;
  uint8_t erase_value = ctx->erasestate;
  struct config_data_s data;
  char rd_buf[50];
  size_t padding_size;

  printf("%s: test begin\n", __func__);

  mtd_fd = open(CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME, O_RDWR);
  if (mtd_fd < 0)
    {
      printf("%s:mtdnvs_register failed, ret=%d\n", __func__, mtd_fd);
      goto test_fail;
    }

  /* write valid data */

  ret = write(mtd_fd, TEST_KEY1, strlen(TEST_KEY1) + 1);
  if (ret < 0)
    {
      printf("%s:write key1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = write(mtd_fd, TEST_DATA1, strlen(TEST_DATA1) + 1);
  if (ret < 0)
    {
      printf("%s:write data1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  padding_size = NVS_ALIGN_UP(sizeof(TEST_KEY1) + sizeof(TEST_DATA1))
                - sizeof(TEST_KEY1) - sizeof(TEST_DATA1);
  for (i = 0; i < padding_size; i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:write data1 padding failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* write valid data again, simulate overwrite data */

  ret = write(mtd_fd, TEST_KEY1, strlen(TEST_KEY1) + 1);
  if (ret < 0)
    {
      printf("%s:write key1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = write(mtd_fd, TEST_DATA2, strlen(TEST_DATA2) + 1);
  if (ret < 0)
    {
      printf("%s:write data2 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* set unused flash to 0xff */

  for (i = 2 * (strlen(TEST_KEY1) + strlen(TEST_DATA1) + 2) + padding_size;
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - 4 * sizeof(struct nvs_ate); i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* Write ate */

  fill_ate(ctx, &ate, TEST_KEY1, strlen(TEST_DATA2) + 1,
    strlen(TEST_KEY1) + strlen(TEST_DATA1) + 2 + padding_size, false);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fill_ate(ctx, &ate, TEST_KEY1, strlen(TEST_DATA1) + 1, 0, false);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* write gc_done ate */

  fill_gc_done_ate(ctx, &ate);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* write close ate, mark section 0 as closed */

  fill_close_ate(ctx, &ate,
    CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
    - 4 * sizeof(struct nvs_ate));
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write close ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  close(mtd_fd);
  mtd_fd = -1;

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  nvs_fd = open("/dev/config", 0);
  if (nvs_fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, nvs_fd);
      goto test_fail;
    }

  strlcpy(data.name, TEST_KEY1, sizeof(data.name));
  data.configdata = (FAR uint8_t *)rd_buf;
  data.len = sizeof(rd_buf);

  ret = ioctl(nvs_fd, CFGDIOC_FIRSTCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_FIRSTCONFIG unexpected failure: %d\n",
        __func__, ret);
      goto test_fail;
    }

  if (strncmp(rd_buf, TEST_DATA2, sizeof(rd_buf)) != 0)
    {
      printf("%s:unexpected value\n", __func__);
      goto test_fail;
    }

  ret = ioctl(nvs_fd, CFGDIOC_NEXTCONFIG, &data);
  if (ret != -1 || errno != ENOENT)
    {
      printf("%s:CFGDIOC_NEXTCONFIG should return ENOENT, but: %d\n",
        __func__, ret);
      goto test_fail;
    }

  close(nvs_fd);
  nvs_fd = -1;

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (mtd_fd >= 0)
    {
      close(mtd_fd);
    }

  if (nvs_fd >= 0)
    {
      close(nvs_fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_corrupted_write
 ****************************************************************************/

static void test_nvs_corrupted_write(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  char rd_buf[512];
  char wr_buf_1[] = TEST_DATA1;
  char wr_buf_2[] = TEST_DATA2;
  char key1[] = TEST_KEY1;
  int mtd_fd = -1;
  int nvs_fd = -1;
  int i;
  uint8_t erase_value = ctx->erasestate;
  struct nvs_ate ate;
  struct config_data_s data;
  size_t padding_size;

  printf("%s: test begin\n", __func__);

  mtd_fd = open(CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME, O_RDWR);
  if (mtd_fd < 0)
    {
      printf("%s:mtdnvs_register failed, ret=%d\n", __func__, mtd_fd);
      goto test_fail;
    }

  /* lets simulate loss of part of data */

  /* write valid data */

  ret = write(mtd_fd, key1, sizeof(key1));
  if (ret != sizeof(key1))
    {
      printf("%s:write key1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = write(mtd_fd, wr_buf_1, sizeof(wr_buf_1));
  if (ret != sizeof(wr_buf_1))
    {
      printf("%s:write data1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  padding_size = NVS_ALIGN_UP(sizeof(key1) + sizeof(wr_buf_1))
                - sizeof(key1) - sizeof(wr_buf_1);
  for (i = 0; i < padding_size; i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:write data1 padding failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* power loss occurs after we write data */

  ret = write(mtd_fd, key1, sizeof(key1));
  if (ret != sizeof(key1))
    {
      printf("%s:write key1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = write(mtd_fd, wr_buf_2, sizeof(wr_buf_1));
  if (ret != sizeof(wr_buf_2))
    {
      printf("%s:write data2 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* set unused flash to 0xff */

  for (i = 2 * (sizeof(key1) + sizeof(wr_buf_1)) + padding_size;
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - 3 * sizeof(struct nvs_ate); i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* write ate */

  fill_ate(ctx, &ate, key1, sizeof(wr_buf_1), 0, false);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* write gc_done ate */

  fill_gc_done_ate(ctx, &ate);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  close(mtd_fd);
  mtd_fd = -1;

  /* now start up */

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  nvs_fd = open("/dev/config", 0);
  if (nvs_fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, nvs_fd);
      goto test_fail;
    }

  strlcpy(data.name, TEST_KEY1, sizeof(data.name));
  data.configdata = (FAR uint8_t *)rd_buf;
  data.len = sizeof(rd_buf);
  ret = ioctl(nvs_fd, CFGDIOC_GETCONFIG, &data);
  if (ret < 0)
    {
      printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  if (strncmp(rd_buf, wr_buf_1, sizeof(rd_buf)) != 0)
    {
      printf("%s:failed, RD buff should be equal to the first WR buff "
        "because subsequent write operation has failed\n",
        __func__);
      goto test_fail;
    }

  close(nvs_fd);
  nvs_fd = -1;

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  printf("%s: failed\n", __func__);
  if (nvs_fd > 0)
    {
      close(nvs_fd);
    }

  if (mtd_fd > 0)
    {
      close(mtd_fd);
    }
}

/****************************************************************************
 * Name: test_nvs_gc
 ****************************************************************************/

static void test_nvs_gc(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  int fd = -1;
  uint8_t buf[44];
  uint8_t rd_buf[44];
  struct config_data_s data;
  uint16_t i;
  uint16_t id;
  const uint16_t max_id = 10;

  /* max_writes will trigger GC. */

  size_t kv_size = NVS_ALIGN_UP(44 + 4) + sizeof(struct nvs_ate);
  size_t block_max_write_size =
    CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
    - sizeof(struct nvs_ate);
  uint16_t block_max_write_nums = block_max_write_size / kv_size;

  const uint16_t max_writes = block_max_write_nums + 1;

  printf("%s: test begin\n", __func__);

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  for (i = 0; i < max_writes; i++)
    {
      id = (i % max_id);
      uint8_t id_data = id + max_id * (i / max_id);

      memset(buf, id_data, sizeof(buf));

      /* 4 byte key */

      snprintf(data.name, sizeof(data.name), "k%02d", id);
      data.configdata = buf;
      data.len = sizeof(buf);

      ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
      if (ret != 0)
        {
          printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }
    }

  for (id = 0; id < max_id; id++)
    {
      /* 4 byte key */

      snprintf(data.name, sizeof(data.name), "k%02d", id);
      data.configdata = rd_buf;
      data.len = sizeof(rd_buf);

      ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);
      if (ret != 0)
        {
          printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }

      for (i = 0; i < sizeof(rd_buf); i++)
        {
          rd_buf[i] = rd_buf[i] % max_id;
          buf[i] = id;
        }

      if (memcmp(buf, rd_buf, sizeof(rd_buf)) != 0)
        {
          printf("RD buff should be equal to the WR buff\n");
          goto test_fail;
        }
    }

  close(fd);
  fd = -1;

  ret = mtdconfig_unregister();
  if (ret < 0)
    {
      printf("%s:mtdconfig_unregister failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  for (id = 0; id < max_id; id++)
    {
      /* 4 byte key */

      snprintf(data.name, sizeof(data.name), "k%02d", id);
      data.configdata = rd_buf;
      data.len = sizeof(rd_buf);

      ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);
      if (ret != 0)
        {
          printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }

      for (i = 0; i < sizeof(rd_buf); i++)
        {
          rd_buf[i] = rd_buf[i] % max_id;
          buf[i] = id;
        }

      if (memcmp(buf, rd_buf, sizeof(rd_buf)) != 0)
        {
          printf("RD buff should be equal to the WR buff\n");
          goto test_fail;
        }
    }

  close(fd);
  fd = -1;

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (fd >= 0)
    {
      close(fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: write_content
 ****************************************************************************/

static int write_content(uint16_t max_id, uint16_t begin, uint16_t end)
{
  uint8_t buf[44];
  int fd = -1;
  int ret;
  struct config_data_s data;
  uint16_t i;

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      return -errno;
    }

  for (i = begin; i < end; i++)
    {
      uint8_t id = (i % max_id);
      uint8_t id_data = id + max_id * (i / max_id);

      memset(buf, id_data, sizeof(buf));

      /* 4 byte key */

      snprintf(data.name, sizeof(data.name), "k%02d", id);
      data.configdata = buf;
      data.len = sizeof(buf);

      ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
      if (ret != 0)
        {
          printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }
    }

  close(fd);
  return ret;

test_fail:
  if (fd >= 0)
    {
      close(fd);
    }

  return ret;
}

/****************************************************************************
 * Name: check_content
 ****************************************************************************/

static int check_content(uint16_t max_id)
{
  uint8_t rd_buf[44];
  uint8_t buf[44];
  int fd = -1;
  int ret;
  struct config_data_s data;

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      return -errno;
    }

  for (uint16_t id = 0; id < max_id; id++)
    {
      /* 4 byte key */

      snprintf(data.name, sizeof(data.name), "k%02d", id);
      data.configdata = rd_buf;
      data.len = sizeof(rd_buf);

      ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);
      if (ret != 0)
        {
          printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }

      for (uint16_t i = 0; i < sizeof(rd_buf); i++)
        {
          rd_buf[i] = rd_buf[i] % max_id;
          buf[i] = id;
        }

      if (memcmp(buf, rd_buf, sizeof(rd_buf)) != 0)
        {
          printf("RD buff should be equal to the WR buff\n");
          goto test_fail;
        }
    }

  close(fd);
  return ret;

test_fail:
  if (fd >= 0)
    {
      close(fd);
    }

  return ret;
}

/****************************************************************************
 * Name: test_nvs_gc_3sectors
 * Description: Full round of GC over 3 sectors
 ****************************************************************************/

static void test_nvs_gc_3sectors(struct mtdnvs_ctx_s *ctx)
{
  int ret;

  /* max_writes will trigger GC */

  size_t kv_size = NVS_ALIGN_UP(44 + 4) + sizeof(struct nvs_ate);
  size_t block_max_write_size =
    CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
    - sizeof(struct nvs_ate);
  uint16_t max_id = block_max_write_size / kv_size;

  const uint16_t max_writes = max_id + 1;
  const uint16_t max_writes_2 = max_writes + max_id;
  const uint16_t max_writes_3 = max_writes_2 + max_id;
  const uint16_t max_writes_4 = max_writes_3 + max_id;

  printf("%s: test begin\n", __func__);

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* Trigger 1st GC */

  ret = write_content(max_id, 0, max_writes);
  if (ret < 0)
    {
      printf("%s:1st GC write failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:1st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = mtdconfig_unregister();
  if (ret < 0)
    {
      printf("%s:1st mtdconfig_unregister failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:1st setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:1st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* Trigger 2nd GC */

  ret = write_content(max_id, max_writes, max_writes_2);
  if (ret < 0)
    {
      printf("%s:2st GC write failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:2st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = mtdconfig_unregister();
  if (ret < 0)
    {
      printf("%s:2st mtdconfig_unregister failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:2st setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:2st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* Trigger 3rd GC */

  ret = write_content(max_id, max_writes_2, max_writes_3);
  if (ret < 0)
    {
      printf("%s:3st GC write failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:3st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = mtdconfig_unregister();
  if (ret < 0)
    {
      printf("%s:3st mtdconfig_unregister failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:3st setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:3st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* Trigger 4th GC */

  ret = write_content(max_id, max_writes_3, max_writes_4);
  if (ret < 0)
    {
      printf("%s:4st GC write failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:4st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = mtdconfig_unregister();
  if (ret < 0)
    {
      printf("%s:4st mtdconfig_unregister failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:4st setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = check_content(max_id);
  if (ret < 0)
    {
      printf("%s:4st GC check failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_corrupted_sector_close
 ****************************************************************************/

static void test_nvs_corrupted_sector_close(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  char key1[] = TEST_KEY1;
  uint8_t rd_buf[NVS_ALIGN_UP(512) - sizeof(key1)];
  uint8_t wr_buf[NVS_ALIGN_UP(512) - sizeof(key1)];
  int mtd_fd = -1;
  int nvs_fd = -1;
  int loop_section;
  int i;
  uint8_t erase_value = ctx->erasestate;
  struct nvs_ate ate;
  struct config_data_s data;

  printf("%s: test begin\n", __func__);

  mtd_fd = open(CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME, O_RDWR);
  if (mtd_fd < 0)
    {
      printf("%s:mtdnvs_register failed, ret=%d\n", __func__, mtd_fd);
      goto test_fail;
    }

  /* lets simulate loss of close at the beginning of gc */

  for (loop_section = 0;
    loop_section <
      CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_COUNT - 1;
    loop_section++)
    {
      /* write valid data */

      for (i = 0; i < 2 ; i++)
        {
          ret = write(mtd_fd, key1, sizeof(key1));
          if (ret != sizeof(key1))
            {
              printf("%s:write key1 failed, ret=%d\n", __func__, ret);
              goto test_fail;
            }

          memset(wr_buf, 'A' + i, sizeof(wr_buf));
          wr_buf[sizeof(wr_buf) - 1] = '\0';

          ret = write(mtd_fd, wr_buf, sizeof(wr_buf));
          if (ret != sizeof(wr_buf))
            {
              printf("%s:write data1 failed, ret=%d\n", __func__, ret);
              goto test_fail;
            }
        }

      /* set unused flash to 0xff */

      for (i = 2 * (sizeof(key1) + sizeof(wr_buf));
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - 4 * sizeof(struct nvs_ate); i++)
        {
          ret = write(mtd_fd, &erase_value, sizeof(erase_value));
          if (ret != sizeof(erase_value))
            {
              printf("%s:erase failed, ret=%d\n", __func__, ret);
              goto test_fail;
            }
        }

      /* write ate 2 times */

      fill_ate(ctx, &ate, key1, sizeof(wr_buf),
        sizeof(wr_buf) + sizeof(key1),
        (loop_section ==
        CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_COUNT - 2)
        ? false : true);
      ret = write(mtd_fd, &ate, sizeof(ate));
      if (ret != sizeof(ate))
        {
          printf("%s:write ate failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }

      fill_ate(ctx, &ate, key1, sizeof(wr_buf), 0, true);
      ret = write(mtd_fd, &ate, sizeof(ate));
      if (ret != sizeof(ate))
        {
          printf("%s:write ate failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }

      /* write gc_done ate */

      fill_gc_done_ate(ctx, &ate);
      ret = write(mtd_fd, &ate, sizeof(ate));
      if (ret != sizeof(ate))
        {
          printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }

      if (loop_section ==
        CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_COUNT - 2)
        {
          fill_corrupted_close_ate(ctx, &ate);
        }
      else
        {
          fill_close_ate(ctx, &ate,
            CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
            - 4 * sizeof(struct nvs_ate));
        }

      ret =  write(mtd_fd, &ate, sizeof(ate));
      if (ret != sizeof(ate))
        {
          printf("%s:write close ate failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  close(mtd_fd);
  mtd_fd = -1;

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  nvs_fd = open("/dev/config", 0);
  if (nvs_fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, nvs_fd);
      goto test_fail;
    }

  strlcpy(data.name, TEST_KEY1, sizeof(data.name));
  data.configdata = rd_buf;
  data.len = sizeof(rd_buf);

  ret = ioctl(nvs_fd, CFGDIOC_GETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  if (memcmp(rd_buf, wr_buf, sizeof(rd_buf)) != 0)
    {
      printf("%s:strncmp failed\n", __func__);
      ret = -EACCES;
      goto test_fail;
    }

  /* Ensure that the NVS is able to store new content. */

  if (execute_long_pattern_write(TEST_KEY2) != 0)
    {
      printf("%s:write again failed\n", __func__);
      ret = -EACCES;
      goto test_fail;
    }

  close(nvs_fd);
  nvs_fd = -1;

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (nvs_fd >= 0)
    {
      close(nvs_fd);
    }

  if (mtd_fd >= 0)
    {
      close(mtd_fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_full_sector
 * Description: Test case when storage become full,
 *              so only deletion is possible.
 ****************************************************************************/

static void test_nvs_full_sector(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  uint16_t filling_id = 0;
  uint16_t i;
  uint16_t data_read;
  int fd = -1;
  struct config_data_s data;

  printf("%s: test begin\n", __func__);

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  while (1)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", filling_id);
      data.configdata = (FAR uint8_t *)&filling_id;
      data.len = sizeof(filling_id);

      ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
      if (ret == -1 && errno == ENOSPC)
        {
          break;
        }
      else if (ret != 0)
        {
          printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }

      filling_id++;
    }

  /* check whether can delete whatever from full storage */

  snprintf(data.name, sizeof(data.name), "k%04x", 1);
  data.configdata = NULL;
  data.len = 0;

  ret = ioctl(fd, CFGDIOC_DELCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_DELCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* the last sector is full now, test re-initialization */

  close(fd);
  fd = -1;
  mtdconfig_unregister();

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  snprintf(data.name, sizeof(data.name), "k%04x", filling_id);
  data.configdata = (FAR uint8_t *)&filling_id;
  data.len = sizeof(filling_id);

  ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* sanitycheck on NVS content */

  for (i = 0; i <= filling_id; i++)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", i);
      data.configdata = (FAR uint8_t *)&data_read;
      data.len = sizeof(data_read);

      ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);

      if (i == 1)
        {
          if (ret != -1 || errno != ENOENT)
            {
              printf("%s:shouldn't found the entry: %d\n", __func__, i);
              ret = -EIO;
              goto test_fail;
            }
        }
      else if (ret != 0)
        {
          printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }
      else
        {
          if (data_read != i)
            {
              printf("%s:read data %d \n", __func__, data_read);
              printf("%s:read expected  %d\n", __func__, i);
              printf("%s:read unexpected data: %d instead of %d\n",
                __func__, data_read, i);
              ret = -EIO;
              goto test_fail;
            }
        }
    }

  close(fd);

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (fd >= 0)
    {
      close(fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_gc_corrupt_close_ate
 * Description: Test that garbage-collection can
 *      recover all ate's even when the last ate, ie close_ate,
 *      is corrupt. In this test the close_ate is set to point to the
 *      last ate at -5. A valid ate is however present at -6.
 *      Since the close_ate has an invalid crc8, the offset
 *      should not be used and a recover of the
 *      last ate should be done instead.
 ****************************************************************************/

static void test_nvs_gc_corrupt_close_ate(struct mtdnvs_ctx_s *ctx)
{
  struct nvs_ate ate;
  struct nvs_ate close_ate;
  int mtd_fd = -1;
  int nvs_fd = -1;
  int ret;
  int i;
  uint8_t erase_value = ctx->erasestate;
  struct config_data_s data;
  char rd_buf[50];

  printf("%s: test begin\n", __func__);

  mtd_fd = open(CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME, O_RDWR);
  if (mtd_fd < 0)
    {
      printf("%s:mtdnvs_register failed, ret=%d\n", __func__, mtd_fd);
      goto test_fail;
    }

  /* write valid data */

  ret = write(mtd_fd, TEST_KEY1, strlen(TEST_KEY1) + 1);
  if (ret != strlen(TEST_KEY1) + 1)
    {
      printf("%s:write key1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  ret = write(mtd_fd, TEST_DATA1, strlen(TEST_DATA1) + 1);
  if (ret != strlen(TEST_DATA1) + 1)
    {
      printf("%s:write data1 failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* set unused flash to 0xff */

  for (i = strlen(TEST_KEY1) + strlen(TEST_DATA1) + 2;
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - 6 * sizeof(struct nvs_ate); i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* Write valid ate at -6 */

  fill_ate(ctx, &ate, TEST_KEY1, strlen(TEST_DATA1) + 1, 0, false);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* set unused flash to 0xff */

  for (i = CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - 5 * sizeof(struct nvs_ate);
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - 2 * sizeof(struct nvs_ate); i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* write gc_done ate */

  fill_gc_done_ate(ctx, &ate);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* write invalid close ate, mark section 0 as closed */

  fill_corrupted_close_ate(ctx, &close_ate);
  ret = write(mtd_fd, &close_ate, sizeof(close_ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* set unused flash to 0xff in section 1 */

  for (i = CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE;
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - sizeof(struct nvs_ate); i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* write invalid close ate, mark section 1 as closed */

  fill_corrupted_close_ate(ctx, &close_ate);
  ret = write(mtd_fd, &close_ate, sizeof(close_ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  close(mtd_fd);
  mtd_fd = -1;

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  nvs_fd = open("/dev/config", 0);
  if (nvs_fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, nvs_fd);
      goto test_fail;
    }

  strlcpy(data.name, TEST_KEY1, sizeof(data.name));
  data.configdata = (FAR uint8_t *)rd_buf;
  data.len = sizeof(rd_buf);

  ret = ioctl(nvs_fd, CFGDIOC_GETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:NVSIOC_READ unexpected failure: %d\n", __func__, ret);
      goto test_fail;
    }

  if (strncmp(rd_buf, TEST_DATA1, sizeof(rd_buf)) != 0)
    {
      printf("%s:unexpected value\n", __func__);
      goto test_fail;
    }

  close(nvs_fd);
  nvs_fd = -1;

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (nvs_fd >= 0)
    {
      close(nvs_fd);
    }

  if (mtd_fd >= 0)
    {
      close(mtd_fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_gc_corrupt_ate
 * Description: Test that garbage-collection correctly handles corrupt ate's.
 ****************************************************************************/

static void test_nvs_gc_corrupt_ate(struct mtdnvs_ctx_s *ctx)
{
  struct nvs_ate ate;
  struct nvs_ate close_ate;
  int mtd_fd = -1;
  int ret;
  int i;
  uint8_t erase_value = ctx->erasestate;

  printf("%s: test begin\n", __func__);

  fill_corrupted_ate(ctx, &ate, TEST_KEY1, 10, 0);

  mtd_fd = open(CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME, O_RDWR);
  if (mtd_fd < 0)
    {
      printf("%s:mtdnvs_register failed, ret=%d\n", __func__, mtd_fd);
      goto test_fail;
    }

  /* set unused flash to 0xff */

  for (i = 0 ;
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE / 2; i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* Write invalid ate */

  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  for (i = CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE / 2
        + sizeof(struct nvs_ate);
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - 2 * sizeof(struct nvs_ate); i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* write gc_done ate */

  fill_gc_done_ate(ctx, &ate);
  ret = write(mtd_fd, &ate, sizeof(ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* write close ate, mark section 0 as closed */

  fill_close_ate(ctx, &close_ate,
    CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE / 2);
  ret = write(mtd_fd, &close_ate, sizeof(close_ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* set unused flash to 0xff in section 1 */

  for (i = CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE;
        i < CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_FLASH_SECTION_SIZE
        - sizeof(struct nvs_ate); i++)
    {
      ret = write(mtd_fd, &erase_value, sizeof(erase_value));
      if (ret != sizeof(erase_value))
        {
          printf("%s:erase failed, ret=%d\n", __func__, ret);
          goto test_fail;
        }
    }

  /* write close ate, mark section 1 as closed */

  ret = write(mtd_fd, &close_ate, sizeof(close_ate));
  if (ret != sizeof(ate))
    {
      printf("%s:write gc_done ate failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  close(mtd_fd);
  mtd_fd = -1;

  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  /* at the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (mtd_fd >= 0)
    {
      close(mtd_fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_gc_touched_deleted_ate
 * Description: Test case when writing and gc touched A prev entry
 *   which was deleted.
 ****************************************************************************/

static void test_nvs_gc_touched_deleted_ate(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  uint16_t filling_id = 0;
  uint16_t i;
  uint16_t data_read;
  int fd = -1;
  struct config_data_s data;

  printf("%s: test begin\n", __func__);
  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  while (1)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", filling_id);
      data.configdata = (FAR uint8_t *)&filling_id;
      data.len = sizeof(filling_id);

      ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);

      /* ENOSPC will be accompanied by gc 3 times(if 3 blocks)
       * block will change three times:
       * block:    0,               1,           2
       *           A                B            gc
       *  (gc==1) gc                B            A
       *  (gc==2)  B                gc           A
       *  (gc==3)  B                A            gc
       */

      if (ret == -1 && errno == ENOSPC)
        {
          break;
        }
      else if (ret != 0)
        {
          printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }

      filling_id++;
    }

  /* Now delete last record.
   * The layout should be:
   * block:    0,               1,           2
   *           B(deleted)       A            gc
   */

  snprintf(data.name, sizeof(data.name), "k%04x", filling_id - 1);
  data.configdata = NULL;
  data.len = 0;
  ret = ioctl(fd, CFGDIOC_DELCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_DELCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* Now input B again,
   * we should trigger gc for once, and we needn't search for
   * the old one again because it is expired.
   */

  filling_id -= 1;
  snprintf(data.name, sizeof(data.name), "k%04x", filling_id);
  data.configdata = (FAR uint8_t *)&filling_id;
  data.len = sizeof(filling_id);
  ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* Sanitycheck on NVS content */

  for (i = 0; i <= filling_id; i++)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", i);
      data.configdata = (FAR uint8_t *)&data_read;
      data.len = sizeof(data_read);

      ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);

      if (ret != 0)
        {
          printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }
      else
        {
          if (data_read != i)
            {
              printf("%s:read data %d \n", __func__, data_read);
              printf("%s:read expected  %d\n", __func__, i);
              printf("%s:read unexpected data: %d instead of %d\n",
                __func__, data_read, i);
              ret = -EIO;
              goto test_fail;
            }
        }
    }

  close(fd);

  /* At the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (fd >= 0)
    {
      close(fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_gc_touched_expired_ate
 * Description: Test case when writing and  gc touched A prev entry
 *   which was moved by gc.
 ****************************************************************************/

static void test_nvs_gc_touched_expired_ate(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  uint16_t filling_id = 0;
  uint16_t update_id;
  uint16_t i;
  uint16_t data_read;
  int fd = -1;
  struct config_data_s data;

  printf("%s: test begin\n", __func__);
  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  while (1)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", filling_id);
      data.configdata = (FAR uint8_t *)&filling_id;
      data.len = sizeof(filling_id);
      ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
      if (ret == -1 && errno == ENOSPC)
        {
          break;
        }
      else if (ret != 0)
        {
          printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }

      filling_id++;
    }

  /* Now delete,
   * the layout should be:
   * block:    0,               1,           2
   *           B                A(deleted)   gc
   */

  snprintf(data.name, sizeof(data.name), "k%04x", 1);
  data.configdata = NULL;
  data.len = 0;
  ret = ioctl(fd, CFGDIOC_DELCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_DELCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* The last sector is full now, test re-initialization */

  close(fd);
  fd = -1;
  mtdconfig_unregister();
  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  /* Now update A.
   * It should trigger gc for twice, and we need to search for
   * the old one again as gc has touched the old one. We will
   * find it and expire the old A.
   */

  update_id = 3;
  snprintf(data.name, sizeof(data.name), "k%04x", 2);
  data.configdata = (FAR uint8_t *)&update_id;
  data.len = sizeof(update_id);
  ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* Sanitycheck on NVS content */

  for (i = 0; i <= filling_id - 1; i++)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", i);
      data.configdata = (FAR uint8_t *)&data_read;
      data.len = sizeof(data_read);
      ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);
      if (i == 1)
        {
          if (ret != -1 || errno != ENOENT)
            {
              printf("%s:shouldn't found the entry: %d\n", __func__, i);
              ret = -EIO;
              goto test_fail;
            }
        }
      else if (ret != 0)
        {
          printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }
      else if (i == 2)
        {
          if (data_read != 3)
            {
              printf("%s:read data %d \n", __func__, data_read);
              printf("%s:read expected  %d\n", __func__, 3);
              printf("%s:read unexpected data: %d instead of %d\n",
                __func__, data_read, 3);
              ret = -EIO;
              goto test_fail;
            }
        }
      else
        {
          if (data_read != i)
            {
              printf("%s:read data %d \n", __func__, data_read);
              printf("%s:read expected  %d\n", __func__, i);
              printf("%s:read unexpected data: %d instead of %d\n",
                __func__, data_read, i);
              ret = -EIO;
              goto test_fail;
            }
        }
    }

  close(fd);

  /* At the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (fd >= 0)
    {
      close(fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Name: test_nvs_gc_not_touched_expired_ate
 * Description: Test case when writing and  gc not touched a prev entry
 *   which was moved by gc.
 ****************************************************************************/

static void test_nvs_gc_not_touched_expired_ate(struct mtdnvs_ctx_s *ctx)
{
  int ret;
  uint16_t filling_id = 0;
  uint16_t update_id;
  uint16_t i;
  uint16_t data_read;
  int fd = -1;
  struct config_data_s data;

  printf("%s: test begin\n", __func__);
  ret = setup(ctx);
  if (ret < 0)
    {
      printf("%s:setup failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  fd = open("/dev/config", 0);
  if (fd < 0)
    {
      printf("%s:open failed, ret=%d\n", __func__, fd);
      goto test_fail;
    }

  while (1)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", filling_id);
      data.configdata = (FAR uint8_t *)&filling_id;
      data.len = sizeof(filling_id);
      ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
      if (ret == -1 && errno == ENOSPC)
        {
          break;
        }
      else if (ret != 0)
        {
          printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }

      filling_id++;
    }

  /* Now delete last record,
   * the layout should be:
   * block:    0,               1,           2
   *           B(deleted)       A            gc
   */

  snprintf(data.name, sizeof(data.name), "k%04x", filling_id - 1);
  data.configdata = NULL;
  data.len = 0;
  ret = ioctl(fd, CFGDIOC_DELCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_DELCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* Now udpate A again.
   * We should trigger gc for once, and we won't need to search for
   * the old one again after gc.
   */

  update_id = 3;
  snprintf(data.name, sizeof(data.name), "k%04x", 2);
  data.configdata = (FAR uint8_t *)&update_id;
  data.len = sizeof(update_id);
  ret = ioctl(fd, CFGDIOC_SETCONFIG, &data);
  if (ret != 0)
    {
      printf("%s:CFGDIOC_SETCONFIG failed, ret=%d\n", __func__, ret);
      ret = -EIO;
      goto test_fail;
    }

  /* Sanitycheck on NVS content */

  for (i = 0; i <= filling_id - 1; i++)
    {
      snprintf(data.name, sizeof(data.name), "k%04x", i);
      data.configdata = (FAR uint8_t *)&data_read;
      data.len = sizeof(data_read);
      ret = ioctl(fd, CFGDIOC_GETCONFIG, &data);
      if (i == filling_id - 1)
        {
          if (ret != -1 || errno != ENOENT)
            {
              printf("%s:shouldn't found the entry: %d\n", __func__, i);
              ret = -EIO;
              goto test_fail;
            }
        }
      else if (ret != 0)
        {
          printf("%s:CFGDIOC_GETCONFIG failed, ret=%d\n", __func__, ret);
          ret = -EIO;
          goto test_fail;
        }
      else if (i == 2)
        {
          if (data_read != 3)
            {
              printf("%s:read data %d \n", __func__, data_read);
              printf("%s:read expected  %d\n", __func__, 3);
              printf("%s:read unexpected data: %d instead of %d\n",
                __func__, data_read, 3);
              ret = -EIO;
              goto test_fail;
            }
        }
      else
        {
          if (data_read != i)
            {
              printf("%s:read data %d \n", __func__, data_read);
              printf("%s:read expected  %d\n", __func__, i);
              printf("%s:read unexpected data: %d instead of %d\n",
                __func__, data_read, i);
              ret = -EIO;
              goto test_fail;
            }
        }
    }

  close(fd);

  /* At the end of test, erase all blocks */

  ret = teardown();
  if (ret < 0)
    {
      printf("%s:teardown failed, ret=%d\n", __func__, ret);
      goto test_fail;
    }

  printf("%s: success\n", __func__);
  return;

test_fail:
  if (fd >= 0)
    {
      close(fd);
    }

  printf("%s: failed\n", __func__);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct mtdnvs_ctx_s *ctx;
  int option;

  ctx = malloc(sizeof(struct mtdnvs_ctx_s));
  if (ctx == NULL)
    {
      printf("malloc ctx feild,exit!\n");
      exit(1);
    }

  memset(ctx, 0, sizeof(struct mtdnvs_ctx_s));

  strlcpy(ctx->mountdir, CONFIG_TESTING_MTD_CONFIG_FAIL_SAFE_MOUNTPT_NAME,
    sizeof(ctx->mountdir));

  /* Opt Parse */

  while ((option = getopt(argc, argv, ":m:hn:")) != -1)
    {
      switch (option)
        {
          case 'm':
            strlcpy(ctx->mountdir, optarg,
                    sizeof(ctx->mountdir));
            break;
          case 'h':
            show_useage();
            free(ctx);
            exit(0);
          case ':':
            printf("Error: Missing required argument\n");
            free(ctx);
            exit(1);
          case '?':
            printf("Error: Unrecognized option\n");
            free(ctx);
            exit(1);
        }
    }

  /* Set up memory monitoring */

  ctx->mmbefore = mallinfo();
  ctx->mmprevious = ctx->mmbefore;

  test_nvs_mount(ctx);
  test_nvs_write(ctx);
  test_nvs_corrupt_expire(ctx);
  test_nvs_corrupted_write(ctx);
  test_nvs_gc(ctx);
  test_nvs_gc_3sectors(ctx);
  test_nvs_corrupted_sector_close(ctx);
  test_nvs_full_sector(ctx);
  test_nvs_gc_corrupt_close_ate(ctx);
  test_nvs_gc_corrupt_ate(ctx);
  test_nvs_gc_touched_deleted_ate(ctx);
  test_nvs_gc_touched_expired_ate(ctx);
  test_nvs_gc_not_touched_expired_ate(ctx);

  /* Show memory usage */

  mtdnvs_endmemusage(ctx);
  fflush(stdout);
  free(ctx);
  return 0;
}

