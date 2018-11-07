/****************************************************************************
 * apps/system/hexed/src/bfile.c
 * Buffered file control
 *
 *   Copyright (c) 2011, B.ZaaR, All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bfile.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Allocate file buffer */

static FAR void *bfallocbuf(FAR struct bfile_s *bf, long sz)
{
  FAR char *buf;

  if (bf == NULL)
    {
      return NULL;
    }

  /* Allocate buffer */

  sz = BFILE_BUF_ALIGN(sz);
  if ((buf = realloc(bf->buf, sz)) == NULL)
    {
      return NULL;
    }

  bf->buf = buf;

  /* Clear new memory */

  if (sz > bf->bufsz)
    {
      memset(bf->buf + bf->bufsz, 0, sz - bf->bufsz);
    }

  bf->bufsz = sz;
  return bf->buf;
}

/* Free a buffered file */

static int bffree(FAR struct bfile_s *bf)
{
  if (bf == NULL)
    {
      return -EBADF;
    }

  /* Free buffer */

  if (bf->buf != NULL)
    {
      free(bf->buf);
    }

  /* Free file name */

  if (bf->name != NULL)
    {
      free(bf->name);
    }

  free(bf);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Get file size */

long fsize(FILE * fp)
{
  long off, sz;

  if (fp == NULL)
    {
      return 0;
    }

  off = ftell(fp);
  fseek(fp, 0, SEEK_END);

  sz = ftell(fp);
  fseek(fp, off, SEEK_SET);

  return sz;
}

long bftruncate(FAR struct bfile_s *bf, long sz)
{
  if (bf == NULL)
    {
      return -EBADF;
    }

  /* Reopen file with truncate */

  bf->fp = freopen(bf->name, "w+", bf->fp);
  return bfflush(bf);
}

/* Flush buffer data to the file */

int bfflush(FAR struct bfile_s *bf)
{
  ssize_t nread;
  int ret = OK;

  if (bf == NULL)
    {
      return -EBADF;
    }

  /* Check for file changes */

  if (!(bf->flags & BFILE_FL_DIRTY))
    {
      return 0;
    }

  /* Write file */

  fseek(bf->fp, 0, SEEK_SET);
  nread = fwrite(bf->buf, 1, bf->size, bf->fp);
  if (nread < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Write to file failed: %d\n", errcode);
      ret = -errcode;
    }
  else if (fwrite(bf->buf, 1, bf->size, bf->fp) == bf->size)
    {
      fprintf(stderr, "ERROR: Bad write size\n");
      ret = -EIO;
    }
  else
    {
      bf->flags &= ~BFILE_FL_DIRTY;
      ret = OK;
    }

  fflush(bf->fp);
  return ret;
}

/* Opens a Buffered File */

FAR struct bfile_s *bfopen(char *name, char *mode)
{
  FAR struct bfile_s *bf;

  /* NULL file name */

  if (name == NULL)
    {
      return NULL;
    }

  /* Allocate a buffered file structure */

  if ((bf = malloc(sizeof(struct bfile_s))) == NULL)
    {
      return NULL;
    }

  memset(bf, 0, sizeof(struct bfile_s));

  /* Set file name */

  if ((bf->name = malloc(strlen(name) + 1)) == NULL)
    {
      bffree(bf);
      return NULL;
    }

  strcpy(bf->name, name);

  /* Open file */

  if ((bf->fp = fopen(bf->name, mode)) == NULL)
    {
      bffree(bf);
      return NULL;
    }

  /* Set file buffer */

  bf->buf = NULL;
  bf->size = fsize(bf->fp);
  if (bfallocbuf(bf, bf->size) == NULL)
    {
      bfclose(bf);
      return NULL;
    }

  return bf;
}

/* Closes a Buffered File */

int bfclose(FAR struct bfile_s *bf)
{
  int r;

  if (bf == NULL)
    {
      return -EBADF;
    }

  bfflush(bf);

  /* Close file */

  r = fclose(bf->fp);
  bffree(bf);
  return r;
}

/* Remove bytes from the Buffered File
 *
 * Moves the data from the end of the buffer, then shrinks the file
 * size.
 */

long bfclip(FAR struct bfile_s *bf, long off, long sz)
{
  long cnt;

  if (bf == NULL)
    {
      return EOF;
    }

  /* Negative error */

  if (off < 0 || sz <= 0)
    {
      return EOF;
    }

  /* Offset past EOF */

  if (off > bf->size)
    {
      return EOF;
    }

  /* Size past EOF */

  if ((off + sz) > bf->size)
    {
      sz = bf->size - off;
    }

  /* Remove from file */

  cnt = bf->size - (off + sz);
  memmove(bf->buf + off, bf->buf + off + sz, cnt);

  bf->size  -= sz;
  bf->flags |= BFILE_FL_DIRTY;

  memset(bf->buf + bf->size, 0, sz);
  return cnt;
}

/* Insert bytes into the Buffered File
 *
 * Increases the file size, moves the current data and inserts the
 * new data.
 */

long bfinsert(FAR struct bfile_s *bf, long off, void *mem, long sz)
{
  long cnt;

  if (bf == NULL)
    {
      return EOF;
    }

  /* Negative error */

  if (off < 0 || sz <= 0)
    {
      return EOF;
    }

  /* Increase buffer size */

  if (bf->bufsz < (bf->size + off + sz))
    {
      if (bfallocbuf(bf, bf->size + off + sz) == NULL)
        {
          return EOF;
        }
    }

  /* Move data */

  if (off < bf->size)
    {
      cnt = bf->size - off;
      memmove(bf->buf + off + sz, bf->buf + off, cnt);
    }

  memmove(bf->buf + off, mem, sz);

  /* Increase file size */

  if (off > bf->size)
    {
      bf->size += off - bf->size;
    }

  bf->size += sz;
  bf->flags |= BFILE_FL_DIRTY;
  return sz;
}

/* Copies bytes from src to off */

long bfcopy(FAR struct bfile_s *bf, long off, long src, long sz)
{
  if (bf == NULL)
    {
      return EOF;
    }

  /* Error: Source past EOF */

  if (src > bf->size)
    {
      return EOF;
    }

  /* Adjust sz to EOF */

  if ((src > off) && (src + sz > bf->size))
    {
      sz = bf->size - src;
    }

  /* Adjust source/length for insert */

  if (src >= off)
    {
      if (src + sz > bf->size)
        {
          sz = bf->size - src;
        }

      src += sz;
    }

  return bfinsert(bf, off, bf->buf + src, sz);
}

/* Moves bytes from src to off
 *
 * Moves the data from src to off then removes the data from the original src
 * Moves data in chunks to save memory allocation
 */

long bfmove(FAR struct bfile_s *bf, long off, long src, long sz)
{
  long adj;
  long cnt;
  long len;

  if (bf == NULL)
    {
      return EOF;
    }

  /* Error: source past EOF */

  if (src > bf->size)
    {
      return EOF;
    }

  /* Edjust sz to EOF */

  if ((src > off) && (src + sz > bf->size))
    {
      sz = bf->size - src;
    }

  /* Adjust source/offset */

  len = BFILE_BUF_MIN > sz ? sz : BFILE_BUF_MIN;
  if (src > off)
    {
      src += len;
      adj = len;
    }
  else
    {
      off += sz;
      adj = 0;
    }

  /* Move data in chunks */

  for (cnt = sz; cnt; cnt -= len)
    {
      /* End of count? */

      if (cnt < len)
        {
          len = cnt;
        }

      /* Move data */

      bfinsert(bf, off, bf->buf + src, len);
      bfclip(bf, src, len);
      off += adj;
    }

  return sz;
}

/* Read Buffered File
 *
 * Reads an entire file to the buffer.
 */

long bfread(FAR struct bfile_s *bf)
{
  long r;

  if (bf == NULL)
    {
      return EOF;
    }

  /* Check buffer size */

  bf->size = fsize(bf->fp);
  if (bf->size > bf->bufsz)
    {
      if (bfallocbuf(bf, bf->size) == NULL)
        {
          return EOF;
        }
    }

  /* Read whole file into the buffer */

  if ((r = fread(bf->buf, 1, bf->size, bf->fp)) == bf->size)
    {
      bf->flags &= ~BFILE_FL_DIRTY;
    }

  return r;
}

/* Write Buffered File
 *
 * Writes data to the buffer, the buffer still needs to be flushed
 * to the file before closing or reading.
 */

long bfwrite(FAR struct bfile_s *bf, long off, void *mem, long sz)
{
  if (bf == NULL)
    {
      return EOF;
    }

  /* Increase buffer size */

  if (bf->bufsz < off + sz)
    {
      if (bfallocbuf(bf, off + sz) == NULL)
        {
          return EOF;
        }
    }

  memmove(bf->buf + off, mem, sz);

  /* Increase file size */

  if (bf->size < off + sz)
    {
      bf->size = off + sz;
    }

  bf->flags |= BFILE_FL_DIRTY;
  return sz;
}
