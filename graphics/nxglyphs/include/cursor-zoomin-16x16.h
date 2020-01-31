#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/video/rgbcolors.h>
#include <nuttx/nx/nxcursor.h>

#if CONFIG_NXWIDGETS_BPP == 8
#  define FGCOLOR1             RGB8_WHITE
#  define FGCOLOR2             RGB8_BLACK
#  define FGCOLOR3             RGB8_GRAY
#elif CONFIG_NXWIDGETS_BPP == 16
#  define FGCOLOR1             RGB16_WHITE
#  define FGCOLOR2             RGB16_BLACK
#  define FGCOLOR3             RGB16_GRAY
#elif CONFIG_NXWIDGETS_BPP == 24 || CONFIG_NXWIDGETS_BPP == 32
#  define FGCOLOR1             RGB24_WHITE
#  define FGCOLOR2             RGB24_BLACK
#  define FGCOLOR3             RGB24_GRAY
#else
#  error "Pixel depth not supported (CONFIG_NXWIDGETS_BPP)"
#endif

static const uint8_t g_zoomInImage[] =
{
  0x00, 0xaa, 0x00, 0x00,    /* Row 0 */
  0x0a, 0xd7, 0xa0, 0x00,    /* Row 1 */
  0x29, 0x55, 0x68, 0x00,    /* Row 2 */
  0x25, 0x55, 0x58, 0x00,    /* Row 3 */
  0xb5, 0x59, 0x5e, 0x00,    /* Row 4 */
  0x95, 0x59, 0x56, 0x00,    /* Row 5 */
  0x95, 0xaa, 0xd6, 0x00,    /* Row 6 */
  0xb5, 0x59, 0x5e, 0x00,    /* Row 7 */
  0x25, 0x5d, 0x58, 0x00,    /* Row 8 */
  0x29, 0x55, 0x7e, 0x00,    /* Row 9 */
  0x0a, 0xd7, 0xb7, 0x80,    /* Row 10 */
  0x00, 0xaa, 0x2d, 0xe0,    /* Row 11 */
  0x00, 0x00, 0x0b, 0x78,    /* Row 12 */
  0x00, 0x00, 0x02, 0xde,    /* Row 13 */
  0x00, 0x00, 0x00, 0xb6,    /* Row 14 */
  0x00, 0x00, 0x00, 0x2a,    /* Row 15 */
};

const struct nx_cursorimage_s g_zoomInCursor =
{
  .size =
  {
    .w = 16,
    .h = 16
  },
  .color1 =
  {
    FGCOLOR1
  },
  .color2 =
  {
    FGCOLOR1
  },
  .color3 =
  {
    FGCOLOR3
  },
  .image  = g_zoomInImage
};
