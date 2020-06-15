/****************************************************************************
 * examples/configdata/configdata_main.c
 *
 *   Copyright (C) 2013 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <crc32.h>
#include <debug.h>

#include <nuttx/mtd/mtd.h>
#include <nuttx/mtd/configdata.h>
#include <nuttx/fs/ioctl.h>
#include <sys/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* The default is to use the RAM MTD device at drivers/mtd/rammtd.c.  But
 * an architecture-specific MTD driver can be used instead by defining
 * CONFIG_EXAMPLES_CONFIGDATA_ARCHINIT.  In this case, the initialization
 * logic will call configdata_archinitialize() to obtain the MTD driver
 * instance.
 */

#ifndef CONFIG_EXAMPLES_CONFIGDATA_ARCHINIT

/* This must exactly match the default configuration in
 * drivers/mtd/rammtd.c
 */

#  ifndef CONFIG_RAMMTD_ERASESIZE
#    define CONFIG_RAMMTD_ERASESIZE 4096
#  endif

#  ifndef CONFIG_EXAMPLES_CONFIGDATA_NEBLOCKS
#    define CONFIG_EXAMPLES_CONFIGDATA_NEBLOCKS (256)
#  endif

#  define EXAMPLES_CONFIGDATA_BUFSIZE \
  (CONFIG_RAMMTD_ERASESIZE * CONFIG_EXAMPLES_CONFIGDATA_NEBLOCKS)
#endif

#ifndef CONFIG_EXAMPLES_CONFIGDATA_MAXSIZE
#  define CONFIG_EXAMPLES_CONFIGDATA_MAXSIZE 512
#endif

#ifndef CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES
#  define CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES 3000
#endif

#ifndef CONFIG_EXAMPLES_CONFIGDATA_NLOOPS
#  define CONFIG_EXAMPLES_CONFIGDATA_NLOOPS 100
#endif

#define EXAMPLES_CONFIGDATA_REPORT  (CONFIG_EXAMPLES_CONFIGDATA_NLOOPS / 20)
#if EXAMPLES_CONFIGDATA_REPORT == 0
#  undef EXAMPLES_CONFIGDATA_REPORT
#  define EXAMPLES_CONFIGDATA_REPORT 1
#endif

#ifndef CONFIG_EXAMPLES_CONFIGDATA_VERBOSE
#  define CONFIG_EXAMPLES_CONFIGDATA_VERBOSE 0
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct configdata_entrydesc_s
{
  uint16_t id;
  uint8_t  instance;
  uint16_t len;
  uint32_t crc;
  uint8_t  deleted;
  uint8_t  changed;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Pre-allocated simulated flash */

#ifndef CONFIG_EXAMPLES_CONFIGDATA_ARCHINIT
static uint8_t g_simflash[EXAMPLES_CONFIGDATA_BUFSIZE << 1];
#endif

static uint8_t g_entryimage[CONFIG_EXAMPLES_CONFIGDATA_MAXSIZE];
static struct configdata_entrydesc_s
  g_entries[CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES];

static int g_nentries;
static int g_ndeleted;
static int g_fd;
static int g_ntests;
static int g_nverified;
static int g_ntotalalloc;
static int g_ntotaldelete;

static struct mallinfo g_mmbefore;
static struct mallinfo g_mmafter;
#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
static struct mallinfo g_mmprevious;
#endif

/****************************************************************************
 * External Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_CONFIGDATA_ARCHINIT
extern FAR struct mtd_dev_s *configdata_archinitialize(void);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: configdata_memusage
 ****************************************************************************/

static void configdata_showmemusage(struct mallinfo *mmbefore,
                               struct mallinfo *mmafter)
{
  printf("VARIABLE  BEFORE   AFTER\n");
  printf("======== ======== ========\n");
  printf("arena    %8x %8x\n", mmbefore->arena,    mmafter->arena);
  printf("ordblks  %8d %8d\n", mmbefore->ordblks,  mmafter->ordblks);
  printf("mxordblk %8x %8x\n", mmbefore->mxordblk, mmafter->mxordblk);
  printf("uordblks %8x %8x\n", mmbefore->uordblks, mmafter->uordblks);
  printf("fordblks %8x %8x\n", mmbefore->fordblks, mmafter->fordblks);
}

/****************************************************************************
 * Name: configdata_loopmemusage
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
static void configdata_loopmemusage(void)
{
  /* Get the current memory usage */

  g_mmafter = mallinfo();

  /* Show the change from the previous loop */

  printf("\nEnd of loop memory usage:\n");
  configdata_showmemusage(&g_mmprevious, &g_mmafter);

  /* Set up for the next test */

  g_mmprevious = g_mmafter;
}
#endif

/****************************************************************************
 * Name: configdata_endmemusage
 ****************************************************************************/

static void configdata_endmemusage(void)
{
  g_mmafter = mallinfo();

  printf("\nFinal memory usage:\n");
  configdata_showmemusage(&g_mmbefore, &g_mmafter);

  printf("\nTotal adds: %d  Total deletes : %d\n",
         g_ntotalalloc, g_ntotaldelete);
  printf("Total tests: %d  Number passed: %d  Failed: %d\n",
         g_ntests, g_nverified, g_ntests - g_nverified);
}

/****************************************************************************
 * Name: configdata_randid
 ****************************************************************************/

static inline uint16_t configdata_randid(void)
{
  int x;
  int value;

retry:
  value = rand() & 0x7fff;
  if (value == 0)
    {
      value = 100;
    }

  /* Ensure we don't have a duplicate id */

  for (x = 0; x < g_nentries; x++)
    {
      if (value == g_entries[x].id)
        {
          goto retry;
        }
    }

  return value;
}

/****************************************************************************
 * Name: configdata_randlen
 ****************************************************************************/

static inline uint16_t configdata_randlen(void)
{
  int value = rand() % CONFIG_EXAMPLES_CONFIGDATA_MAXSIZE;

  if (value == 0)
    {
      return 1;
    }

  return value;
}

/****************************************************************************
 * Name: configdata_randinstance
 ****************************************************************************/

static inline uint8_t configdata_randinstance(void)
{
  return rand() & 0xff;
}

/****************************************************************************
 * Name: configdata_freefile
 ****************************************************************************/

#if 0 /* Not used */
static void configdata_freeentry(FAR struct configdata_entrydesc_s *entry)
{
  memset(entry, 0, sizeof(struct configdata_entrydesc_s));
}
#endif

/****************************************************************************
 * Name: configdata_wrentry
 ****************************************************************************/

static inline int
  configdata_wrentry(FAR struct configdata_entrydesc_s *entry)
{
  size_t x;
  int ret;
  struct config_data_s  config;

  /* Create a random entry */

  entry->id = configdata_randid();
  entry->instance = configdata_randinstance();
  entry->len = configdata_randlen();

  /* Write some random data to the entry */

  for (x = 0; x < entry->len; x++)
    {
      g_entryimage[x] = rand() & 0xff;
    }

  /* Calculate the crc32 for the data */

  entry->crc = crc32(g_entryimage, entry->len);

  /* Write the entry to the /dev/config device */

  config.id = entry->id;
  config.instance = entry->instance;
  config.len = entry->len;
  config.configdata = g_entryimage;
  ret = ioctl(g_fd, CFGDIOC_SETCONFIG, (unsigned long) &config);
  if (ret < 0)
    {
      entry->id = 0;
      entry->len = 0;
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: configdata_fillconfig
 ****************************************************************************/

static int configdata_fillconfig(void)
{
  FAR struct configdata_entrydesc_s *entry;
  int ret;
  int i;

  /* Create a file for each unused file structure */

  for (i = 0; i < CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES; i++)
    {
      entry = &g_entries[i];
      if (entry->id == 0)
        {
          ret = configdata_wrentry(entry);
          if (ret < 0)
            {
#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
              printf("  /dev/config full\n");
#endif
              return ERROR;
            }

#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
         printf("  Created entry %04X, %d  Len=%d\n",
                entry->id, entry->instance, entry->len);
#endif
         g_nentries++;
         g_ntotalalloc++;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: configdata_rdentry
 ****************************************************************************/

static inline int
  configdata_rdentry(FAR struct configdata_entrydesc_s *entry)
{
  struct config_data_s config;
  uint32_t crc;
  int ret;

  /* Read the config entry from /dev/config */

  config.id = entry->id;
  config.instance = entry->instance;
  config.len = entry->len;
  config.configdata = g_entryimage;
  ret = ioctl(g_fd, CFGDIOC_GETCONFIG, (unsigned long) &config);
  if (ret < 0)
    {
      return ERROR;
    }

  /* Verify the file image CRC */

  crc = crc32(g_entryimage, entry->len);
  if (crc != entry->crc)
    {
      printf("ERROR: Bad CRC: %u vs %u\n", crc, entry->crc);
      printf("  Entry id:   %04X\n", entry->id);
      printf("  Entry size: %d\n", entry->len);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: configdata_verifyconfig
 ****************************************************************************/

static int configdata_verifyconfig(void)
{
  FAR struct configdata_entrydesc_s *entry;
  int ret;
  int errcode = OK;
  int i;
  static int iteration = 0;

  /* Create a file for each unused file structure */

  iteration++;
  for (i = 0; i < CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES; i++)
    {
      entry = &g_entries[i];
      if (entry->id != 0)
        {
          g_ntests++;
          ret = configdata_rdentry(entry);
          if (ret < 0)
            {
              /* Check if this entry has been deleted */

              if (entry->deleted)
                {
                  /* Good, it wasn't found (as it shouldn't be) */

                  g_nverified++;
#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
                  printf("  Verified delete %04X, %d\n", entry->id,
                         entry->instance);
#endif
                }
              else
                {
                  printf("ERROR: Failed to read an entry: %d\n", i);
                  printf("  Entry id:   %04X\n", entry->id);
                  printf("  Entry size: %d\n", entry->len);
                  errcode = ERROR;
                }
            }
          else
            {
              /* Check if this entry has been deleted and should report an
               * error.
               */

              if (entry->deleted)
                {
                  printf("ERROR: Succesffully read a deleted entry\n");
                  printf("  Entry id:   %04X\n", entry->id);
                  printf("  Entry size: %d\n", entry->len);
                  errcode = ERROR;
                }
              else
                {
                  g_nverified++;
#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
                  printf("  Verifed entry %04X, %d\n",
                         entry->id, entry->instance);
#endif
                }
            }
        }
    }

  return errcode;
}

/****************************************************************************
 * Name: configdata_delentries
 ****************************************************************************/

static int configdata_delentries(void)
{
  FAR struct configdata_entrydesc_s *entry;
  struct config_data_s hdr;
  int ndel;
  int ret;
  int i;
  int j;

  /* Are there any files to be deleted? */

  int nentries = g_nentries - g_ndeleted;
  if (nentries < 1)
    {
      return 0;
    }

  /* Yes... How many files should we delete? */

  ndel = (rand() % nentries) + 1;

  /* Now pick which files to delete */

  for (i = 0; i < ndel; i++)
    {
      /* Guess a file index */

      int ndx = (rand() % (g_nentries - g_ndeleted));

      /* And delete the next undeleted file after that random index */

      for (j = ndx + 1; j != ndx; )
        {
          entry = &g_entries[j];
          if (entry->id && !entry->deleted)
            {
              hdr.id = entry->id;
              hdr.instance = entry->instance;
              hdr.len = 0;
              ret = ioctl(g_fd, CFGDIOC_SETCONFIG, (unsigned long) &hdr);
              if (ret < 0)
                {
                  printf("ERROR: Delete %d failed: %d\n", i + 1, errno);
                  printf("  Entry id:    %04X\n", entry->id);
                  printf("  Entry size:  %d\n", entry->len);
                  printf("  Entry index: %d\n", j);
                }
              else
                {
#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
                  printf("  Deleted entry %04X\n", entry->id);
#endif
                  entry->deleted = true;
                  g_ndeleted++;
                  g_ntotaldelete++;
                  break;
                }
            }

          /* Increment the index and test for wrap-around */

          if (++j >= CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES)
            {
              j = 0;
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Name: configdata_getnextdeleted
 ****************************************************************************/

static int configdata_getnextdeleted(void)
{
  int x;
  int nextdeleted = -1;

  /* Find next deleted entry */

  for (x = 0; x < CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES; x++)
    {
      if (g_entries[x].deleted)
        {
          nextdeleted = x;
          break;
        }
    }

  return nextdeleted;
}

/****************************************************************************
 * Name: configdata_cleardeleted
 ****************************************************************************/

static void configdata_cleardeleted(void)
{
  int nextdeleted;
  int x;

  while ((nextdeleted = configdata_getnextdeleted()) != -1)
    {
      /* Find next non-deleted entry after the deleted one */

      for (x = nextdeleted + 1;
           x < CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES;
           x++)
        {
          if (g_entries[x].id && !g_entries[x].deleted)
            {
              break;
            }
        }

      /* Test if an non-deleted entry found */

      if (x < CONFIG_EXAMPLES_CONFIGDATA_MAXENTRIES)
        {
          /* Move this entry to the deleted entry location */

#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
          printf("  Overwrite entry %d, OLD=%04X  NEW=%04X\n",
                 nextdeleted, g_entries[nextdeleted].id, g_entries[x].id);
#endif

          g_entries[nextdeleted] = g_entries[x];
          g_entries[x].id = 0;
        }
      else
        {
          /* Just remove the entry */

          g_entries[nextdeleted].id = 0;
          g_entries[nextdeleted].deleted = FALSE;
        }
    }

  g_nentries -= g_ndeleted;
  g_ndeleted = 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: configdata_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  unsigned int i;
  int ret;
  FAR struct mtd_dev_s *mtd;

  /* Seed the random number generated */

  srand(0x93846);

  /* Create and initialize a RAM MTD device instance */

#ifdef CONFIG_EXAMPLES_CONFIGDATA_ARCHINIT
  mtd = configdata_archinitialize();
#else
#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
  printf("Creating %d byte RAM drive\n", EXAMPLES_CONFIGDATA_BUFSIZE);
#endif
  mtd = rammtd_initialize(g_simflash, EXAMPLES_CONFIGDATA_BUFSIZE);
#endif
  if (!mtd)
    {
      printf("ERROR: Failed to create RAM MTD instance\n");
      fflush(stdout);
      exit(1);
    }

  /* Initialize to provide CONFIGDATA on an MTD interface */

#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
  printf("Registering /dev/config device\n");
#endif
  MTD_IOCTL(mtd, MTDIOC_BULKERASE, 0);
  ret = mtdconfig_register(mtd);
  if (ret < 0)
    {
      printf("ERROR: /dev/config registration failed: %d\n", -ret);
      fflush(stdout);
      exit(2);
    }

  /* Zero out our entry array */

  memset(g_entries, 0, sizeof(g_entries));

  /* Open the /dev/config device */

  g_fd = open("/dev/config", O_RDOK);
  if (g_fd == -1)
    {
      printf("ERROR: Failed to open /dev/config %d\n", -errno);
      fflush(stdout);
      exit(2);
    }

  /* Initialize the before memory values */

  g_mmbefore = mallinfo();

  /* Loop seveal times ... create some config data items, delete them
   * randomly, verify them randomly, add new config items.
   */

  g_ntests = g_nverified = 0;
  g_ntotaldelete = g_ntotalalloc = 0;

#if CONFIG_EXAMPLES_CONFIGDATA_NLOOPS == 0
  for (i = 0; ; i++)
#else
  for (i = 1; i <= CONFIG_EXAMPLES_CONFIGDATA_NLOOPS; i++)
#endif
    {
      /* Write config data to the /dev/config device until either (1) all of
       * the open file structures are utilized or until (2) CONFIGDATA
       * reports an error (hopefully that the /dev/config device is full)
       */

#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
      printf("\n=== FILLING %u =============================\n", i);
#endif
      configdata_fillconfig();
#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
      printf("Filled /dev/config\n");
      printf("  Number of entries: %d\n", g_nentries);
#endif

      /* Verify all files entries to FLASH */

      ret = configdata_verifyconfig();
      if (ret < 0)
        {
          printf("ERROR: Failed to verify partition\n");
          printf("  Number of entries: %d\n", g_nentries);
        }
      else
        {
#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of entries: %d\n", g_nentries);
#endif
#endif
        }

      /* Delete some entries */

#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
      printf("\n=== DELETING %u ============================\n", i);
#endif
      ret = configdata_delentries();
      if (ret < 0)
        {
          printf("ERROR: Failed to delete enries\n");
          printf("  Number of entries: %d\n", g_nentries);
          printf("  Number deleted:    %d\n", g_ndeleted);
        }
      else
        {
#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
          printf("Deleted some enries\n");
          printf("  Number of entries: %d\n", g_nentries);
          printf("  Number deleted:    %d\n", g_ndeleted);
#endif
        }

      /* Verify all files written to FLASH */

      ret = configdata_verifyconfig();
      if (ret < 0)
        {
          printf("ERROR: Failed to verify partition\n");
          printf("  Number of entries: %d\n", g_nentries);
          printf("  Number deleted:    %d\n", g_ndeleted);
        }
      else
        {
#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
#if CONFIG_EXAMPLES_CONFIGDATA_VERBOSE != 0
          printf("Verified!\n");
          printf("  Number of entries: %d\n", g_nentries);
          printf("  Number deleted:    %d\n", g_ndeleted);
#endif
#endif
        }

      /* Clear deleted entries */

      configdata_cleardeleted();

      /* Show memory usage */

#ifndef CONFIG_EXAMPLES_CONFIGDATA_SILENT
      configdata_loopmemusage();
      fflush(stdout);
#else
      if ((i % EXAMPLES_CONFIGDATA_REPORT) == 0)
        {
          printf("%u\n", i);
          fflush(stdout);
        }
#endif
    }

#if 0
  /* Delete all files then show memory usage again */

  configdata_delallfiles();
#endif

  configdata_endmemusage();
  fflush(stdout);
  return 0;
}
