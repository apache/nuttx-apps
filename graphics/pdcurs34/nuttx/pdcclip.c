/****************************************************************************
 * apps/graphics/nuttx/pdclip.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#include "pdcnuttx.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: PDC_getclipboard
 *
 * Description:
 *   PDC_getclipboard() gets the textual contents of the system's clipboard.
 *   This function returns the contents of the clipboard in the contents
 *   argument. It is the responsibilitiy of the caller to free the memory
 *   returned, via PDC_freeclipboard().  The length of the clipboard
 *   contents is returned in the length argument.
 *
 * Returned Value:
 *   PDC_CLIP_SUCCESS        The call was successful
 *   PDC_CLIP_MEMORY_ERROR   unable to allocate sufficient memory for
 *                           the clipboard contents
 *   PDC_CLIP_EMPTY          The clipboard contains no text
 *   PDC_CLIP_ACCESS_ERROR   No clipboard support
 *
 ****************************************************************************/

int PDC_getclipboard(char **contents, long *length)
{
  PDC_LOG(("PDC_getclipboard() - called\n"));
  return PDC_CLIP_ACCESS_ERROR;
}

/****************************************************************************
 * Name: PDC_setclipboard
 *
 * Description:
 *   PDC_setclipboard copies the supplied text into the system's clipboard,
 *   emptying the clipboard prior to the copy.
 *
 * Returned Value:
 *   PDC_CLIP_SUCCESS        The call was successful
 *   PDC_CLIP_MEMORY_ERROR   unable to allocate sufficient memory for
 *                           the clipboard contents
 *   PDC_CLIP_EMPTY          The clipboard contains no text
 *   PDC_CLIP_ACCESS_ERROR   No clipboard support
 *
 ****************************************************************************/

int PDC_setclipboard(const char *contents, long length)
{
  PDC_LOG(("PDC_setclipboard() - called\n"));
  return PDC_CLIP_ACCESS_ERROR;
}

/****************************************************************************
 * Name: PDC_freeclipboard
 *
 * Description:
 *   Free memory obtained with PDC_getclipboard().
 *
 * Returned Value:
 *   PDC_CLIP_SUCCESS        The call was successful
 *   PDC_CLIP_MEMORY_ERROR   unable to allocate sufficient memory for
 *                           the clipboard contents
 *   PDC_CLIP_EMPTY          The clipboard contains no text
 *   PDC_CLIP_ACCESS_ERROR   No clipboard support
 *
 ****************************************************************************/

int PDC_freeclipboard(char *contents)
{
  PDC_LOG(("PDC_freeclipboard() - called\n"));
  return PDC_CLIP_ACCESS_ERROR;
}

/****************************************************************************
 * Name: PDC_clearclipboard
 *
 * Description:
 *   PDC_clearclipboard() clears the internal clipboard.
 *
 * Returned Value:
 *   PDC_CLIP_SUCCESS        The call was successful
 *   PDC_CLIP_MEMORY_ERROR   unable to allocate sufficient memory for
 *                           the clipboard contents
 *   PDC_CLIP_EMPTY          The clipboard contains no text
 *   PDC_CLIP_ACCESS_ERROR   No clipboard support
 *
 ****************************************************************************/

int PDC_clearclipboard(void)
{
  PDC_LOG(("PDC_clearclipboard() - called\n"));
  return PDC_CLIP_ACCESS_ERROR;
}
