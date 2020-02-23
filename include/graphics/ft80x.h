/****************************************************************************
 * apps/include/graphics/ft80x.h
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

#ifndef __APPS_INCLUDE_GRAPHICS_FT80X_H
#define __APPS_INCLUDE_GRAPHICS_FT80X_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

#ifdef CONFIG_GRAPHICS_FT80X

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Buffer size in units of words (truncating any unaligned bytes) */

#define FT80X_DL_BUFSIZE  (CONFIG_GRAPHICS_FT80X_BUFSIZE & ~3)
#define FT80X_DL_BUFWORDS (CONFIG_GRAPHICS_FT80X_BUFSIZE >> 2)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure defines the local display list buffer */

struct ft80x_dlbuffer_s
{
  bool coproc;       /* True: Use co-processor FIFO; false: Use DL memory */
  uint16_t dlsize;   /* Total sizeof the display list written to hardware */
  uint16_t dloffset; /* The number display list bytes buffered locally */
  uint32_t dlbuffer[FT80X_DL_BUFWORDS];
};

/* Describes touch sample */

union ft80x_touchpos_u
{
  uint32_t xy;       /* To force 32-bit alignment */
  struct             /* Little-endian */
  {
    int16_t x;       /* Touch X position (-32768 if no touch) */
    int16_t y;       /* Touch Y position (-32768 if no touch) */
  } u;
};

struct ft80x_touchinfo_s
{
  uint8_t tag;                    /* Touch 0 tag */
#if defined(CONFIG_LCD_FT800)
  int16_t pressure;               /* Touch pressure (32767 if not touched) */
#endif
  union ft80x_touchpos_u tagpos;  /* Position associated with tag */
#if defined(CONFIG_LCD_FT800) || !defined(CONFIG_LCD_FT801_MULTITOUCH)
  union ft80x_touchpos_u pos;     /* Current touch position */
#else
  union ft80x_touchpos_u pos[4];  /* Current touch position for up to 5 touches */
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: ft80x_dl_start
 *
 * Description:
 *   Start a new display list.  This function will:
 *
 *   1) Set the total display list size to zero
 *   2) Set the display list buffer offset to zero
 *   3) Reposition the VFS so that subsequent writes will be to the
 *      beginning of the hardware display list.
 *   4) Write the CMD_DLSTART command into the local display list buffer
 *      (Only for co-processor commands)
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   coproc - True: Use co-processor FIFO; false: Use DL memory.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_start(int fd, FAR struct ft80x_dlbuffer_s *buffer, bool coproc);

/****************************************************************************
 * Name: ft80x_dl_end
 *
 * Description:
 *   Terminate the display list.  This function will:
 *
 *   1) Add the DISPLAY command to the local display list buffer to finish
 *      the last display
 *   2) If using co-processor RAM CMD, add the CMD_SWAP to the DL command
 *      list
 *   3) Flush the local display buffer to hardware and set the display list
 *      buffer offset to zero.
 *   4) Swap to the newly created display list (DL memory case only).
 *   5) For the case of the co-processor RAM CMD, it will also wait for the
 *      FIFO to be emptied.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_end(int fd, FAR struct ft80x_dlbuffer_s *buffer);

/****************************************************************************
 * Name: ft80x_dl_data
 *
 * Description:
 *   Add data to the display list and increment the display list buffer
 *   offset.  If the data will not fit into the local display buffer, then
 *   the local display buffer will first be flushed to hardware in order to
 *   free up space.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   data   - The data to be added to the display list
 *   datlen - The length of the data to be added to the display list.  If
 *            this is not an even multiple of 4 bytes, then the actual length
 *            will be padded with zero bytes to achieve alignment.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_data(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                  FAR const void *data, size_t datlen);

/****************************************************************************
 * Name: ft80x_dl_string
 *
 * Description:
 *   Add the string along with its NUL terminator to the display list and
 *   increment the display list buffer offset.  If the length of the string
 *   with its NUL terminator is not an even multiple of 4 bytes, then the
 *   actual length will be padded with zero bytes to achieve alignment.
 *
 *   If the data will not fit into the local display buffer, then the local
 *   display buffer will first be flushed to hardware in order to free up
 *   space.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   str    - The string to be added to the display list.  If NUL, then a
 *            NUL string will be added to the display list.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_string(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                    FAR const char *str);

/****************************************************************************
 * Name: ft80x_dl_flush
 *
 * Description:
 *   Flush the current contents of the local local display list buffer to
 *   hardware and reset the local display list buffer offset to zero.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller with
 *            write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   wait   - True: wait until data has been consumed by the co-processor
 *            (only for co-processor destination); false:  Send to hardware
 *            and return immediately.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_flush(int fd, FAR struct ft80x_dlbuffer_s *buffer, bool wait);

/****************************************************************************
 * Name: ft80x_dl_create
 *
 * Description:
 *   For simple display lists, this function combines all functionality into
 *   a single combined.  This function does the following:
 *
 *   1) Calls ft80x_dl_dlstart() to initialize the display list.
 *   2) Calls ft80x_dl_data() to transfer the simple display list
 *   3) Calls ft80x_dl_end() to complete the display list
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   buffer - An instance of struct ft80x_dlbuffer_s allocated by the caller.
 *   data   - Pointer to a uint32_t array containing the simple display list
 *   nwords - The number of 32-bit words in the array.
 *   coproc - True: Use co-processor FIFO; false: Use DL memory.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_create(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                    FAR const uint32_t *cmds, unsigned int nwords,
                    bool coproc);

/****************************************************************************
 * Name: ft80x_coproc_send
 *
 * Description:
 *   Send commands to the co-processor via the CMD RAM FIFO.  This function
 *   will not return until the command has been consumed by the co-processor.
 *
 *   NOTE:  This command is not appropriate use while a display is being
 *   formed.  It is will mess up the CMD RAM FIFO offsets managed by the
 *   display list logic.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   cmds  - A list of 32-bit commands to be sent.
 *   ncmds - The number of commands in the list.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_coproc_send(int fd, FAR const uint32_t *cmds, size_t ncmds);

/****************************************************************************
 * Name: ft80x_coproc_waitlogo
 *
 * Description:
 *   Wait for the logo animation to complete.  The logo command causes the
 *   co-processor engine to play back a short animation of the FTDI logo.
 *   During logo playback the MCU should not access any FT800 resources.
 *   After 2.5 seconds have elapsed, the co-processor engine writes zero to
 *   REG_CMD_READ and REG_CMD_WRITE, and starts waiting for commands.  After
 *   this command is complete, the MCU shall write the next command to the
 *   starting address of RAM_CMD.
 *
 * Input Parameters:
 *   fd - The file descriptor of the FT80x device.  Opened by the caller
 *        with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_coproc_waitlogo(int fd);

/****************************************************************************
 * Name: ft80x_ramg_write
 *
 * Description:
 *   Write to graphics memory
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   offset - Offset in graphics memory to write to (dest)
 *   data   - Pointer to a data to be written (src)
 *   nbytes - The number of bytes to write to graphics memory.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramg_write(int fd, unsigned int offset, FAR const void *data,
                     unsigned int nbytes);

/****************************************************************************
 * Name: ft80x_touch_gettransform
 *
 * Description:
 *   Read the touch transform matrix
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   matrix - The location to return the transform matrix
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_gettransform(int fd, FAR uint32_t matrix[6]);

/****************************************************************************
 * Name: ft80x_touch_tag
 *
 * Description:
 *   Read the current touch tag.  The touch tag is an 8-bit value
 *   identifying the specific graphics object on the screen that is being
 *   touched. The value zero indicates that there is no graphic object being
 *   touched.
 *
 *   Only a single touch can be queried.  For the FT801 in "extended",
 *   multi-touch mode, this value indicates only the tag associated with
 *   touch 0.
 *
 * Input Parameters:
 *   fd - The file descriptor of the FT80x device.  Opened by the caller
 *        with write access.
 *
 * Returned Value:
 *   A value of 1-255 is returned if a graphics object is touched.  Zero is
 *   returned if no graphics object is touched.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_tag(int fd);

/****************************************************************************
 * Name: ft80x_touch_waittag
 *
 * Description:
 *   Wait until there is a change in the touch tag.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   oldtag - The previous tag value.  This function will return when the
 *            current touch tag differs from this value.
 *
 * Returned Value:
 *   A value of 1-255 is returned if a graphics object is touched.  Zero is
 *   returned if no graphics object is touched.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_waittag(int fd, uint8_t oldtag);

/****************************************************************************
 * Name: ft80x_touch_info
 *
 * Description:
 *   Return the current touch tag and touch position information.
 *
 *   For the FT801 in "extended", multi-touch mode, the tag value indicates
 *   only the tag associated with touch 0.
 *
 *   Touch positions of -32768 indicate that no touch is detected.
 *
 * Input Parameters:
 *   fd   - The file descriptor of the FT80x device.  Opened by the caller
 *          with write access.
 *   info - Location in which to return the touch information
 *
 * Returned Value:
 *   A value of 1-255 is returned if a graphics object is touched.  Zero is
 *   returned if no graphics object is touched.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_touch_info(int fd, FAR struct ft80x_touchinfo_s *info);

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

int ft80x_audio_enable(int fd, bool enable);

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

int ft80x_audio_playsound(int fd, uint16_t effect, uint16_t pitch);

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
                         uint16_t frequency, uint8_t volume);
#endif

/****************************************************************************
 * Name: ft80x_backlight_set
 *
 * Description:
 *   Set the backlight intensity via the PWM duty.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   duty  - The new backlight duty (as a percentage 0..100)
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_backlight_set(int fd, uint8_t duty);

/****************************************************************************
 * Name: ft80x_backlight_fade
 *
 * Description:
 *   Change the backlight intensity with a controllable fade.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   duty  - The terminal duty (as a percentage 0..100)
 *   delay - The duration of the fade in milliseconds (10..16700).
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_backlight_fade(int fd, uint8_t duty, uint16_t delay);

/****************************************************************************
 * Name: ft80x_gpio_configure
 *
 * Description:
 *   Configure an FT80x GPIO pin
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   gpio  - Identifies the GPIO pin {0,1}
 *   dir   - Direction:  0=input, 1=output
 *   drive - Common output drive strength for GPIO 0 and 1 (see
 *           FT80X_GPIO_DRIVE_* definitions).  Default is 4mA.
 *   value - Initial value for output pins
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_gpio_configure(int fd, uint8_t gpio, uint8_t dir, uint8_t drive,
                         bool value);

/****************************************************************************
 * Name: ft80x_gpio_write
 *
 * Description:
 *   Write a value to a pin configured for output
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   gpio  - Identifies the GPIO pin {0,1}
 *   value - True: high, false: low
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_gpio_write(int fd, uint8_t gpio, bool value);

/****************************************************************************
 * Name: ft80x_gpio_read
 *
 * Description:
 *   Read the value from a pin configured for input
 *
 * Input Parameters:
 *   fd   - The file descriptor of the FT80x device.  Opened by the caller
 *          with write access.
 *   gpio - Identifies the GPIO pin {0,1}
 *
 * Returned Value:
 *   True: high, false: low
 *
 ****************************************************************************/

bool ft80x_gpio_read(int fd, uint8_t gpio);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_GRAPHICS_FT80X */
#endif /* __APPS_INCLUDE_GRAPHICS_FT80X_H */
