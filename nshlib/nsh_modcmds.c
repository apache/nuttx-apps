/****************************************************************************
 * apps/nshlib/nsh_modcmds.c
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
