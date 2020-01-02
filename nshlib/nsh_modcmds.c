/****************************************************************************
 * apps/nshlib/nsh_modcmds.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <string.h>

#include <nuttx/module.h>

#include "nsh.h"
#include "nsh_console.h"

#if defined(CONFIG_MODULE) && !defined(CONFIG_NSH_DISABLE_MODCMDS)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_insmod
 ****************************************************************************/

int cmd_insmod(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR void *handle;

  /* Usage: insmod <filepath> <modulename> */
  /* Install the module  */

  handle = insmod(argv[1], argv[2]);
  if (handle == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "insmod", NSH_ERRNO);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: cmd_rmmod
 ****************************************************************************/

int cmd_rmmod(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR void *handle;
  int ret;

  /* Usage: rmmod <modulename> */
  /* Get the module handle associated with the name */

  handle = modhandle(argv[1]);
  if (handle == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "modhandle", NSH_ERRNO);
      return ERROR;
    }

  /* Remove the module  */

  ret = rmmod(handle);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "rmmod", NSH_ERRNO);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: cmd_lsmod
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_MODULE)
int cmd_lsmod(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FILE *stream;

  /* Usage: lsmod */
  /* Open /proc/modules */

  stream = fopen("/proc/modules", "r");
  if (stream == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "fopen", NSH_ERRNO);
      return ERROR;
    }

  /* Output a Header */

  nsh_output(vtbl, "%-16s %8s %8s %8s %8s %8s %8s %8s %8s\n",
             "NAME", "INIT", "UNINIT", "ARG", "NEXPORTS", "TEXT", "SIZE",
             "DATA", "SIZE");

  /* Read each line from the procfs "file" */

  while (fgets(vtbl->iobuffer, IOBUFFERSIZE, stream) != NULL)
    {
      FAR char *modulename;
      FAR char *initializer;
      FAR char *uninitializer;
      FAR char *arg;
      FAR char *nexports;
      FAR char *text;
      FAR char *textsize;
      FAR char *data;
      FAR char *datasize;
      FAR char *lasts;

      /* Form of returned data is:
       *
       *  "%s,%p,%p,%p,%p,%lu,%p,%lu\n"
       *
       *   1) Module name (string)
       *   2) Initializer address (hex)
       *   3) Uninitializer address (hex)
       *   4) Uninitializer argument (hex)
       *   5) .text address (hex)
       *   6) Size of .text (decimal)
       *   7) .bss/.data address (hex)
       *   8) Size of .bss/.data (decimal)
       */

      modulename    = strtok_r(vtbl->iobuffer, ",\n", &lasts);
      initializer   = strtok_r(NULL, ",\n", &lasts);
      uninitializer = strtok_r(NULL, ",\n", &lasts);
      arg           = strtok_r(NULL, ",\n", &lasts);
      nexports      = strtok_r(NULL, ",\n", &lasts);
      text          = strtok_r(NULL, ",\n", &lasts);
      textsize      = strtok_r(NULL, ",\n", &lasts);
      data          = strtok_r(NULL, ",\n", &lasts);
      datasize      = strtok_r(NULL, ",\n", &lasts);

      nsh_output(vtbl, "%-16s %8s %8s %8s %8s %8s %8s %8s %8s\n",
                 modulename,
                 initializer   ? initializer   : "",
                 uninitializer ? uninitializer : "",
                 arg           ? arg           : "",
                 nexports      ? nexports      : "",
                 text          ? text          : "",
                 textsize      ? textsize      : "",
                 data          ? data          : "",
                 datasize      ? datasize      : "");
    }

  fclose(stream);
  return OK;
}
#endif /* CONFIG_FS_PROCFS && !CONFIG_FS_PROCFS_EXCLUDE_MODULE */

#endif /* CONFIG_MODULE && !CONFIG_NSH_DISABLE_MODCMDS */
