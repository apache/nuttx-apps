/****************************************************************************
 * apps/boot/nxboot/nxboot_main.c
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
#include <syslog.h>

#include <nxboot.h>
#include <sys/boardctl.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

bool progress_dots_started = false;

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS
static const char *g_progress_txt[] =
{
  /* Text that will be printed  ...from...    enum progress_msg_e  */

  "*** nxboot ***",                          /* startup_msg */
  "Power Reset detected, check images only", /* power_reset */
  "Soft reset detected, check for update",   /* soft_reset */
  "Found bootable image, boot from primary", /* found_bootable_image */
  "No bootable image found",                 /* no_bootable_image */
  "Board failed to boot bootable image",     /* boardioc_image_boot_fail */
  "Copying bootable image to RAM",           /* ramcopy_started */
  "Reverting image to recovery",             /* recovery_revert */
  "Creating recovery image",                 /* recovery_create */
  "Updating from update image",              /* update_from_update */
  "Validating primary image",                /* validate_primary */
  "Validating recovery image",               /* validate_recovery */
  "Validating update image",                 /* validate_update */
  "Recovery image created",                  /* recovery_created */
  "Recovery image invalid, update stopped",  /* recovery_invalid */
  "Update failed",                           /* update_failed */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxboot_progress
 *
 * Description:
 *   If enabled, thos function prints progress messages to stdout.
 *   Messages are handled via integer enums, allowing this function to be
 *   easily replaced if required with no changes needed to the underlying
 *   code
 *
 ****************************************************************************/

void nxboot_progress(enum progress_type_e type, ...)
{
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS
  va_list args;

  va_start(args, type);

  switch (type)
    {
      case nxboot_info:
        {
          int idx = va_arg(args, int);
          const char *msg = g_progress_txt[idx];
          dprintf(STDOUT_FILENO, "%s\n", msg);
        }
      break;
      case nxboot_error:
        {
          int idx = va_arg(args, int);
          const char *msg = g_progress_txt[idx];
          dprintf(STDOUT_FILENO, "ERROR: %s\n", msg);
        }
      break;
      case nxboot_progress_start:
        {
          int idx = va_arg(args, int);
          const char *msg = g_progress_txt[idx];
          dprintf(STDOUT_FILENO, "%s", msg);
          progress_dots_started = true;
        }
      break;
      case nxboot_progress_dot:
        {
          assert(progress_dots_started);
          if (!progress_dots_started)
            {
              printf("Progress dot requested "
                     "but no previous progress start\n");
            }
          else
            {
              dprintf(STDOUT_FILENO, ".");
            }
        }
      break;
      case nxboot_progress_end:
        {
          assert(progress_dots_started == false);
          if (!progress_dots_started)
            {
              printf("Progress dot stop requested "
                     "but no previous progress start\n");
            }
          else
            {
              dprintf(STDOUT_FILENO, "\n");
            }

          progress_dots_started = false;
        }
      break;
      default:
        {
          assert(false);
          dprintf(STDOUT_FILENO, "progress: unknown type!\n");
        }
      break;
    }

  va_end(args);
#endif /* CONFIG_NXBOOT_PRINTF_PROGRESS */
}

/****************************************************************************
 * Name: nxboot_main
 *
 * Description:
 *   NuttX bootloader entry point.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  struct boardioc_boot_info_s info;
  bool check_only;
#ifdef CONFIG_NXBOOT_SWRESET_ONLY
  FAR struct boardioc_reset_cause_s cause;
#endif

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif

  syslog(LOG_INFO, "*** nxboot ***\n");
  nxboot_progress(nxboot_info, startup_msg);

#ifdef CONFIG_NXBOOT_SWRESET_ONLY
  check_only = true;
  ret = boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&cause);
  if (ret >= 0)
    {
      if (cause.cause == BOARDIOC_RESETCAUSE_CPU_SOFT ||
          cause.cause == BOARDIOC_RESETCAUSE_CPU_RWDT ||
          cause.cause == BOARDIOC_RESETCAUSE_PIN)
        {
          check_only = false;
          nxboot_progress(nxboot_info, soft_reset);
        }
      else
        {
          syslog(LOG_INFO, "Power reset detected, performing check only.\n");
          nxboot_progress(nxboot_info, power_reset);
        }
    }
#else
  check_only = false;
#endif

  if (nxboot_perform_update(check_only) < 0)
    {
      syslog(LOG_ERR, "Could not find bootable image.\n");
      nxboot_progress(nxboot_error, no_bootable_image);
      return OK;
    }

  syslog(LOG_INFO, "Found bootable image, boot from primary.\n");
  nxboot_progress(nxboot_info, found_bootable_image);

#ifdef CONFIG_NXBOOT_COPY_TO_RAM
  syslog(LOG_INFO, "Copying image to RAM.\n");
  nxboot_ramcopy();
#endif

  /* Call board specific image boot */

  info.path        = CONFIG_NXBOOT_PRIMARY_SLOT_PATH;
  info.header_size = CONFIG_NXBOOT_HEADER_SIZE;

  ret = boardctl(BOARDIOC_BOOT_IMAGE, (uintptr_t)&info);

  /* Only get here if the board boot fails */

  nxboot_progress(nxboot_error, boardioc_image_boot_fail);

  return ret;
}

