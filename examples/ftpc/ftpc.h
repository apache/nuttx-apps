/****************************************************************************
 * apps/examples/ftpc/ftpc.h
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

#ifndef __APPS_EXAMPLES_FTPC_FTPC_H
#define __APPS_EXAMPLES_FTPC_FTPC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "netutils/ftpc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum size of one command line */

#ifndef CONFIG_FTPC_LINELEN
#  define CONFIG_FTPC_LINELEN 80
#endif

/* If CONFIG_STDIO_LINEBUFFER is defined, the STDIO buffer will be flushed
 * on each new line.  Otherwise, STDIO needs to be explicitly flushed to
 * see the output in context.
 */

#if defined(CONFIG_FILE_STREAM) && CONFIG_STDIO_BUFFER_SIZE > 0 && \
    !defined(CONFIG_STDIO_LINEBUFFER)
#  define FFLUSH() fflush(stdout)
#else
#  define FFLUSH()
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef int (*cmd_t)(SESSION handle, int argc, char **argv);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* FTP command handlers */

extern int cmd_rlogin(SESSION handle, int argc, char **argv);
extern int cmd_rquit(SESSION handle, int argc, char **argv);
extern int cmd_rchdir(SESSION handle, int argc, char **argv);
extern int cmd_rpwd(SESSION handle, int argc, char **argv);
extern int cmd_rcdup(SESSION handle, int argc, char **argv);
extern int cmd_rmkdir(SESSION handle, int argc, char **argv);

extern int cmd_rrmdir(SESSION handle, int argc, char **argv);
extern int cmd_runlink(SESSION handle, int argc, char **argv);
extern int cmd_rchmod(SESSION handle, int argc, char **argv);
extern int cmd_rrename(SESSION handle, int argc, char **argv);
extern int cmd_rsize(SESSION handle, int argc, char **argv);
extern int cmd_rtime(SESSION handle, int argc, char **argv);
extern int cmd_ridle(SESSION handle, int argc, char **argv);
extern int cmd_rnoop(SESSION handle, int argc, char **argv);
extern int cmd_rhelp(SESSION handle, int argc, char **argv);
extern int cmd_rls(SESSION handle, int argc, char **argv);
extern int cmd_rget(SESSION handle, int argc, char **argv);
extern int cmd_rput(SESSION handle, int argc, char **argv);

#endif /* __APPS_EXAMPLES_FTPC_FTPC_H */
