#include <fcntl.h>
#include <unistd.h>
#include <nuttx/config.h>
#include <nuttx/input/keyboard.h>
#include <nuttx/input/kbd_codec.h>
#include "device.h"
#include "mwtypes.h"

enum kbd_mode_e
  {
    KBD_MODE_NONE = 0,
    KBD_MODE_EVENT = 1,
    KBD_MODE_RAW = 2,
  };

static int kbd_fd = -1;
static enum kbd_mode_e g_kbd_mode = KBD_MODE_NONE;
static MWKEYMOD modifiers = 0;

static bool g_pending_release = false;
static uint8_t g_last_raw_key = 0;

/* Standard keyboard_event_s paths, eg. sim */
static const char *g_kbd_event_paths[] = {
  "/dev/kbd",
  "/dev/kbd0",
  "/dev/kbd1",
  "/dev/ukeyboard",
  NULL
};

/* Legacy raw character paths, eg. qemu */
static const char *g_kbd_raw_paths[] = {
  "/dev/kbda",
  "/dev/kbdb",
  NULL
};

static int nuttxkbd_Open(KBDDEVICE * pkd);
static void nuttxkbd_Close(void);
static void nuttxkbd_GetModifierInfo(MWKEYMOD * mods, MWKEYMOD * curmods);
static int nuttxkbd_Read(MWKEY * kbuf, MWKEYMOD * mods, MWSCANCODE * scancode);

KBDDEVICE kbddev = {
  nuttxkbd_Open,
  nuttxkbd_Close,
  nuttxkbd_GetModifierInfo,
  nuttxkbd_Read,
  NULL
};

static MWKEY translate_keycode(uint32_t code)
{
  switch (code)
    {
    case KEYCODE_BACKDEL:
      return MWKEY_BACKSPACE;
    case KEYCODE_FWDDEL:
      return MWKEY_DELETE;
    case KEYCODE_HOME:
      return MWKEY_HOME;
    case KEYCODE_END:
      return MWKEY_END;
    case KEYCODE_LEFT:
      return MWKEY_LEFT;
    case KEYCODE_RIGHT:
      return MWKEY_RIGHT;
    case KEYCODE_UP:
      return MWKEY_UP;
    case KEYCODE_DOWN:
      return MWKEY_DOWN;
    case KEYCODE_PAGEUP:
      return MWKEY_PAGEUP;
    case KEYCODE_PAGEDOWN:
      return MWKEY_PAGEDOWN;
    case KEYCODE_INSERT:
      return MWKEY_INSERT;
    case KEYCODE_ENTER:
      return MWKEY_ENTER;
    case KEYCODE_F1:
      return MWKEY_F1;
    case KEYCODE_F2:
      return MWKEY_F2;
    case KEYCODE_F3:
      return MWKEY_F3;
    case KEYCODE_F4:
      return MWKEY_F4;
    case KEYCODE_F5:
      return MWKEY_F5;
    case KEYCODE_F6:
      return MWKEY_F6;
    case KEYCODE_F7:
      return MWKEY_F7;
    case KEYCODE_F8:
      return MWKEY_F8;
    case KEYCODE_F9:
      return MWKEY_F9;
    case KEYCODE_F10:
      return MWKEY_F10;
    case KEYCODE_F11:
      return MWKEY_F11;
    case KEYCODE_F12:
      return MWKEY_F12;
    case KEYCODE_CAPSLOCK:
      return MWKEY_CAPSLOCK;
    case KEYCODE_NUMLOCK:
      return MWKEY_NUMLOCK;
    case KEYCODE_SCROLLLOCK:
      return MWKEY_SCROLLOCK;
    case KEYCODE_PAUSE:
      return MWKEY_PAUSE;
    case KEYCODE_PRTSCRN:
      return MWKEY_PRINT;
    case KEYCODE_MENU:
      return MWKEY_MENU;

    default:
      if (code < 128)
        {
          return (MWKEY) code;
        }
      return MWKEY_UNKNOWN;
    }
}

static void update_modifiers(MWKEY key, int press)
{
  MWKEYMOD mask = 0;

  switch (key)
    {
    case MWKEY_LSHIFT:
      mask = MWKMOD_LSHIFT;
      break;
    case MWKEY_RSHIFT:
      mask = MWKMOD_RSHIFT;
      break;
    case MWKEY_LCTRL:
      mask = MWKMOD_LCTRL;
      break;
    case MWKEY_RCTRL:
      mask = MWKMOD_RCTRL;
      break;
    case MWKEY_LALT:
      mask = MWKMOD_LALT;
      break;
    case MWKEY_RALT:
      mask = MWKMOD_RALT;
      break;
    case MWKEY_LMETA:
      mask = MWKMOD_LMETA;
      break;
    case MWKEY_RMETA:
      mask = MWKMOD_RMETA;
      break;
    case MWKEY_NUMLOCK:
      mask = MWKMOD_NUM;
      break;
    case MWKEY_CAPSLOCK:
      mask = MWKMOD_CAPS;
      break;
    }

  if (mask)
    {
      if (press)
        {
          modifiers |= mask;
        }
      else
        {
          modifiers &= ~mask;
        }
    }
}

static MWKEY raw_byte_to_mwkey(uint8_t ch)
{
  if (ch < 128)
    {
      return (MWKEY) ch;
    }
  return MWKEY_UNKNOWN;
}

static int nuttxkbd_Open(KBDDEVICE *pkd)
{
  int i;
  useconds_t delay = 1000;

  if (kbd_fd >= 0)
    {
      close(kbd_fd);
      kbd_fd = -1;
    }

  g_kbd_mode = KBD_MODE_NONE;
  g_pending_release = false;
  modifiers = 0;

  /* Retry with exponential backoff to wait for asynchronously loaded input
   * device drivers (e.g. USB HID) to register their device nodes. */
  while (delay <= 1024000)
    {
      for (i = 0; g_kbd_event_paths[i]; i++)
        {
          kbd_fd = open(g_kbd_event_paths[i], O_RDONLY | O_NONBLOCK);
          if (kbd_fd >= 0)
            {
              g_kbd_mode = KBD_MODE_EVENT;
              return DRIVER_OKFILEDESC(kbd_fd);
            }
        }

      for (i = 0; g_kbd_raw_paths[i]; i++)
        {
          kbd_fd = open(g_kbd_raw_paths[i], O_RDONLY | O_NONBLOCK);
          if (kbd_fd >= 0)
            {
              g_kbd_mode = KBD_MODE_RAW;
              g_pending_release = false;
              return DRIVER_OKFILEDESC(kbd_fd);
            }
        }

      usleep(delay);
      delay *= 2;
    }

  return DRIVER_FAIL;
}

static void nuttxkbd_Close(void)
{
  if (kbd_fd >= 0)
    {
      close(kbd_fd);
      kbd_fd = -1;
    }
  modifiers = 0;
  g_kbd_mode = KBD_MODE_NONE;
  g_pending_release = false;
}

static void nuttxkbd_GetModifierInfo(MWKEYMOD *mods, MWKEYMOD *curmods)
{
  if (mods)
    {
      *mods = MWKMOD_SHIFT |
        MWKMOD_CTRL | MWKMOD_ALT | MWKMOD_META | MWKMOD_NUM | MWKMOD_CAPS;
    }
  if (curmods)
    {
      *curmods = modifiers;
    }
}

static int nuttxkbd_Read(MWKEY *kbuf, MWKEYMOD *mods, MWSCANCODE *scancode)
{
  if (g_kbd_mode == KBD_MODE_EVENT)
    {
      struct keyboard_event_s event;
      int n;

      n = read(kbd_fd, &event, sizeof(event));
      if (n < (int)sizeof(event))
        {
          return KBD_NODATA;
        }

      MWKEY key = translate_keycode(event.code);
      int press = (event.type == KEYBOARD_PRESS);

      update_modifiers(key, press);

      *kbuf = key;
      *mods = modifiers;
      *scancode = 0;

      return press ? KBD_KEYPRESS : KBD_KEYRELEASE;
    }
  else
    {
      uint8_t ch;
      int n;

      if (g_pending_release)
        {
          g_pending_release = false;
          *kbuf = (MWKEY) g_last_raw_key;
          *mods = modifiers;
          *scancode = 0;
          return KBD_KEYRELEASE;
        }

      n = read(kbd_fd, &ch, 1);
      if (n < 1)
        {
          return KBD_NODATA;
        }

      MWKEY key = raw_byte_to_mwkey(ch);
      if (key == MWKEY_UNKNOWN)
        {
          return KBD_NODATA;
        }

      g_pending_release = true;
      g_last_raw_key = ch;

      *kbuf = key;
      *mods = modifiers;
      *scancode = 0;
      return KBD_KEYPRESS;
    }
}
