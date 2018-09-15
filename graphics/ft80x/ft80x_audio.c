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
#include <sys/ioctl.h>
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

/* These configuration settings define the graphics memory region that we
 * will use for audio buffering.
 */

#define AUDIO_BUFOFFSET    CONFIG_GRAPHICS_FT80X_AUDIO_BUFOFFSET
#define AUDIO_BUFSIZE      CONFIG_GRAPHICS_FT80X_AUDIO_BUFSIZE
#define AUDIO_BUFEND       (AUDIO_BUFOFFSET + AUDIO_BUFSIZE)

#if AUDIO_BUFEND > FT80X_RAM_G_SIZE
#  error "Audio buffer extends beyond RAM G"
#endif

#define RAMG_STARTADDR     (FT80X_RAM_G + AUDIO_BUFOFFSET)
#define RAMG_ENDADDR       (FT80X_RAM_G + AUDIO_BUFEND)

/* The display list buffer will be re-purposed as an I/O buffer for the
 * transfer of audio data to RAM G.
 */

#if FT80X_DL_BUFSIZE > AUDIO_BUFSIZE
#  define MAX_DLBUFFER     AUDIO_BUFSIZE
#else
#  define MAX_DLBUFFER     FT80X_DL_BUFSIZE
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_audio_enable
 *
 * Description:
 *   Play an short sound effect.  If there is a audio amplifier on board
 *   (such as TPA6205A or LM4864), then there may also be an active low
 *   audio shutdown output.  That output is controlled by this interface.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the
 *            caller with write access.
 *   enable - True: Enabled the audio amplifier; false: disable
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_audio_enable(int fd, bool enable)
{
#ifndef CONFIG_LCD_FT80X_AUDIO_NOSHUTDOWN
  int ret;

  ret = ioctl(fd, FT80X_IOC_AUDIO, (unsigned long)enable);
  if (ret < 0)
    {
      ret = -errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_AUDIO) failed: %d\n", ret);
    }

  return ret;

#else
  return OK;
#endif
}

/****************************************************************************
 * Name: ft80x_audio_playsound
 *
 * Description:
 *   Play an short sound effect.
 *
 *   NOTE:  It may be necessary to enable the audio amplifier with
 *   ft80x_audio_enable() prior to calling this function.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the
 *            caller with write access.
 *   effect - The sound effect to use (see FT80X_EFFECT_* definitions).
 *   pitch  - Pitch associated with the sound effect (see FT80X_NOTE_*
 *            definitions).  May be zero if there is no pitch associated
 *            with the effect.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_audio_playsound(int fd, uint16_t effect, uint16_t pitch)
{
  uint32_t cmds[2];

  cmds[0] = effect | pitch;
  cmds[1] = 1;

  return ft80x_putregs(fd, FT80X_REG_SOUND, 2, cmds);
}

/****************************************************************************
 * Name: ft80x_audio_playfile
 *
 * Description:
 *   Play an audio file.  Audio files must consist of raw sample data.
 *
 *   NOTE:  It may be necessary to enable the audio amplifier with
 *   ft80x_audio_enable() prior to calling this function.
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

      /* Check size of the buffer we can fit into RAM G */

      do
        {
          /* Get the current playback position */

          readptr = 0;
          ret = ft80x_getreg32(fd, FT80X_REG_PLAYBACK_READPTR, &readptr);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* Check if the readptr is before the current offset.  In this
           * case, we can use all of the RAM G from the current offset to
           * the end of the buffer.
           *
           *   Case 1:  readptr <= offset
           *
           *     +--------+-------+-----+
           *     |        |       |xxxxx|
           *     +--------+-------+-----+
           *     |        |       |     ^ end buffer
           *     |        |       ^ offset
           *     |        ^ readptr
           *     ^ start buffer
           *
           * NOTE: There is a race condition here:  The readptr could
           * overtake the offset before we can perform the write.
           */

          if (readptr <= offset)
            {
              freespace = AUDIO_BUFSIZE - offset;
              break;
            }

          /* No, the readptr is after the offset.  In this case we can use
           * all of the RAM G from the current offset to the readptr.
           *
           *   Case 2:  readptr > offset
           *
           *     +--------+-------+-----+
           *     |        |xxxxxxx|     |
           *     +--------+-------+-----+
           *     |        |       |     ^ end buffer
           *     |        |       ^ readptr
           *     |        ^ offset
           *     ^ start buffer
           */

          else
            {
              freespace = readptr - offset;
            }

          /* Terminate the poll when all the memory to end of the RAM G
           * buffer is available (see the 'break' above), when an optimally
           * sized space is available (MAX_DLBUFFER), there is space to
           * write all of the 'remaining' file data, or all of the memory is
           * free up to the end of the RAM G buffer (actually already
           * handled by the above 'break')
           */
       }
      while (freespace < MAX_DLBUFFER &&
             freespace < remaining &&
             freespace < (AUDIO_BUFSIZE - offset));

      /* Clip to the amount that will fit at the tail of the RAM G buffer */

      if (readlen > freespace)
        {
          readlen = freespace;
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

      /* Update pointers and counts */

      offset    += nread;
      remaining -= nread;

      /* Wrap the offset back to the beginning of the buffer if necessary */

      if (offset >= AUDIO_BUFSIZE)
        {
          offset = 0;
        }

      /* If we have started playing the audio file in RAGM_G.  NOTE that
       * there is a race condition here.  If the chunk size is small or if
       * the data transfer rate is low, then there may be data under-run
       * which will disrupt the continuity of the audio.
       */

      if (!started)
        {
          /* Start playing at the beginning of graphics memory */
          /* Set the audio playback start address */

          ret = ft80x_putreg32(fd, FT80X_REG_PLAYBACK_START,
                               RAMG_STARTADDR);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_putreg32 failed: %d\n", ret);
              goto errout_with_fd;
            }

          /* Set the length of the audio buffer */

          ret = ft80x_putreg32(fd, FT80X_REG_PLAYBACK_LENGTH,
                               AUDIO_BUFSIZE);
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
   }

  /* Transfer is complete.  'offset' points to the end of the file in RAM G.
   * Clear all of the RAM G at the end of the file so that audio is muted
   * when the end of file is encountered.
   *
   *   Case 1:  readptr <= offset
   *
   *     +--------+-------+-----+
   *     |00000000|       |00000|
   *     +--------+-------+-----+
   *     |        |       |     ^ end buffer
   *     |        |       ^ offset
   *     |        ^ readptr
   *     ^ start buffer
   *
   *   Case 2:  readptr > offset
   *
   *     +--------+-------+-----+
   *     |        |0000000|     |
   *     +--------+-------+-----+
   *     |        |       |     ^ end buffer
   *     |        |       ^ readptr
   *     |        ^ offset
   *     ^ start buffer
   */

  memset.cmd       = FT80X_CMD_MEMSET;
  memset.value     = 0;

  readptr = 0;
  ret = ft80x_getreg32(fd, FT80X_REG_PLAYBACK_READPTR, &readptr);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_getreg32 failed: %d\n", ret);
      goto errout_with_fd;
    }

  if (readptr <= offset)
    {
      if (offset < AUDIO_BUFSIZE)
        {
          memset.ptr  = RAMG_STARTADDR + offset;
          memset.num  = AUDIO_BUFSIZE - offset;

          ret = ft80x_coproc_send(fd, (FAR const uint32_t *)&memset, 4);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_coproc_send failed: %d\n", ret);
              goto errout_with_fd;
            }
        }

      if (readptr > 0)
        {
          memset.ptr  = RAMG_STARTADDR;
          memset.num  = readptr;

          ret = ft80x_coproc_send(fd, (FAR const uint32_t *)&memset, 4);
          if (ret < 0)
            {
              ft80x_err("ERROR: ft80x_coproc_send failed: %d\n", ret);
              goto errout_with_fd;
            }
        }
    }
  else /* if (readptr > offset) */
    {
      memset.ptr  = RAMG_STARTADDR + offset;
      memset.num  = readptr - offset;

      ret = ft80x_coproc_send(fd, (FAR const uint32_t *)&memset, 4);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_coproc_send failed: %d\n", ret);
          goto errout_with_fd;
        }

    }

  /* Wait until the read pointer wraps back to the beginning of the buffer */

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

  /* Wait until the read pointer pass through write pointer into the zeroed
   * area.
   *
   * REVISIT:  What if the offset is at the very end of the buffer?
   */

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
