#include <fcntl.h>
#include <unistd.h>
#include <nuttx/config.h>
#include <nuttx/input/mouse.h>
#include <nuttx/input/touchscreen.h>
#include "device.h"

#define SCALE   3
#define THRESH  5

#define TYPE_REALMOUSE 1
#define TYPE_TOUCHSCREEN 0
#define MOUSE_DEVICE_PATH "/dev/mouse0"
#define TOUCHSCREEN_DEVICE_PATH "/dev/input0"

static int fd = -1;
static int mouse_type = TYPE_TOUCHSCREEN;

static int nuttxmouse_Open(MOUSEDEVICE * pmd);
static void nuttxmouse_Close(void);
static int nuttxmouse_GetButtonInfo(void);
static void nuttxmouse_GetDefaultAccel(int *pscale, int *pthresh);
static int nuttxmouse_Read(MWCOORD * dx, MWCOORD * dy, MWCOORD * dz, int *bp);

MOUSEDEVICE mousedev = {
  nuttxmouse_Open,
  nuttxmouse_Close,
  nuttxmouse_GetButtonInfo,
  nuttxmouse_GetDefaultAccel,
  nuttxmouse_Read,
  NULL,
  MOUSE_NORMAL
};

static int nuttxmouse_Open(MOUSEDEVICE *pmd)
{
  useconds_t delay = 1000;

  /* Retry with exponential backoff to wait for asynchronously loaded input
   * device drivers (e.g. USB HID) to register their device nodes. */
  while (delay <= 1024000)
    {
      fd = open(MOUSE_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
      if (fd >= 0)
        {
          mouse_type = TYPE_REALMOUSE;
          return DRIVER_OKFILEDESC(fd);
        }

      fd = open(TOUCHSCREEN_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
      if (fd >= 0)
        {
          mouse_type = TYPE_TOUCHSCREEN;
          return DRIVER_OKFILEDESC(fd);
        }

      usleep(delay);
      delay *= 2;
    }

  return MOUSE_FAIL;
}

static void nuttxmouse_Close(void)
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
}

static int nuttxmouse_GetButtonInfo(void)
{
  if (mouse_type == TYPE_REALMOUSE)
    {
      return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R | MWBUTTON_SCROLLUP |
        MWBUTTON_SCROLLDN;
    }
  else
    {
      return MWBUTTON_L;
    }
}

static void nuttxmouse_GetDefaultAccel(int *pscale, int *pthresh)
{
  *pscale = SCALE;
  *pthresh = THRESH;
}

static int nuttxmouse_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
  if (mouse_type == TYPE_REALMOUSE)
    {
      struct mouse_report_s report;
      int n = read(fd, &report, sizeof(report));
      if (n <= 0)
        return MOUSE_NODATA;

      *dx = report.x;
      *dy = report.y;
      *dz = report.wheel;

      *bp = 0;
      if (report.buttons & MOUSE_BUTTON_1)
        *bp |= MWBUTTON_L;
      if (report.buttons & MOUSE_BUTTON_2)
        *bp |= MWBUTTON_R;
      if (report.buttons & MOUSE_BUTTON_3)
        *bp |= MWBUTTON_M;

      if (report.wheel > 0)
        *bp |= MWBUTTON_SCROLLUP;
      if (report.wheel < 0)
        *bp |= MWBUTTON_SCROLLDN;

      return MOUSE_ABSPOS;
    }
  else
    {
      struct touch_sample_s sample;
      int n = read(fd, &sample, sizeof(sample));
      if (n <= 0)
        return MOUSE_NODATA;

      *dz = 0;

      uint8_t flags = sample.point[0].flags;
      *bp = (flags & (TOUCH_DOWN | TOUCH_MOVE)) ? MWBUTTON_L : 0;

      if (flags & TOUCH_POS_VALID)
        {
          *dx = sample.point[0].x;
          *dy = sample.point[0].y;
          return MOUSE_ABSPOS;
        }

      return MOUSE_NOMOVE;
    }
}
