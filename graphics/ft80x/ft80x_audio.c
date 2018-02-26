/****************************************************************************
 * apps/graphics/ft80x/ft80x_audio.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* We will use graphics memory starting at offset zero and through the
 * following offset.
 *
 * REVISIT:  Should these not be input parameters?
 */

#define RAMG_MAXOFFSET   (64 * 1024)
#define RAMG_MAXMASK     (RAMG_MAXOFFSET - 1)

#if FT80X_DL_BUFSIZE > RAMG_MAXOFFSET
#  define MAX_DLBUFFER    RAMG_MAXOFFSET
#else
#  define MAX_DLBUFFER    FT80X_DL_BUFSIZE
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_audio_playfile
 *
 * Description:
 *   Play an audio file
 *
 * Input Parameters:
 *   fd        - The file descriptor of the FT80x device.  Opened by the
 *               caller with write access.
 *   buffer    - An instance of struct ft80x_dlbuffer_s allocated by the
 *               caller.
 *   filepath  - Absolute path to the audio file
 *   format    - Audio format.  One of:
 *
 *               AUDIO_FORMAT_LINEAR  Linear Sample format
 *               AUDIO_FORMAT_ULAW    uLaw Sample format
 *               AUDIO_FORMAT_ADPCM   4-bit IMA ADPCM Sample format
 *
 *   frequency - Audio sample frequency (<65,536)
 *   volume    - Playback volume (0=mute; 255=max)
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

#if CONFIG_NFILE_DESCRIPTORS > 3
int ft80x_audio_playfile(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                         FAR const char *filepath, uint8_t format,
                         uint16_t frequency, uint8_t volume)
{
  struct ft80x_cmd_memset_s memset;
  struct stat buf;
  size_t freespace;
  size_t remaining;
  size_t readlen;
  ssize_t nread;
  uint32_t offset;
  uint32_t readptr;
  bool started;
  int audiofd;
  int ret;

  DEBUGASSERT(filepath != NULL);

  /* Open the audio file */

  audiofd = open(filepath, O_RDONLY);
  if (audiofd < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: open of %s failed: %d\n", filepath, errcode);
      return -errcode;
    }

  /* Get the size of the file */

  ret = fstat(audiofd, &buf);
  if (ret < 0)
    {
      ft80x_err("ERROR: fstat of %s failed: %d\n", filepath, ret);
      goto errout_with_fd;
    }

  ft80x_info("File: %s\n", filepath);
  ft80x_info("Size: %lu\n", (unsigned long)buf.st_size);

  /* Loop until the whole audio file has been sent */

  remaining = buf.st_size;
  offset    = 0;
  started   = false;

  while (remaining > 0)
    {
      /* How much can we read on this pass? */

      readlen = remaining;

      /* Clip to the amount that will fit into the display buffer (which we
       * are re-purposing for audio data buffer for now)
       */

      if (readlen > MAX_DLBUFFER)
        {
          readlen = MAX_DLBUFFER;
        }

      /* Read the data into the available display list buffer */

      nread = read(audiofd, buffer->dlbuffer, readlen);
      if (nread < 0)
        {
          ret = -errno;
          ft80x_err("ERROR: read from %s failed: %d\n", filepath, ret);
          goto errout_with_fd;
        }

      /* Since we know the size of the file, we don't expect EOF */

      DEBUGASSERT(nread > 0);

      /* Copy the data read into the graphics memory */

      ret = ft80x_ramg_write(fd, offset, buffer->dlbuffer, nread);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_ramg_write failed: %d\n", ret);
          goto errout_with_fd;
        }

      offset    += nread;
      remaining -= nread;

      /* If we have started playing the audio file in RAGM_G.  NOTE that
       * there is a race condition here.  If the chunk size is small or if
       * the data transfer rate is low, then there may be data under-run
       * which will disrupt the continuity of the audio.
       */

      if (!started)
        {
          /* Start playing at the beginning of graphics memory */
          /* Set the audio playback start address */

          ret = ft80x_putreg32(fd, FT80X_REG_PLAYBACK_START, FT80X_RAM_G);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg32 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* Set the length of the audio buffer */

          ret = ft80x_putreg32(fd, FT80X_REG_PLAYBACK_LENGTH,
                               RAMG_MAXOFFSET);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg32 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* Set the sample frequency */

          ret = ft80x_putreg16(fd, FT80X_REG_PLAYBACK_FREQ, frequency);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg16 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* Set the audio format */

          ret = ft80x_putreg8(fd, FT80X_REG_PLAYBACK_FORMAT, format);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* Loop when we get to the end of the file */

          ret = ft80x_putreg8(fd, FT80X_REG_PLAYBACK_LOOP, 1);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* Set the volume */

          ret = ft80x_putreg8(fd, FT80X_REG_VOL_PB, volume);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* And start the playback */

          ret = ft80x_putreg8(fd, FT80X_REG_PLAYBACK_PLAY, 1);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
              goto errout_with_fd;
            }

          started = true;
        }

      /* Check the freespace if we can fit bull buffer (or the remaining
       * buffer) into RAM_G.
       */

      do
        {
          uint32_t inuse;

          /* Get the current playback position */

          readptr = 0;
          ret = ft80x_getreg32(fd, FT80X_REG_PLAYBACK_READPTR, &readptr);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
              goto errout_with_fd;
            }

          inuse     = (offset - readptr) & RAMG_MAXMASK;
          freespace = RAMG_MAXOFFSET - inuse;
       }
      while (freespace < MAX_DLBUFFER || freespace < remaining);
   }

  /* Mute the sound clearing buffer.
   * REVISIT:  There is an ugly hack into the display list logic in the
   * following 8(
   */

  memset.cmd       = FT80X_CMD_MEMSET;
  memset.ptr       = FT80X_RAM_G + offset;
  memset.value     = 0;
  memset.num       = RAMG_MAXOFFSET - offset;

  buffer->coproc   = true;
  buffer->dlsize   = 0;
  buffer->dloffset = sizeof(struct ft80x_cmd_memset_s);

  ret = ft80x_ramcmd_append(fd, buffer, &memset,
                            sizeof(ft80x_ramcmd_append));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_ramcmd_append failed: %d\n", ret);
      goto errout_with_fd;
    }

  ret = ft80x_ramcmd_waitfifoempty(fd);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_ramcmd_waitfifoempty failed: %d\n", ret);
      goto errout_with_fd;
    }

  /* If the read pointer is already passed over write pointer */

  do
    {
      readptr = 0;
      ret = ft80x_getreg32(fd, FT80X_REG_PLAYBACK_READPTR, &readptr);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
          goto errout_with_fd;
        }
    }
  while (readptr > offset);

  /* Wait till read pointer pass through write pointer */

  do
    {
      readptr = 0;
      ret = ft80x_getreg32(fd, FT80X_REG_PLAYBACK_READPTR, &readptr);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
          goto errout_with_fd;
        }
    }
  while (readptr < offset);

  /* The file is done... */
  /* Stop looping */

  ret = ft80x_putreg8(fd, FT80X_REG_PLAYBACK_LOOP, 1);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
      goto errout_with_fd;
    }

  /* Set the playback length to zero */

  ret = ft80x_putreg32(fd, FT80X_REG_PLAYBACK_LENGTH, 0);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg32 failed: %d\n", ret);
      goto errout_with_fd;
    }

  /* Set the volume to zero */

  ret = ft80x_putreg8(fd, FT80X_REG_VOL_PB, volume);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
      goto errout_with_fd;
    }

  /* And stop the playback */

  ret = ft80x_putreg8(fd, FT80X_REG_PLAYBACK_PLAY, 1);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
    }

errout_with_fd:
  close(audiofd);
  return ret;
}

#endif /* CONFIG_NFILE_DESCRIPTORS > 3 */
