/****************************************************************************
 * apps/examples/lzf/lzf_main.c
 *
 *   Copyright (c) 2006 Stefan Traby <stefan@hello-penguin.com>
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <lzf.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLOCKSIZE     ((1 << CONFIG_SYSTEM_LZF_BLOG) - 1)
#define MAX_BLOCKSIZE BLOCKSIZE

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if CONFIG_SYSTEM_LZF != CONFIG_m
static sem_t g_exclsem = SEM_INITIALIZER(1);
#endif

static off_t g_nread;
static off_t g_nwritten;

static FAR const char *g_imagename;
static enum { COMPRESS, UNCOMPRESS } g_mode;
static bool g_verbose;
static bool g_force;
static unsigned long g_blocksize;
static lzf_state_t g_htab;
static uint8_t g_buf1[MAX_BLOCKSIZE + LZF_MAX_HDR_SIZE + 16];
static uint8_t g_buf2[MAX_BLOCKSIZE + LZF_MAX_HDR_SIZE + 16];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if CONFIG_SYSTEM_LZF != 2
static void lzf_exit(int exitcode) noreturn_function;
static void lzf_exit(int exitcode)
{
  sem_post(&g_exclsem);
  exit(exitcode);
}
#else
#  define lzf_exit(c) exit(c)
#endif

static void usage(int ret)
{
  fprintf(stderr, "\n"
          "lzf, a very lightweight compression/decompression utility written by Stefan Traby.\n"
          "uses liblzf written by Marc Lehmann <schmorp@schmorp.de> You can find more info at\n"
          "http://liblzf.plan9.de/\n"
          "\n"
          "usage: lzf [-dufhvb] [file ...]\n\n"
          "-c   Compress\n"
          "-d   Decompress\n"
          "-f   Force overwrite of output file\n"
          "-h   Give this help\n"
          "-v   Verbose mode\n"
          "-b # Set blocksize (max %lu)\n"
          "\n", (unsigned long)MAX_BLOCKSIZE);

  lzf_exit(ret);
}

static inline ssize_t rread(int fd, FAR void *buf, size_t len)
{
  FAR char *p = buf;
  ssize_t ret = 0;
  ssize_t offset = 0;

  while (len && (ret = read(fd, &p[offset], len)) > 0)
    {
      offset += ret;
      len -= ret;
    }

  g_nread += offset;

  if (ret < 0)
    {
      return ret;
    }

  return offset;
}

/* Returns 0 if all written else -1 */

static inline ssize_t wwrite(int fd, void *buf, size_t len)
{
  FAR char *b = buf;
  ssize_t ret;
  size_t l = len;

  while (l)
    {
      ret = write(fd, b, l);
      if (ret < 0)
        {
          fprintf(stderr, "%s: write error: %d\n", g_imagename, errno);
          return -1;
        }

      l -= ret;
      b += ret;
    }

  g_nwritten += len;
  return 0;
}

/* Anatomy: an lzf file consists of any number of blocks in the following format:
 *
 * \x00   EOF (optional)
 * "ZV\0" 2-byte-usize <uncompressed data>
 * "ZV\1" 2-byte-csize 2-byte-usize <compressed data>
 * "ZV\2" 4-byte-crc32-0xdebb20e3 (NYI)
 */

static int compress_fd(int from, int to)
{
  FAR struct lzf_header_s *header;
  ssize_t us;
  ssize_t len;

  g_nread = g_nwritten = 0;
  while ((us = rread(from, &g_buf1[LZF_MAX_HDR_SIZE], g_blocksize)) > 0)
    {
      len = lzf_compress(&g_buf1[LZF_MAX_HDR_SIZE], us, &g_buf2[LZF_MAX_HDR_SIZE],
                         us > 4 ? us - 4 : us, g_htab, &header);
      if (wwrite(to, header, len) == -1)
        {
          return -1;
        }
    }

  return 0;
}

static int uncompress_fd(int from, int to)
{
  uint8_t header[LZF_MAX_HDR_SIZE];
  FAR uint8_t *p;
  int l;
  int rd;
  ssize_t ret;
  ssize_t cs;
  ssize_t us;
  ssize_t bytes;
  ssize_t over = 0;

  g_nread = g_nwritten = 0;
  while (1)
    {
      ret = rread(from, header + over, LZF_MAX_HDR_SIZE - over);
      if (ret < 0)
        {
          fprintf(stderr, "%s: read error: %d\n", g_imagename, errno);
          return -1;
        }

      ret += over;
      over = 0;
      if (!ret || header[0] == 0)
        {
          return 0;
        }

      if (ret < LZF_MIN_HDR_SIZE || header[0] != 'Z' || header[1] != 'V')
        {
          fprintf(stderr, "%s: invalid data stream - magic not found or short header\n",
                  g_imagename);
          return -1;
        }

      switch (header[2])
        {
          case 0:
            cs = -1;
            us = (header[3] << 8) | header[4];
            p = &header[LZF_TYPE0_HDR_SIZE];
            break;

          case 1:
            if (ret < LZF_TYPE1_HDR_SIZE)
              {
                goto short_read;
              }

            cs = (header[3] << 8) | header[4];
            us = (header[5] << 8) | header[6];
            p = &header[LZF_TYPE1_HDR_SIZE];
            break;

          default:
            fprintf(stderr, "%s: unknown blocktype\n", g_imagename);
            return -1;
        }

      bytes = cs == -1 ? us : cs;
      l = &header[ret] - p;

      if (l > 0)
        {
          memcpy(g_buf1, p, l);
        }

      if (l > bytes)
        {
          over = l - bytes;
          memmove(header, &p[bytes], over);
        }

      p  = &g_buf1[l];
      rd = bytes - l;
      if (rd > 0)
        {
          if ((ret = rread(from, p, rd)) != rd)
            {
              goto short_read;
            }
        }

      if (cs == -1)
        {
          if (wwrite (to, g_buf1, us))
            {
              return -1;
            }
        }
      else
        {
          if (lzf_decompress(g_buf1, cs, g_buf2, us) != us)
            {
              fprintf(stderr, "%s: decompress: invalid stream - data corrupted\n",
                      g_imagename);
              return -1;
            }

          if (wwrite(to, g_buf2, us))
            {
              return -1;
            }
        }
    }

  return 0;

short_read:
  fprintf(stderr, "%s: short data\n", g_imagename);
  return -1;
}

static int open_out(FAR const char *name)
{
  int fd;
  int m = O_EXCL;

  if (g_force)
    {
      m = 0;
    }

  fd = open(name, O_CREAT | O_WRONLY | O_TRUNC | m, 600);
  return fd;
}

static int compose_name(FAR const char *fname, FAR char *oname, int namelen)
{
  FAR char *p;

  if (g_mode == COMPRESS)
    {
      if (strlen(fname) > PATH_MAX - 4)
        {
          fprintf(stderr, "%s: %s.lzf: name too long", g_imagename, fname);
          return -1;
        }

      strncpy(oname, fname, namelen);
      p = strchr(oname, '.');
      if (p != NULL)
        {
          *p = '_';  /* _ for dot */
        }

       strcat (oname, ".lzf");
    }
  else
    {
      if (strlen(fname) > PATH_MAX)
        {
          fprintf(stderr, "%s: %s: name too long\n", g_imagename, fname);
          return -1;
        }

      strcpy(oname, fname);
      p = strstr(oname, ".lzf");
      if (p == NULL)
        {
          fprintf(stderr, "%s: %s: unknown suffix\n", g_imagename, fname);
          return -1;
        }

      *p = 0;
      p  = strchr(oname, '_');
      if (p != NULL)
        {
          *p = '.';
        }
    }

  return 0;
}

static int run_file(FAR const char *fname)
{
  struct stat mystat;
  char oname[PATH_MAX + 1];
  int fd;
  int fd2;
  int ret;

  memset(oname, 0, sizeof(oname));
  if (compose_name(fname, oname, PATH_MAX + 1))
    {
      return -1;
    }

  ret = stat(fname, &mystat);
  fd = open(fname, O_RDONLY);

  if (ret || fd == -1)
    {
      fprintf(stderr, "%s: %s: %d\n", g_imagename, fname, errno);
      return -1;
    }

  if (!S_ISREG(mystat.st_mode))
    {
      fprintf(stderr, "%s: %s: not a regular file.\n", g_imagename, fname);
      close(fd);
      return -1;
    }

  fd2 = open_out(oname);
  if (fd2 == -1)
    {
      fprintf(stderr, "%s: %s: %d\n", g_imagename, oname, errno);
      close(fd);
      return -1;
    }

  if (g_mode == COMPRESS)
    {
      ret = compress_fd(fd, fd2);
      if (!ret && g_verbose)
        {
          fprintf(stderr, "%s:  %5.1f%% -- replaced with %s\n",
                  fname, g_nread == 0 ? 0 :
                  100.0 - g_nwritten / ((double) g_nread / 100.0), oname);
        }
    }
  else
    {
      ret = uncompress_fd(fd, fd2);
      if (!ret && g_verbose)
        {
          fprintf(stderr, "%s:  %5.1f%% -- replaced with %s\n",
                  fname, g_nwritten == 0 ? 0 :
                  100.0 - g_nread / ((double) g_nwritten / 100.0), oname);
        }
    }

  close(fd);
  close(fd2);

  if (!ret)
    {
      unlink(fname);
    }

  return ret;
}

/****************************************************************************
 * lzf_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *p = argv[0];
  int optc;
  int ret = 0;

#if CONFIG_SYSTEM_LZF != CONFIG_m
  /* Get exclusive access to the global variables.  Global variables are
   * used because the hash table and buffers are too large to allocate on
   * the embedded stack.  But the use of global variables has the downside
   * or forcing serialization of this logic in order to work in a multi-
   * tasking environment.
   *
   * REVISIT:  An alternative would be to pack all of the globals into a
   * structure and allocate a per-thread instance of that structure here.
   *
   * NOTE:  This applies only in the FLAT and PROTECTED build modes.  In the
   * KERNEL build mode, this will be a separate process with its own private
   * global variables.
   */

  ret = sem_wait(&g_exclsem);
  if (ret < 0)
    {
      fprintf(stderr, "sem_wait failed: %d\n", errno);
      exit(1);
    }
#endif

  /* Set defaults. */

  g_mode      = COMPRESS;
  g_verbose   = false;
  g_force     = 0;
  g_blocksize = BLOCKSIZE;

#ifndef CONFIG_DISABLE_ENVIRON
  /* Block size may be specified as an environment variable */

  p = getenv("LZF_BLOCKSIZE");
  if (p)
    {
      g_blocksize = strtoul(p, 0, 0);
      if (g_blocksize == 0 || g_blocksize > MAX_BLOCKSIZE)
        {
          g_blocksize = BLOCKSIZE;
        }
    }
#endif

  /* Get the program name sans path */

  p = strrchr(argv[0], '/');
  g_imagename = p ? ++p : argv[0];

  /* Handle command line options */

  while ((optc = getopt(argc, argv, "cdfhvb:")) != -1)
    {
      switch (optc)
        {
          case 'c':
            g_mode = COMPRESS;
            break;

          case 'd':
            g_mode = UNCOMPRESS;
            break;

          case 'f':
            g_force = true;
            break;

          case 'h':
            usage(0);
            break;

          case 'v':
            g_verbose = true;
            break;

          case 'b':
            g_blocksize = strtoul(optarg, 0, 0);
            if (g_blocksize == 0 || g_blocksize > MAX_BLOCKSIZE)
              {
                g_blocksize = BLOCKSIZE;
              }

            break;

          default:
            usage(1);
            break;
        }
    }

  if (optind == argc)
    {
      /* stdin stdout */

#ifdef CONFIG_SERIAL_TERMIOS
      if (!g_force)
        {
          if ((g_mode == UNCOMPRESS) && isatty(0))
            {
              fprintf(stderr, "%s: compressed data not read from a terminal. "
                      "Use -f to force decompression.\n", g_imagename);
              lzf_exit(1);
            }

          if (g_mode == COMPRESS && isatty(1))
            {
              fprintf(stderr, "%s: compressed data not written to a terminal. "
                      "Use -f to force compression.\n", g_imagename);
              lzf_exit(1);
            }
        }
#endif

      if (g_mode == COMPRESS)
        {
          ret = compress_fd(0, 1);
        }
      else
        {
          ret = uncompress_fd(0, 1);
        }

      lzf_exit(ret ? 1 : 0);
    }

  while (optind < argc)
    {
      ret |= run_file(argv[optind++]);
    }

  lzf_exit(ret ? 1 : 0);
}
