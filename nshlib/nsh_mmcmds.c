/****************************************************************************
 * apps/nshlib/nsh_mmcmds.c
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

#include "nsh.h"
#include "nsh_console.h"

#if !defined(CONFIG_NSH_DISABLE_FREE) && defined(NSH_HAVE_CATFILE)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_free
 ****************************************************************************/

int cmd_free(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  return nsh_catfile(vtbl, argv[0], CONFIG_NSH_PROC_MOUNTPOINT "/meminfo");
}

#endif /* !CONFIG_NSH_DISABLE_FREE && NSH_HAVE_CATFILE */
