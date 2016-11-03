/****************************************************************************
 * apps/modbus/nuttx/portother_m.c
 *
 *   FreeModbus Library: NuttX Port
 *   Copyright (c) 2006 Christian Walter <wolti@sil.at>
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "port.h"

#include "modbus/mb.h"
#include "modbus/mb_m.h"
#include "modbus/mbport.h"

#if defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NELEMS(x) (sizeof((x))/sizeof((x)[0]))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FILE *fLogFile = NULL;
static eMBPortLogLevel eLevelMax = MB_LOG_DEBUG;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void vMBMasterPortLogLevel(eMBPortLogLevel eNewLevelMax)
{
  eLevelMax = eNewLevelMax;
}

void vMBMasterPortLogFile(FILE * fNewLogFile)
{
  fLogFile = fNewLogFile;
}

void vMBMasterPortLog(eMBPortLogLevel eLevel, const char * szModule,
                      const char * szFmt, ...)
{
  char     szBuf[512];
  int      i;
  va_list  args;
  FILE    *fOutput = fLogFile == NULL ? stderr : fLogFile;

  static const char *arszLevel2Str[] = { "ERROR", "WARN", "INFO", "DEBUG" };

  i = snprintf(szBuf, NELEMS(szBuf),
               "%s: %s: ", arszLevel2Str[eLevel], szModule);

  if (i != 0)
    {
      va_start(args, szFmt);
      i += vsnprintf(&szBuf[i], NELEMS(szBuf) - i, szFmt, args);
      va_end(args);
    }

  if (i != 0)
    {
      if (eLevel <= eLevelMax)
        {
          fputs(szBuf, fOutput);
        }
    }
}

#endif /* defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER) */
