/****************************************************************************
 * apps/wireless/wapi/examples/ifadd.c
 *
 *  Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of  source code must  retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/wireless/wapi.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int parse_wapi_mode(FAR const char *s, FAR wapi_mode_t * mode)
{
  int ret = 0;

  if (!strcmp(s, "auto"))
    {
      *mode = WAPI_MODE_AUTO;
  else if (!strcmp(s, "adhoc"))
    {
      *mode = WAPI_MODE_ADHOC;
    }
  else if (!strcmp(s, "managed"))
    {
      *mode = WAPI_MODE_MANAGED;
    }
  else if (!strcmp(s, "master"))
    {
      *mode = WAPI_MODE_MASTER;
    }
  else if (!strcmp(s, "repeat"))
    {
      *mode = WAPI_MODE_REPEAT;
    }
  else if (!strcmp(s, "second"))
    {
      *mode = WAPI_MODE_SECOND;
    }
  else if (!strcmp(s, "monitor"))
    {
      *mode = WAPI_MODE_MONITOR;
    }
  else
    {
      ret = 1;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int ifadd_main(int argc, char *argv[])
#endif
{
  wapi_mode_t mode;
  FAR const char *ifname;
  FAR const char *name;
  int ret;

  if (argc != 4)
    {
      fprintf(stderr, "Usage: %s <IFNAME> <NAME> <MODE>\n", argv[0]);
      return EXIT_FAILURE;
    }

  ifname = argv[1];
  name   = argv[2];

  if (parse_wapi_mode(argv[3], &mode))
    {
      fprintf(stderr, "Unknown mode: %s!\n", argv[3]);
      return EXIT_FAILURE;
    }

  ret = wapi_if_add(-1, ifname, name, mode);
  fprintf(stderr, "wapi_if_add(): ret: %d\n", ret);

  return ret;
}
