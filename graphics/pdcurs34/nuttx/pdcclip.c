/****************************************************************************
 * apps/graphics/nuttx/pdclip.c
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
