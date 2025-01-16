/****************************************************************************
 * apps/nshlib/nsh_mmcmds.c
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
#include <string.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_FREE) && defined(NSH_HAVE_CATFILE)

/****************************************************************************
 * Name: cmd_free
 ****************************************************************************/

int cmd_free(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  return nsh_catfile(vtbl, argv[0], CONFIG_NSH_PROC_MOUNTPOINT "/meminfo");
}

#endif /* !CONFIG_NSH_DISABLE_FREE && NSH_HAVE_CATFILE */

#if !defined(CONFIG_NSH_DISABLE_MEMDUMP) && defined(NSH_HAVE_WRITEFILE)

/****************************************************************************
 * Name: cmd_memdump
 ****************************************************************************/

int cmd_memdump(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *arg;
  int i;

  arg = lib_get_tempbuffer(LINE_MAX);
  if (arg == NULL)
    {
      return -ENOMEM;
    }

  arg[0] = '\0';

  if (argc == 1)
    {
      strlcpy(arg, "used", LINE_MAX);
    }
  else if (argc >= 2 && (strcmp(argv[1], "-h") == 0 ||
                         strcmp(argv[1], "help") == 0))
    {
      i = nsh_catfile(vtbl, argv[0],
                      CONFIG_NSH_PROC_MOUNTPOINT "/memdump");
      lib_put_tempbuffer(arg);
      return i;
    }
  else
    {
      for (i = 1; i < argc; i++)
        {
          strlcat(arg, argv[i], LINE_MAX);
          if (i < argc - 1)
            {
              strlcat(arg, " ", LINE_MAX);
            }
        }
    }

  i = nsh_writefile(vtbl, argv[0], arg, strlen(arg),
                    CONFIG_NSH_PROC_MOUNTPOINT "/memdump");
  lib_put_tempbuffer(arg);
  return i;
}

#endif /* !CONFIG_NSH_DISABLE_MEMDUMP && NSH_HAVE_WRITEFILE */
