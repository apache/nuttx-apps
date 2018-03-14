/****************************************************************************
 * apps/exmaples/lzf/lxf_main.c
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
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <lzf.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLOCKSIZE (1024 * 1 - 1)
#define MAX_BLOCKSIZE BLOCKSIZE

#define TYPE0_HDR_SIZE 5
#define TYPE1_HDR_SIZE 7
#define MAX_HDR_SIZE 7
#define MIN_HDR_SIZE 5

/****************************************************************************
 * Private Data
 ****************************************************************************/

static off_t g_nread;
static off_t g_nwritten;

static FAR const char *g_imagename;
static enum { COMPRESS, UNCOMPRESS, LZCAT } g_mode = COMPRESS;
static bool g_verbose = false;
static bool g_force = 0;
static long blocksize = BLOCKSIZE;

static FAR const char *opt =
  "-c   compress\n"
  "-d   decompress\n"
  "-f   force overwrite of output file\n"
  "-h   give this help\n"
  "-v   verbose mode\n"
  "-b # set blocksize\n"
  "\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(int ret)
{
  fprintf (stderr, "\n"
           "lzf, a very lightweight compression/decompression utility written by Stefan Traby.\n"
           "uses liblzf written by Marc Lehmann <schmorp@schmorp.de> You can find more info at\n"
           "http://liblzf.plan9.de/\n"
           "\n"
           "usage: lzf [-dufhvb] [file ...]\n"
           "       unlzf [file ...]\n"
           "       lzcat [file ...]\n"
           "\n%s",
           opt);

  exit(ret);
}

static inline ssize_t rread(int fd, FAR void *buf, size_t len)
{
  FAR char *p = buf;
  ssize_t ret = 0;
  ssize_t offset = 0;

  while (len && (ret = read (fd, &p[offset], len)) > 0)
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
      ret = write (fd, b, l);
      if (ret < 0)
        {
          fprintf (stderr, "%s: write error: %d\n", g_imagename, errno);
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

static int compress_fd (int from, int to)
{
  ssize_t us;
  ssize_t cs;
  ssize_t len;
  uint8_t buf1[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
  uint8_t buf2[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
  uint8_t *header;

  g_nread = g_nwritten = 0;
  while ((us = rread (from, &buf1[MAX_HDR_SIZE], blocksize)) > 0)
    {
      cs = lzf_compress (&buf1[MAX_HDR_SIZE], us, &buf2[MAX_HDR_SIZE], us > 4 ? us - 4 : us);
      if (cs)
        {
          header    = &buf2[MAX_HDR_SIZE - TYPE1_HDR_SIZE];
          header[0] = 'Z';
          header[1] = 'V';
          header[2] = 1;
          header[3] = cs >> 8;
          header[4] = cs & 0xff;
          header[5] = us >> 8;
          header[6] = us & 0xff;
          len       = cs + TYPE1_HDR_SIZE;
        }
      else
        {
          /* Write uncompressed */

          header    = &buf1[MAX_HDR_SIZE - TYPE0_HDR_SIZE];
          header[0] = 'Z';
          header[1] = 'V';
          header[2] = 0;
          header[3] = us >> 8;
          header[4] = us & 0xff;
          len      = us + TYPE0_HDR_SIZE;
        }

      if (wwrite (to, header, len) == -1)
        {
          return -1;
        }
    }

  return 0;
}

static int uncompress_fd(int from, int to)
{
  uint8_t header[MAX_HDR_SIZE];
  uint8_t buf1[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
  uint8_t buf2[MAX_BLOCKSIZE + MAX_HDR_SIZE + 16];
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
      ret = rread (from, header + over, MAX_HDR_SIZE - over);
      if (ret < 0)
        {
          fprintf (stderr, "%s: read error: ", g_imagename);
          perror ("");
          return -1;
        }

      ret += over;
      over = 0;
      if (!ret || header[0] == 0)
        {
          return 0;
        }

      if (ret < MIN_HDR_SIZE || header[0] != 'Z' || header[1] != 'V')
        {
          fprintf (stderr, "%s: invalid data stream - magic not found or short header\n",
                   g_imagename);
          return -1;
        }

      switch (header[2])
        {
          case 0:
            cs = -1;
            us = (header[3] << 8) | header[4];
            p = &header[TYPE0_HDR_SIZE];
            break;

          case 1:
            if (ret < TYPE1_HDR_SIZE)
              {
                goto short_read;
              }

            cs = (header[3] << 8) | header[4];
            us = (header[5] << 8) | header[6];
            p = &header[TYPE1_HDR_SIZE];
            break;

          default:
            fprintf (stderr, "%s: unknown blocktype\n", g_imagename);
            return -1;
        }

      bytes = cs == -1 ? us : cs;
      l = &header[ret] - p;

      if (l > 0)
        {
          memcpy (buf1, p, l);
        }

      if (l > bytes)
        {
          over = l - bytes;
          memmove (header, &p[bytes], over);
        }

      p  = &buf1[l];
      rd = bytes - l;
      if (rd > 0)
        {
          if ((ret = rread (from, p, rd)) != rd)
            {
              goto short_read;
            }
        }

      if (cs == -1)
        {
          if (wwrite (to, buf1, us))
            {
              return -1;
            }
        }
      else
        {
          if (lzf_decompress (buf1, cs, buf2, us) != us)
            {
              fprintf (stderr, "%s: decompress: invalid stream - data corrupted\n",
                       g_imagename);
              return -1;
            }

          if (wwrite (to, buf2, us))
            {
              return -1;
            }
        }
    }

  return 0;

short_read:
  fprintf (stderr, "%s: short data\n", g_imagename);
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

  fd = open (name, O_CREAT | O_WRONLY | O_TRUNC | m, 600);
  return fd;
}

static int compose_name (FAR const char *fname, FAR char *oname)
{
  FAR char *p;

  if (g_mode == COMPRESS)
    {
      if (strlen (fname) > PATH_MAX - 4)
        {
          fprintf (stderr, "%s: %s.lzf: name too long", g_imagename, fname);
          return -1;
        }

      strcpy (oname, fname);
      p = strchr (oname, '.');
      *p = '_'; /* _ for dot */
      strcat (oname, ".lzf");
    }
  else
    {
      if (strlen (fname) > PATH_MAX)
        {
          fprintf (stderr, "%s: %s: name too long\n", g_imagename, fname);
          return -1;
        }

      strcpy (oname, fname);
      p = &oname[strlen (oname)] - 4;
      if (p < oname || strcmp (p, ".lzf"))
        {
          fprintf (stderr, "%s: %s: unknown suffix\n", g_imagename, fname);
          return -1;
        }

      *p = 0;
      p = strchr (oname, '_');
      *p = '.';
    }

  return 0;
}

static int run_file(const char *fname)
{
  int fd, fd2;
  int ret;
  struct stat mystat;
  char oname[PATH_MAX + 1];

  if (g_mode != LZCAT)
    {
      if (compose_name (fname, oname))
        {
          return -1;
        }
    }

  ret = stat (fname, &mystat);
  fd = open (fname, O_RDONLY);

  if (ret || fd == -1)
    {
      fprintf (stderr, "%s: %s: %d\n", g_imagename, fname, errno);
      return -1;
    }

  if (!S_ISREG (mystat.st_mode))
    {
      fprintf (stderr, "%s: %s: not a regular file.\n", g_imagename, fname);
      close (fd);
      return -1;
    }

  if (g_mode == LZCAT)
    {
      ret = uncompress_fd(fd, 1);
      close (fd);
      return ret;
    }

  fd2 = open_out(oname);
  if (fd2 == -1)
    {
      fprintf (stderr, "%s: %s: %d\n", g_imagename, oname, errno);
      close (fd);
      return -1;
    }

  if (g_mode == COMPRESS)
    {
      ret = compress_fd(fd, fd2);
      if (!ret && g_verbose)
        {
          fprintf (stderr, "%s:  %5.1f%% -- replaced with %s\n",
                   fname, g_nread == 0 ? 0 :
                   100.0 - g_nwritten / ((double) g_nread / 100.0), oname);
        }
    }
  else
    {
      ret = uncompress_fd(fd, fd2);
      if (!ret && g_verbose)
        {
          fprintf (stderr, "%s:  %5.1f%% -- replaced with %s\n",
                   fname, g_nwritten == 0 ? 0 :
                   100.0 - g_nread / ((double) g_nwritten / 100.0), oname);
        }
    }

  close (fd);
  close (fd2);

  if (!ret)
    {
      unlink (fname);
    }

  return ret;
}

/****************************************************************************
 * lzf_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int lzf_main(int argc, FAR char *argv[])
#endif
{
  FAR char *p = argv[0];
  int optc;
  int ret = 0;

#ifndef CONFIG_DISABLE_ENVIRON
  p = getenv ("LZF_BLOCKSIZE");
  if (p)
    {
      blocksize = strtoul (p, 0, 0);
      if (!blocksize || blocksize > MAX_BLOCKSIZE)
        {
          blocksize = BLOCKSIZE;
        }
    }
#endif

  p = strrchr (argv[0], '/');
  g_imagename = p ? ++p : argv[0];

  if (!strncmp (g_imagename, "un", 2) || !strncmp (g_imagename, "de", 2))
    {
      g_mode = UNCOMPRESS;
    }

  if (strstr (g_imagename, "cat"))
    {
      g_mode = LZCAT;
    }

  while ((optc = getopt (argc, argv, "cdfhvb:")) != -1)
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
            usage (0);
            break;

          case 'v':
            g_verbose = true;
            break;

          case 'b':
            errno = 0;
            blocksize = strtoul(optarg, 0, 0);
            if (errno || !blocksize || blocksize > MAX_BLOCKSIZE)
              {
                blocksize = BLOCKSIZE;
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
          if ((g_mode == UNCOMPRESS || g_mode == LZCAT) && isatty(0))
            {
              fprintf (stderr, "%s: compressed data not read from a terminal. "
                       "Use -f to force decompression.\n", g_imagename);
              exit (1);
            }

          if (g_mode == COMPRESS && isatty (1))
            {
              fprintf (stderr, "%s: compressed data not written to a terminal. "
                       "Use -f to force compression.\n", g_imagename);
              exit (1);
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

      exit (ret ? 1 : 0);
    }

  while (optind < argc)
    {
      ret |= run_file (argv[optind++]);
    }

  exit (ret ? 1 : 0);
}
