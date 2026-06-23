/****************************************************************************
 * apps/netutils/dropbear/port/dropbear_utils.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "dropbear_utils.h"

#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int dropbear_hex_value(char ch)
{
  if (ch >= '0' && ch <= '9')
    {
      return ch - '0';
    }

  if (ch >= 'a' && ch <= 'f')
    {
      return ch - 'a' + 10;
    }

  if (ch >= 'A' && ch <= 'F')
    {
      return ch - 'A' + 10;
    }

  return -1;
}

static int dropbear_try_mkdir(FAR const char *path)
{
  if (mkdir(path, 0700) < 0 && errno != EEXIST)
    {
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void dropbear_hex_encode(FAR char *dst, FAR const uint8_t *src,
                         size_t srclen)
{
  static const char hex[] = "0123456789abcdef";
  size_t i;

  for (i = 0; i < srclen; i++)
    {
      dst[i * 2] = hex[src[i] >> 4];
      dst[i * 2 + 1] = hex[src[i] & 0x0f];
    }

  dst[srclen * 2] = '\0';
}

int dropbear_hex_decode(FAR const char *src, size_t srclen,
                        FAR uint8_t *dst, size_t dstlen)
{
  size_t i;

  if (srclen != dstlen * 2)
    {
      return -EINVAL;
    }

  for (i = 0; i < dstlen; i++)
    {
      int hi = dropbear_hex_value(src[i * 2]);
      int lo = dropbear_hex_value(src[i * 2 + 1]);

      if (hi < 0 || lo < 0)
        {
          return -EINVAL;
        }

      dst[i] = (uint8_t)((hi << 4) | lo);
    }

  return OK;
}

int dropbear_try_prepare_parent(FAR const char *path)
{
  char dir[PATH_MAX];
  struct stat st;
  FAR char *slash;
  FAR char *p;
  int ret;

  if (strlcpy(dir, path, sizeof(dir)) >= sizeof(dir))
    {
      return -ENAMETOOLONG;
    }

  slash = strrchr(dir, '/');
  if (slash == NULL || slash == dir)
    {
      return OK;
    }

  *slash = '\0';
  if (stat(dir, &st) == 0)
    {
      return OK;
    }

  for (p = dir + 1; *p != '\0'; p++)
    {
      if (*p == '/')
        {
          *p = '\0';
          ret = dropbear_try_mkdir(dir);
          if (ret < 0)
            {
              return ret;
            }

          *p = '/';
        }
    }

  return dropbear_try_mkdir(dir);
}

#ifndef CONFIG_SCHED_USER_IDENTITY
int dropbear_getgroups(int size, gid_t list[])
{
  (void)size;
  (void)list;
  set_errno(ENOSYS);
  return ERROR;
}
#endif
