/****************************************************************************
 * apps/system/hexed/inlcude/bfile.h
 * Buffered file control header
 *
 *   Copyright (c) 2010, B.ZaaR, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   The names of contributors may not be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_SYSTEM_HEXED_INCLUDE_BFILE_H
#define __APPS_SYSTEM_HEXED_INCLUDE_BFILE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Buffer size/alignment */

#define BFILE_BUF_MIN  0x1000
#define BFILE_BUF_ALIGN(sz) \
  ((sz + BFILE_BUF_MIN - 1) / BFILE_BUF_MIN * BFILE_BUF_MIN)

/* Buffered File flags */

#define BFILE_FL_DIRTY 0x00000001

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Buffered File */

struct bfile_s
{
  FAR FILE *fp;
  FAR char *name;
  long size;
  int flags;
  long bufsz;
  FAR char *buf;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

long   fsize(FAR FILE *fp);
int    bfflush(FAR struct bfile_s *bf);
struct bfile_s *bfopen(FAR char *name, FAR char *mode);
int    bfclose(FAR struct bfile_s *bf);
long   bfclip(FAR struct bfile_s *bf, long off, long sz);
long   bfcopy(FAR struct bfile_s *bf, long dest, long src, long sz);
long   bfinsert(FAR struct bfile_s *bf, long off, FAR void *mem, long sz);
long   bfmove(FAR struct bfile_s *bf, long dest, long src, long sz);
long   bfread(FAR struct bfile_s *bf);
long   bftruncate(FAR struct bfile_s *bf, long sz);
long   bfwrite(FAR struct bfile_s *bf, long off, FAR void *mem, long sz);

#endif /* __APPS_SYSTEM_HEXED_INCLUDE_BFILE_H */
