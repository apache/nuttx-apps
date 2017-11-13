/****************************************************************************
 * apps/nshlib/nsh_mmcmds.c
 *
 *   Copyright (C) 2011-2012, 2017 Gregory Nutt. All rights reserved.
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

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef NEED_CATLINE2
#ifndef CONFIG_NSH_DISABLE_FREE
#  if defined(HAVE_PROC_KMEM) && (defined(HAVE_PROC_UMEM) || \
      defined(HAVE_PROC_PROGMEM))
#    define NEED_CATLINE2 1
#  elif defined(HAVE_PROC_UMEM) && defined(HAVE_PROC_PROGMEM)
#    define NEED_CATLINE2 1
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cat_free
 ****************************************************************************/

#ifdef NEED_CATLINE2
static void nsh_catline2(FAR struct nsh_vtbl_s *vtbl, FAR const char *filepath)
{
  int fd;

  /* Open the file for reading */

  fd = open(filepath, O_RDONLY);
  if (fd >= 0)
    {
      /* Skip the first line which contains the header */

      if (fgets(vtbl->iobuffer, IOBUFFERSIZE, INSTREAM(pstate)) != NULL)
        {
          /* The second line contains the info to send to stdout */

          if (fgets(vtbl->iobuffer, IOBUFFERSIZE, INSTREAM(pstate)) != NULL)
            {
               size_t linesize = strnlen(vtbl->iobuffer, IOBUFFERSIZE);
               (void)nsh_write(vtbl, vtbl->iobuffer, linesize);
            }
        }

      (void)close(fd);
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_free
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_FREE
int cmd_free(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
#if defined(HAVE_PROC_KMEM)
   (void)nsh_catfile(vtbl, argv[0], CONFIG_NSH_PROC_MOUNTPOINT "/kmm");
#  ifdef HAVE_PROC_UMEM
   nsh_catline2(vtbl, CONFIG_NSH_PROC_MOUNTPOINT "/umm");
#  endif
#  ifdef HAVE_PROC_PROGMEM
   nsh_catline2(vtbl, CONFIG_NSH_PROC_MOUNTPOINT "/progmem");
#  endif
   return OK;

#elif defined(HAVE_PROC_UMEM)
   (void)nsh_catfile(vtbl, argv[0], CONFIG_NSH_PROC_MOUNTPOINT "/umm");
#  ifdef HAVE_PROC_PROGMEM
   nsh_catline2(vtbl, CONFIG_NSH_PROC_MOUNTPOINT "/progmem");
#  endif
   return OK;

#else
#  ifdef HAVE_PROC_PROGMEM
   return nsh_catfile(vtbl, argv[0], CONFIG_NSH_PROC_MOUNTPOINT "/progmem");
#  else
   return ERROR;
#  endif
#endif
}
#endif /* !CONFIG_NSH_DISABLE_FREE */
