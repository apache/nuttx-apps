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
#include <nuttx/ascii.h>
#include <sys/param.h>
#include <nxboot.h>
#include <sys/boardctl.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_PROGRESS
static bool g_progress_started = false;
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT
static bool g_progress_percent_started = false;
#endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS

#  ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT  
static const char backtab[] =
{
  ASCII_BS, ASCII_BS, ASCII_BS, ASCII_BS, '\0',
};
#  endif

static const char *progress_msgs[] =
{
  [startup_msg]              = "*** nxboot ***",
  [power_reset]              = "Power Reset detected, check images only",
  [soft_reset]               = "Soft reset detected, check for update",
  [found_bootable_image]     = "Found bootable image, boot from primary",
  [no_bootable_image]        = "No bootable image found",
  [boardioc_image_boot_fail] = "Board failed to boot bootable image",
  [ramcopy_started]          = "Copying bootable image to RAM",
  [recovery_revert]          = "Reverting image to recovery",
  [recovery_create]          = "Creating recovery image",
  [update_from_update]       = "Updating from update image",
  [validate_primary]         = "Validating primary image",
  [validate_recovery]        = "Validating recovery image",
  [validate_update]          = "Validating update image",
  [recovery_created]         = "Recovery image created",
  [recovery_invalid]         = "Recovery image invalid, update stopped",
  [update_failed]            = "Update failed",
};
#endif /* CONFIG_NXBOOT_PRINTF_PROGRESS */

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

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
 *   If enabled, this function prints progress messages to stdout.
 *   Messages are handled via integer enums, allowing this function to be
 *   easily replaced if required with no changes needed to the underlying
 *   code
 *
 * Input Parameters:
 *   type - the progress message type to be printed, as per progress_type_e:
 *          - nxboot_info:             Prefixes arg. string with "INFO:"
 *          - nxboot_error:            Prefixes arg. string with "ERR:"
 *          - nxboot_progress_start:   Prints arg. string with no newline
 *                                     to allow a ..... sequence
 *                                     or % remaining to follow
 *          - nxboot_progress_dot:     Prints a "." to the ..... sequence
 *          - nxboot_progress_percent: Displays progress as % remaining
 *          - nxboot_progress_end,     Flags end of a progrees sequence
 *
 *   ... - variadic argument:
 *          - the enum (int) reference to the message string or
 *          - the % remaining in the case of type: nxboot_progress_percent
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_PROGRESS
void nxboot_progress(enum progress_type_e type, ...)
{
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS
  va_list arg;

  va_start(arg, type);

  switch (type)
    {
      case nxboot_info:
        {
          int idx = va_arg(arg, int);
          assert(progress_msgs[idx]);
          if (strlen(progress_msgs[idx]) == 0)
            {
              syslog(LOG_ERR, "No progress message for idx: %d\n", idx);
              break;
            }

          dprintf(STDOUT_FILENO, "%s\n", progress_msgs[idx]);
        }
      break;
      case nxboot_error:
        {
          int idx = va_arg(arg, int);
          assert(progress_msgs[idx]);
          if (strlen(progress_msgs[idx]) == 0)
            {
              syslog(LOG_ERR, "No progress message for idx: %d\n", idx);
              break;
            }

          dprintf(STDOUT_FILENO, "ERROR: %s\n", progress_msgs[idx]);
        }
      break;
      case nxboot_progress_start:
        {
          int idx = va_arg(arg, int);
          assert(progress_msgs[idx]);
          if (strlen(progress_msgs[idx]) == 0)
            {
              syslog(LOG_ERR, "No progress message for idx: %d\n", idx);
              break;
            }

          dprintf(STDOUT_FILENO, "%s", progress_msgs[idx]);
          g_progress_started = true;
        }
      break;
      case nxboot_progress_dot:
        {
          assert(g_progress_started);
          if (!g_progress_started)
            {
              syslog(LOG_ERR, "Progress dot requested "
                              "but no previous progress start\n");
            }
          else
            {
              dprintf(STDOUT_FILENO, ".");
            }
        }
      break;
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT      
      case nxboot_progress_percent:
        {
          assert(g_progress_started);
          if (!g_progress_started)
            {
              syslog(LOG_ERR, "Progress percent requested "
                              "but no previous progress start\n");
            }
          else
            {
              int percent = va_arg(arg, int);
              if (!g_progress_percent_started)
                {
                  g_progress_percent_started = true;
                  dprintf(STDOUT_FILENO, ": ");
                }
              else
                {
                  dprintf(STDOUT_FILENO, "%s", backtab);
                }

              dprintf(STDOUT_FILENO, "%3d%%", MIN(percent, 100));
            }
        }
      break;
#endif
      case nxboot_progress_end:
        {
          assert(g_progress_started);
          if (!g_progress_started)
            {
              syslog(LOG_ERR, "Progress dot stop requested "
                              "but no previous progress start\n");
            }
          else
            {
              dprintf(STDOUT_FILENO, "\n");
            }

          g_progress_started = false;
#ifdef CONFIG_NXBOOT_PRINTF_PROGRESS_PERCENT   
          g_progress_percent_started = false;
#endif
        }
      break;
      default:
        {
          assert(false);
          syslog(LOG_ERR, "Invalid progress message type: %d\n", type);
          dprintf(STDOUT_FILENO, "progress: unknown type!\n");
        }
      break;
    }

  va_end(arg);
#endif /* CONFIG_NXBOOT_PRINTF_PROGRESS */
}
#endif

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
