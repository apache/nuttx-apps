/****************************************************************************
 * apps/boot/nxboot/include/nxboot.h
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

#ifndef __BOOT_NXBOOT_INCLUDE_NXBOOT_H
#define __BOOT_NXBOOT_INCLUDE_NXBOOT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <assert.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ERROR                      -1

#define NXBOOT_PRIMARY_SLOT_NUM   (0)
#define NXBOOT_SECONDARY_SLOT_NUM (1)
#define NXBOOT_TERTIARY_SLOT_NUM  (2)

#define NXBOOT_HEADER_MAGIC     0x534f584e /* NXOS. The NX images, both
                                            * uploaded directly to primary
                                            * partition via debugger and to
                                            * update via some application
                                            * are used with this magic. If
                                            * this image is uploaded to
                                            * primary flash, it is considered
                                            * valid.
                                            */
#define NXBOOT_HEADER_MAGIC_INT 0xaca0abb0 /* NXOS internal. This is used
                                            * for internal bootloader
                                            * handling and operations. It is
                                            * switch internally to distinguish
                                            * between images uploaded via
                                            * debugger or the ones updated
                                            * after the bootloader performed
                                            * its operation. The first two
                                            * bits are reserved to point
                                            * what partition is a recovery
                                            * for this image.
                                            */

#define NXBOOT_HEADER_MAGIC_INT_MASK 0xfffffff0
#define NXBOOT_RECOVERY_PTR_MASK 0x3

#define NXBOOT_HEADER_PRERELEASE_MAXLEN 94

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum nxboot_update_type
{
  NXBOOT_UPDATE_TYPE_NONE = 0,    /* No action to do */
  NXBOOT_UPDATE_TYPE_UPDATE = 1,  /* Update will take place upon reboot */
  NXBOOT_UPDATE_TYPE_REVERT = 2,  /* Revert will take place upon reboot */
};

/* Versioning is according to Semantic Versioning 2.0.0
 * refer to (https://semver.org/spec/v2.0.0.html)
 */

struct nxboot_img_version
{
  uint16_t major;        /* MAJOR version */
  uint16_t minor;        /* MINOR version */
  uint16_t patch;        /* PATCH version */

  char pre_release[NXBOOT_HEADER_PRERELEASE_MAXLEN];  /* Additional pre-release version */
};

struct nxboot_hdr_version
{
  uint8_t major;
  uint8_t minor;
};

struct nxboot_img_header
{
  uint32_t magic;                         /* Header magic */
  struct nxboot_hdr_version hdr_version;  /* Version of the header */

  uint16_t header_size;     /* Length of the header in bytes */
  uint32_t crc;             /* CRC32 of image (excluding the previous
                             * fields in header, but including the following
                             * ones).
                             */
  uint32_t size;            /* Image size (excluding the header) */
  uint64_t identifier;      /* Platform identifier. An image is rejected
                             * if it does not match the one set for
                             * the bootloader in NXBOOT_PLATFORM_IDENTIFIER.
                             */
  uint32_t extd_hdr_ptr;    /* Address of the next extended header.
                             * This is a hook for future additional headers.
                             */

  struct nxboot_img_version img_version; /* Image version */
};

static_assert(CONFIG_NXBOOT_HEADER_SIZE > sizeof(struct nxboot_img_header),
              "CONFIG_NXBOOT_HEADER_SIZE has to be larger than"
              "sizeof(struct nxboot_img_header)");

struct nxboot_state
{
  int update;                         /* Number of update slot */
  int recovery;                       /* Number of recovery slot */
  bool recovery_valid;                /* True if recovery image contains valid recovery */
  bool recovery_present;              /* True if the image in primary has a recovery */
  bool primary_confirmed;             /* True if primary slot is confirmed */
  enum nxboot_update_type next_boot;  /* nxboot_update_type with next operation */
};

enum progress_type_e
{
  nxboot_info = 0,         /* Prefixes arg. string with "INFO:" */
  nxboot_error,            /* Prefixes arg. string with "ERR:" */
  nxboot_progress_start,   /* Prints arg. string with no newline to allow ..... sequence to follow */
  nxboot_progress_dot,     /* Prints of a "." to the ..... progress sequence */
  nxboot_progress_percent, /* Displays progress as % remaining */
  nxboot_progress_end,     /* Flags end of a "..." progrees sequence and prints newline */
};

enum progress_msg_e
{
  startup_msg              = 0,
  power_reset,
  soft_reset,
  found_bootable_image,
  no_bootable_image,
  boardioc_image_boot_fail,
  ramcopy_started,
  recovery_revert,
  recovery_create,
  update_from_update,
  validate_primary,
  validate_recovery,
  validate_update,
  recovery_created,
  recovery_invalid,
  update_failed,
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_PROGRESS
void nxboot_progress(enum progress_type_e type, ...);
#else
#define nxboot_progress(type, ...) do {} while (0)
#endif

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
 *   OK (0) on success, ERROR (-1) and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_get_state(struct nxboot_state *state);

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
 *   Valid file descriptor on success, ERROR (-1) and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_open_update_partition(void);

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
 *   1 if confirmed, OK (0) on success, ERROR (-1) and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_get_confirm(void);

/****************************************************************************
 * Name: nxboot_confirm
 *
 * Description:
 *   Confirms the image currently located in primary partition and marks
 *   its copy in update partition as a recovery.
 *
 * Returned Value:
 *   OK (0) on success, ERROR (-1) and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_confirm(void);

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
 *   OK (0) on success, ERROR (-1) and sets errno on failure.
 *
 ****************************************************************************/

int nxboot_perform_update(bool check_only);

/****************************************************************************
 * Name: nxboot_ramcopy
 *
 * Description:
 *   Copies the (already) validate bootable image to RAM memory
 *
 *   NOTE - no checking that the RAM location is correct, nor that the
 *          image size is appropriate for that RAM address!
 *
 * Input parameters:
 *   none
 *
 * Returned Value:
 *   OK (0) on success, ERROR (-1) on fail
 *
 ****************************************************************************/

int nxboot_ramcopy(void);

#endif /* __BOOT_NXBOOT_INCLUDE_NXBOOT_H */
