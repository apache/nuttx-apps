/****************************************************************************
 * apps/boot/nxboot/loader/boot.c
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
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <syslog.h>
#include <sys/param.h>

#include <nuttx/crc32.h>
#include <nxboot.h>

#include "flash.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_PREVENT_DOWNGRADE
  # warning "Downgrade prevention currently ignores prerelease."
#endif

#define IS_INTERNAL_MAGIC(magic) ((magic & NXBOOT_HEADER_MAGIC_INT_MASK) \
                                  == NXBOOT_HEADER_MAGIC_INT)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void get_image_header(int fd, struct nxboot_img_header *header)
{
  int ret;
  ret = flash_partition_read(fd, header, sizeof *header, 0);
  if (ret < 0)
    {
      /* Something went wrong, treat the partition as empty. */

      memset(header, 0, sizeof *header);
    }
}

static inline bool validate_image_header(struct nxboot_img_header *header)
{
  return (header->magic == NXBOOT_HEADER_MAGIC ||
         IS_INTERNAL_MAGIC(header->magic)) &&
         header->identifier == CONFIG_NXBOOT_PLATFORM_IDENTIFIER;
}

static uint32_t calculate_crc(int fd, struct nxboot_img_header *header)
{
  char *buf;
  int remain;
  int readsiz;
  off_t off;
  uint32_t crc;
  struct flash_partition_info info;
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT
  int total_size;
#endif  

  if (flash_partition_info(fd, &info) < 0)
    {
      return false;
    }

  buf = malloc(info.blocksize);
  if (!buf)
    {
      return false;
    }

  crc = 0xffffffff;
  off = offsetof(struct nxboot_img_header, crc) + sizeof crc;
  remain = header->size + header->header_size - off;
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT
  total_size = remain;
#endif
  while (remain > 0)
    {
      readsiz = remain > info.blocksize ? info.blocksize : remain;
      if (flash_partition_read(fd, buf, readsiz, off) != 0)
        {
          free(buf);
          return 0xffffffff;
        }

      off += readsiz;
      remain -= readsiz;
      crc = crc32part((uint8_t *)buf, readsiz, crc);
      if ((remain % 25) == 0)
        {
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT
          nxboot_progress(nxboot_progress_percent,
                          ((total_size - remain) * 100) / total_size);
#else
          nxboot_progress(nxboot_progress_dot);
#endif
        }
    }

  free(buf);
  return ~crc;
}

static int copy_partition(int from, int where, struct nxboot_state *state,
                          bool update)
{
  struct nxboot_img_header header;
  struct flash_partition_info info_from;
  struct flash_partition_info info_where;
  uint32_t magic;
  int readsiz;
  int remain;
  int blocksize;
  off_t off;
  char *buf;
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT  
  int total_size;
#endif

  get_image_header(from, &header);

  if (flash_partition_info(from, &info_from) < 0)
    {
      return ERROR;
    }

  if (flash_partition_info(where, &info_where) < 0)
    {
      return ERROR;
    }

  remain = header.size + header.header_size;
  if (remain > info_where.size)
    {
      return ERROR;
    }

#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT
  total_size = remain * 100;
#endif
  blocksize = MAX(info_from.blocksize, info_where.blocksize);

  buf = malloc(blocksize);
  if (!buf)
    {
      return ERROR;
    }

  /* Flip header's magic. We go from standard to internal in case of
   * image update and from internal to standard in case of recovery
   * creation or revert operation.
   */

  magic = IS_INTERNAL_MAGIC(header.magic) ?
    NXBOOT_HEADER_MAGIC : NXBOOT_HEADER_MAGIC_INT;

  if (update)
    {
      /* This is update operation, add pointer to recovery image to the
       * image's magic.
       */

      magic |= state->update;
    }

  if (flash_partition_read(from, buf, blocksize, 0) < 0)
    {
      free(buf);
      return ERROR;
    }

  memcpy(buf + offsetof(struct nxboot_img_header, magic), &magic,
          sizeof magic);
  if (flash_partition_write(where, buf, blocksize, 0) < 0)
    {
      free(buf);
      return ERROR;
    }

  off = blocksize;
  remain -= blocksize;

  while (remain > 0)
    {
      readsiz = remain > blocksize ? blocksize : remain;
      if (flash_partition_read(from, buf, readsiz, off) < 0)
        {
          free(buf);
          return ERROR;
        }

      if (flash_partition_write(where, buf, readsiz, off) < 0)
        {
          free(buf);
          return ERROR;
        }

      off += readsiz;
      remain -= readsiz;
      if ((remain % 25) == 0)
        {
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT
          nxboot_progress(nxboot_progress_percent,
                          ((total_size - remain) * 100) / total_size);
#else
          nxboot_progress(nxboot_progress_dot);
#endif
        }
    }

  free(buf);
  return OK;
}

static bool validate_image(int fd)
{
  struct nxboot_img_header header;

  get_image_header(fd, &header);
  if (!validate_image_header(&header))
    {
      return false;
    }

  syslog(LOG_INFO, "Validating image.\n");
  return calculate_crc(fd, &header) == header.crc;
}

static bool compare_versions(struct nxboot_img_version *v1,
                             struct nxboot_img_version *v2)
{
#ifndef CONFIG_NXBOOT_PREVENT_DOWNGRADE
  int i;
  if (v1->major != v2->major ||
      v1->minor != v2->minor ||
      v1->patch != v2->patch)
    {
      return false;
    }

  for (i = 0; i < NXBOOT_HEADER_PRERELEASE_MAXLEN; i++)
    {
      if (v1->pre_release[i] != v2->pre_release[i])
        {
          return false;
        }
    }

  return true;
#else
  if (v1->major > v2->major ||
      v1->minor > v2->minor ||
      v1->patch > v2->patch)
    {
      return true;
    }

  if (v1->major == v2->major &&
      v1->minor == v2->minor &&
      v1->patch == v2->patch)
    {
      /* TODO: compare prerelease */
    }

  return false;
#endif
}

static enum nxboot_update_type
  get_update_type(struct nxboot_state *state,
                  int primary, int update, int recovery,
                  struct nxboot_img_header *primary_header,
                  struct nxboot_img_header *update_header,
                  struct nxboot_img_header *recovery_header)
{
  nxboot_progress(nxboot_progress_start, validate_primary);
  bool primary_valid = validate_image(primary);
  nxboot_progress(nxboot_progress_end);

  nxboot_progress(nxboot_progress_start, validate_update);
  if (update_header->magic == NXBOOT_HEADER_MAGIC && validate_image(update))
    {
      if (primary_header->crc != update_header->crc ||
          !compare_versions(&primary_header->img_version,
          &update_header->img_version) || !primary_valid)
        {
          nxboot_progress(nxboot_progress_end);
          return NXBOOT_UPDATE_TYPE_UPDATE;
        }
    }

  nxboot_progress(nxboot_progress_end);

  if (IS_INTERNAL_MAGIC(recovery_header->magic) && state->recovery_valid &&
      ((IS_INTERNAL_MAGIC(primary_header->magic) &&
      !state->primary_confirmed) || !primary_valid))
    {
      return NXBOOT_UPDATE_TYPE_REVERT;
    }

  return NXBOOT_UPDATE_TYPE_NONE;
}

static int perform_update(struct nxboot_state *state, bool check_only)
{
  int successful;
  int update;
  int recovery;
  int primary;
  int secondary;
  int tertiary;
  bool primary_valid;

  primary = flash_partition_open(CONFIG_NXBOOT_PRIMARY_SLOT_PATH);
  if (primary < 0)
    {
      return ERROR;
    }

  secondary = flash_partition_open(CONFIG_NXBOOT_SECONDARY_SLOT_PATH);
  if (secondary < 0)
    {
      flash_partition_close(primary);
      return ERROR;
    }

  tertiary = flash_partition_open(CONFIG_NXBOOT_TERTIARY_SLOT_PATH);
  if (tertiary < 0)
    {
      flash_partition_close(primary);
      flash_partition_close(secondary);
      return ERROR;
    }

  if (state->update == NXBOOT_SECONDARY_SLOT_NUM)
    {
      update = secondary;
      recovery = tertiary;
    }
  else
    {
      update = tertiary;
      recovery = secondary;
    }

  nxboot_progress(nxboot_progress_start, validate_primary);
  if (state->next_boot == NXBOOT_UPDATE_TYPE_REVERT &&
      (!check_only || !validate_image(primary)))
    {
      nxboot_progress(nxboot_progress_end);
      if (state->recovery_valid)
        {
          syslog(LOG_INFO, "Reverting image to recovery.\n");
          nxboot_progress(nxboot_progress_start, recovery_revert);
          copy_partition(recovery, primary, state, false);
          nxboot_progress(nxboot_progress_end);
        }
    }
  else
    {
      nxboot_progress(nxboot_progress_end);
      nxboot_progress(nxboot_progress_start, validate_primary);
      primary_valid = validate_image(primary);
      nxboot_progress(nxboot_progress_end);
      if (primary_valid && check_only)
        {
          /* Skip if primary image is valid (does not mather whether
           * confirmed or not) and check_only option is set.
           */

          goto perform_update_done;
        }

      if ((!state->recovery_present || !state->recovery_valid) &&
          state->primary_confirmed && primary_valid)
        {
          /* Save current image as recovery only if it is valid and
           * confirmed. We have to check this in case of restart
           * during update process.
           * If board is restarted during update, primary slot contains
           * non-valid image and we do not want to copy this image to
           * recovery slot.
           * There also might be a case where primary image is valid
           * but not confirmed (timing of board reset right after
           * update is uploaded to secondary). Still we do not want
           * to upload this to recovery.
           */

          syslog(LOG_INFO, "Creating recovery image.\n");
          nxboot_progress(nxboot_progress_start, recovery_create);
          copy_partition(primary, recovery, state, false);
          nxboot_progress(nxboot_progress_end);
          nxboot_progress(nxboot_progress_start, validate_recovery);
          successful = validate_image(recovery);
          nxboot_progress(nxboot_progress_end);
          if (!successful)
            {
              syslog(LOG_INFO,
                            "New recovery is not valid,stop update.\n");
              nxboot_progress(nxboot_info, recovery_invalid);
              goto perform_update_done;
            }

          syslog(LOG_INFO, "Recovery image created.\n");
          nxboot_progress(nxboot_info, recovery_created);
        }

      nxboot_progress(nxboot_progress_start, validate_update);
      successful = validate_image(update);
      nxboot_progress(nxboot_progress_end);
      if (successful)
        {
          /* Perform update only if update slot contains valid image. */

          syslog(LOG_INFO, "Updating from update image.\n");
          nxboot_progress(nxboot_progress_start, update_from_update);
          if (copy_partition(update, primary, state, true) >= 0)
            {
              /* Erase the first sector of update partition. This marks the
               * partition as updated so we don't end up in an update loop.
               * The sector is written back again during the image
               * confirmation.
               */

              flash_partition_erase_first_sector(update);
            }

          nxboot_progress(nxboot_progress_end);
        }
    }

perform_update_done:
  flash_partition_close(primary);
  flash_partition_close(secondary);
  flash_partition_close(tertiary);
  return OK;
}

#ifdef CONFIG_NXBOOT_COPY_TO_RAM
int nxboot_ramcopy(void)
{
  int ret = OK;
  int primary;
  struct nxboot_img_header header;
  ssize_t bytes;
  static uint8_t *buf;

  primary = flash_partition_open(CONFIG_NXBOOT_PRIMARY_SLOT_PATH);
  if (primary < 0)
    {
      return ERROR;
    }

  get_image_header(primary, &header);
  buf = malloc(header.size);
  if (!buf)
    {
      ret = ERROR;
      goto exit_with_error;
    }

  bytes = pread(primary, buf, header.size, header.header_size);
  if (bytes != header.size)
    {
      ret = ERROR;
      goto exit_with_error;
    }

  memcpy((uint32_t *)CONFIG_NXBOOT_RAMSTART, buf, header.size);

exit_with_error:
  flash_partition_close(primary);
  free(buf);

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxboot_get_state
 *
 * Description:
 *   Gets the current bootloader state and stores it in the nxboot_state
 *   structure passed as an argument. This function may be used to determine
 *   which slot is update slot and where should application save incoming
 *   firmware.
 *
 * Input parameters:
 *   state: The pointer to nxboot_state structure. The state is stored here.
 *
 * Returned Value:
 *   0 on success, -1 and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_get_state(struct nxboot_state *state)
{
  int primary;
  int secondary;
  int tertiary;
  int update;
  int recovery;
  int recovery_pointer;
  struct nxboot_img_header primary_header;
  struct nxboot_img_header secondary_header;
  struct nxboot_img_header tertiary_header;
  struct nxboot_img_header *update_header;
  struct nxboot_img_header *recovery_header;

  memset(state, 0, sizeof *state);

  primary = flash_partition_open(CONFIG_NXBOOT_PRIMARY_SLOT_PATH);
  if (primary < 0)
    {
      return ERROR;
    }

  secondary = flash_partition_open(CONFIG_NXBOOT_SECONDARY_SLOT_PATH);
  if (secondary < 0)
    {
      flash_partition_close(primary);
      return ERROR;
    }

  tertiary = flash_partition_open(CONFIG_NXBOOT_TERTIARY_SLOT_PATH);
  if (tertiary < 0)
    {
      flash_partition_close(primary);
      flash_partition_close(secondary);
      return ERROR;
    }

  get_image_header(primary, &primary_header);
  get_image_header(secondary, &secondary_header);
  get_image_header(tertiary, &tertiary_header);

  /* Determine which partition is for update and which is a recovery.
   * This depends on many factors, but in general a partition with
   * NXBOOT_HEADER_MAGIC is an update partition and partition with
   * NXBOOT_HEADER_MAGIC_INT is a recovery.
   *
   * A special case is when both partitions have NXBOOT_HEADER_MAGIC_INT,
   * then we look into a recovery pointer in primary header magic and
   * determine the recovery from it.
   */

  update = secondary;
  recovery = tertiary;
  update_header = &secondary_header;
  recovery_header = &tertiary_header;
  state->update = NXBOOT_SECONDARY_SLOT_NUM;
  state->recovery = NXBOOT_TERTIARY_SLOT_NUM;

  if (tertiary_header.magic == NXBOOT_HEADER_MAGIC)
    {
      update = tertiary;
      recovery = secondary;
      update_header = &tertiary_header;
      recovery_header = &secondary_header;
      state->recovery = NXBOOT_SECONDARY_SLOT_NUM;
      state->update = NXBOOT_TERTIARY_SLOT_NUM;
    }
  else if (IS_INTERNAL_MAGIC(secondary_header.magic) &&
           IS_INTERNAL_MAGIC(tertiary_header.magic))
    {
      if (IS_INTERNAL_MAGIC(primary_header.magic))
        {
          recovery_pointer = primary_header.magic & NXBOOT_RECOVERY_PTR_MASK;
          if (recovery_pointer == NXBOOT_SECONDARY_SLOT_NUM &&
              primary_header.crc == secondary_header.crc)
            {
              update = tertiary;
              recovery = secondary;
              update_header = &tertiary_header;
              recovery_header = &secondary_header;
              state->recovery = NXBOOT_SECONDARY_SLOT_NUM;
              state->update = NXBOOT_TERTIARY_SLOT_NUM;
            }
        }
      else if (primary_header.crc == secondary_header.crc)
        {
          update = tertiary;
          recovery = secondary;
          update_header = &tertiary_header;
          recovery_header = &secondary_header;
          state->recovery = NXBOOT_SECONDARY_SLOT_NUM;
          state->update = NXBOOT_TERTIARY_SLOT_NUM;
        }
    }
  else if (IS_INTERNAL_MAGIC(secondary_header.magic))
    {
      update = tertiary;
      recovery = secondary;
      update_header = &tertiary_header;
      recovery_header = &secondary_header;
      state->recovery = NXBOOT_SECONDARY_SLOT_NUM;
      state->update = NXBOOT_TERTIARY_SLOT_NUM;
    }

  nxboot_progress(nxboot_progress_start, validate_recovery);
  state->recovery_valid = validate_image(recovery);
  nxboot_progress(nxboot_progress_end);
  state->recovery_present = primary_header.crc == recovery_header->crc;

  /* The image is confirmed if it has either NXBOOT_HEADER_MAGIC or a
   * recovery exists.
   */

  if (primary_header.magic == NXBOOT_HEADER_MAGIC)
    {
      state->primary_confirmed = true;
    }
  else if (IS_INTERNAL_MAGIC(primary_header.magic))
    {
      recovery_pointer = primary_header.magic & NXBOOT_RECOVERY_PTR_MASK;
      if (recovery_pointer == NXBOOT_SECONDARY_SLOT_NUM)
        {
          state->primary_confirmed =
            primary_header.crc == secondary_header.crc;
        }
      else if (recovery_pointer == NXBOOT_TERTIARY_SLOT_NUM)
        {
          state->primary_confirmed =
            primary_header.crc == tertiary_header.crc;
        }
    }

  state->next_boot = get_update_type(state, primary, update, recovery,
                                     &primary_header, update_header,
                                     recovery_header);

  flash_partition_close(primary);
  flash_partition_close(secondary);
  flash_partition_close(tertiary);
  return OK;
}

/****************************************************************************
 * Name: nxboot_open_update_partition
 *
 * Description:
 *   Gets the current bootloader state and opens the partition to which an
 *   update image should be stored. It returns the valid file descriptor to
 *   this partition, the user is responsible for writing to it and closing
 *   if afterwards.
 *
 * Returned Value:
 *   Valid file descriptor on success, -1 and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_open_update_partition(void)
{
  char *path;
  struct nxboot_state state;

  nxboot_get_state(&state);

  path = state.update == NXBOOT_SECONDARY_SLOT_NUM ?
    CONFIG_NXBOOT_SECONDARY_SLOT_PATH : CONFIG_NXBOOT_TERTIARY_SLOT_PATH;

  return flash_partition_open(path);
}

/****************************************************************************
 * Name: nxboot_get_confirm
 *
 * Description:
 *   This function can be used to determine whether primary image is
 *   confirmed or not. This provides more direct access to confirm
 *   state compared to nxboot_get_state function that returns the full
 *   state of the bootloader.
 *
 * Returned Value:
 *   1 means confirmed, 0 not confirmed, -1 and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_get_confirm(void)
{
  int primary;
  int recovery;
  int recovery_pointer;
  char *path;
  int ret = OK;
  struct nxboot_img_header primary_header;
  struct nxboot_img_header recovery_header;

  primary = flash_partition_open(CONFIG_NXBOOT_PRIMARY_SLOT_PATH);
  if (primary < 0)
    {
      return ERROR;
    }

  get_image_header(primary, &primary_header);

  if (primary_header.magic == NXBOOT_HEADER_MAGIC)
    {
      close(primary);
      return 1;
    }
  else if (IS_INTERNAL_MAGIC(primary_header.magic))
    {
      recovery_pointer = primary_header.magic & NXBOOT_RECOVERY_PTR_MASK;
      if (recovery_pointer != 0)
        {
          path = recovery_pointer == NXBOOT_SECONDARY_SLOT_NUM ?
            CONFIG_NXBOOT_SECONDARY_SLOT_PATH :
            CONFIG_NXBOOT_TERTIARY_SLOT_PATH;

          recovery = flash_partition_open(path);
          if (recovery < 0)
            {
              close(primary);
              return ERROR;
            }

          get_image_header(recovery, &recovery_header);
          if (primary_header.crc == recovery_header.crc)
            {
              ret = 1;
            }

          close(recovery);
        }
    }

  close(primary);
  return ret;
}

/****************************************************************************
 * Name: nxboot_confirm
 *
 * Description:
 *   Confirms the image currently located in primary partition and marks
 *   its copy in update partition as a recovery.
 *
 * Returned Value:
 *   0 on success, -1 and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_confirm(void)
{
  int ret;
  int update;
  int primary;
  int remain;
  int readsiz;
  char *path;
  char *buf;
  off_t off;
  struct nxboot_state state;
  struct flash_partition_info info_update;

  ret = OK;
  nxboot_get_state(&state);
  if (state.primary_confirmed)
    {
      return OK;
    }

  path = state.update == NXBOOT_SECONDARY_SLOT_NUM ?
    CONFIG_NXBOOT_SECONDARY_SLOT_PATH : CONFIG_NXBOOT_TERTIARY_SLOT_PATH;

  primary = flash_partition_open(CONFIG_NXBOOT_PRIMARY_SLOT_PATH);
  if (primary < 0)
    {
      return ERROR;
    }

  update = flash_partition_open(path);
  if (update < 0)
    {
      flash_partition_close(primary);
      return ERROR;
    }

  /* Confirm the image by creating a recovery. The recovery image is
   * already present in the update slot, but without the first erase
   * page. Therefore, just copy the first erase page to the update slot.
   */

  if (flash_partition_info(update, &info_update) < 0)
    {
      ret = ERROR;
      goto confirm_done;
    }

  /* Write by write pages to avoid large array buffering. */

  buf = malloc(info_update.blocksize);
  remain = info_update.erasesize;
  off = 0;

  while (remain > 0)
    {
      readsiz = remain > info_update.blocksize ?
        info_update.blocksize : remain;
      if (flash_partition_read(primary, buf, readsiz, off) < 0)
        {
          free(buf);
          ret = ERROR;
          goto confirm_done;
        }

      if (flash_partition_write(update, buf, readsiz, off) < 0)
        {
          free(buf);
          ret = ERROR;
          goto confirm_done;
        }

      off += readsiz;
      remain -= readsiz;
    }

  free(buf);

confirm_done:
  flash_partition_close(primary);
  flash_partition_close(update);

  return ret;
}

/****************************************************************************
 * Name: nxboot_perform_update
 *
 * Description:
 *   Checks for the possible firmware update and performs it by copying
 *   update image to primary slot or recovery image to primary slot in case
 *   of the revert. In any situation, this function ends with the valid
 *   image in primary slot.
 *
 *   This is an entry point function that should be called from the
 *   bootloader application.
 *
 * Input parameters:
 *   check_only: Only repairs corrupted update, but do not start another one
 *
 * Returned Value:
 *   0 on success, -1 and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_perform_update(bool check_only)
{
  int ret;
  int primary;
  struct nxboot_state state;
  struct nxboot_img_header header;

  ret = nxboot_get_state(&state);
  if (ret < 0)
    {
      return ERROR;
    }

  if (state.next_boot != NXBOOT_UPDATE_TYPE_NONE)
    {
      /* We either want to update or revert. */

      ret = perform_update(&state, check_only);
      if (ret < 0)
        {
          /* Update process failed, raise error and try to boot into
           * primary.
           */

          syslog(LOG_ERR, "Update process failed: %s\n", strerror(errno));
          nxboot_progress(nxboot_error, update_failed);
        }
    }

  /* Check whether there is a valid image in the primary slot. This just
   * checks whether the header is valid, but does not calculate the CRC
   * of the image as this would prolong the boot process.
   */

  primary = flash_partition_open(CONFIG_NXBOOT_PRIMARY_SLOT_PATH);
  if (primary < 0)
    {
      return ERROR;
    }

  get_image_header(primary, &header);
  if (!validate_image_header(&header))
    {
      ret = ERROR;
    }

  flash_partition_close(primary);

  return ret;
}

