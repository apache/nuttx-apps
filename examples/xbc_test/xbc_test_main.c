/****************************************************************************
 * examples/xbc_test/xbc_test_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 *   Copyright (C) 2017 Brian Webb. All rights reserved.
 *   Author: Brian Webb <webbbn@gmail.com>
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/input/xbox-controller.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

/* Sanity checking */

#ifndef CONFIG_USBHOST
#  error "CONFIG_USBHOST is not defined"
#endif

#ifdef CONFIG_USBHOST_INT_DISABLE
#  error "Interrupt endpoints are disabled (CONFIG_USBHOST_INT_DISABLE)"
#endif

#ifndef CONFIG_NFILE_DESCRIPTORS
#  error "CONFIG_NFILE_DESCRIPTORS > 0 needed"
#endif

/* Provide some default values for other configuration settings */

#ifndef CONFIG_EXAMPLES_XBC_DEVNAME
#  define CONFIG_EXAMPLES_XBC_DEVNAME "/dev/xboxa"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int xbc_test_main(int argc, char *argv[])
#endif
{
  char buffer[256];
  ssize_t nbytes;
  int fd;

  /* Eventually logic here will open the controller device and perform the
   * controller test.
   */

  for (;;)
    {
      /* Open the controller device.  Loop until the device is successfully
       * opened.
       */

      do
        {
          printf("Opening device %s\n", CONFIG_EXAMPLES_XBC_DEVNAME);
          fd = open(CONFIG_EXAMPLES_XBC_DEVNAME, O_RDONLY);
          if (fd < 0)
            {
               printf("Failed: %d\n", errno);
               fflush(stdout);
               sleep(3);
            }
        }
      while (fd < 0);

      printf("Device %s opened\n", CONFIG_EXAMPLES_XBC_DEVNAME);
      fflush(stdout);

      /* Loop until there is a read failure (or EOF?) */

      do
        {
          /* Read a buffer of data */

          nbytes = read(fd, buffer, 256);
          if (nbytes > 0)
            {
              /* On success, echo the buffer to stdout */

	      printf("%d bytes read\n", nbytes);
	      if (nbytes == sizeof(struct xbox_controller_buttonstate_s))
		{
		  struct xbox_controller_buttonstate_s *rpt = (struct xbox_controller_buttonstate_s*)buffer;
		  printf("guide: %d  sync: %d  start: %d  back: %d  a: %d  b: %d  x: %d  y: %d\n",
			 rpt->guide, rpt->sync, rpt->start, rpt->back, rpt->a, rpt->b, rpt->x, rpt->y);
		  printf("dpad_u: %d  d: %d  l: %d  r: %d  bump_l: %d  r: %d  stick_l: %d  r: %d\n",
			 rpt->dpad_up, rpt->dpad_down, rpt->dpad_left, rpt->dpad_right,
			 rpt->bumper_left, rpt->bumper_right, rpt->stick_click_left, rpt->stick_click_right);
		  printf("stick_left_x: %d  y: %d  right_x: %d  y: %d  trigger_l: %d  r: %d\n",
			 rpt->stick_left_x, rpt->stick_left_y, rpt->stick_right_x, rpt->stick_right_y,
			 rpt->trigger_left, rpt->trigger_right);
		}
	    }
        }
      while (nbytes > 0);

      printf("Closing device %s: %d\n", CONFIG_EXAMPLES_XBC_DEVNAME, (int)nbytes);
      fflush(stdout);
      close(fd);
      break;
    }

  return 0;
}
