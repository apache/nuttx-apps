/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_font.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 ****************************************************************************/

#ifndef __TEXTSCREEN_FONT_H__
#define __TEXTSCREEN_FONT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct
{
  const char *name;
  const uint8_t *data;
  unsigned int w;
  unsigned int h;
} txt_font_t;

#endif /* __TEXTSCREEN_FONT_H__ */
