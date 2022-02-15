/****************************************************************************
 * apps/system/readline/readline.c
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
#include <assert.h>

#include "system/readline.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: readline
 *
 *   readline() is same to readline_fd() but accept a file stream instead
 *   of a file handle.
 *
 ****************************************************************************/

#ifdef CONFIG_FILE_STREAM
ssize_t readline(FAR char *buf, int buflen, FILE *instream, FILE *outstream)
{
  /* Sanity checks */

  DEBUGASSERT(instream && outstream);

  /* Let readline_fd do the work */

  return readline_fd(buf, buflen, instream->fs_fd, outstream->fs_fd);
}
#endif
