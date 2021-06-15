/****************************************************************************
 * apps/examples/romfs/romfs_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* Mount the ROMFS image, Verify that it contains the
 * following:
 *
 * testdir
 *  |---------- [drwxr-xr-x      4096]  adir
 *  |   |------ [-rw-r--r--        21]  anotherfile.txt
 *  |   |------ [drwxr-xr-x      4096]  subdir
 *  |   |   `-- [-rw-r--r--        21]  subdirfile.txt
 *  |   `------ [-rw-r--r--        25]  yafile.txt
 *  |---------- [-rw-r--r--        15]  afile.txt
 *  |---------- [-rw-r--r--        21]  hfile
 *  `---------- [lrwxrwxrwx        11]  ldir -> adir/subdir
 *
 * testdir/ldir is a soft-link and should not be detectable.
 * hfile is a hardlink to subdirfile and should be identical
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/boardctl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <nuttx/drivers/ramdisk.h>

#include "romfs_testdir.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration settings */

#ifndef CONFIG_EXAMPLES_ROMFS_RAMDEVNO
#  define CONFIG_EXAMPLES_ROMFS_RAMDEVNO 1
#endif

#ifndef CONFIG_EXAMPLES_ROMFS_SECTORSIZE
#  define CONFIG_EXAMPLES_ROMFS_SECTORSIZE 64
#endif

#ifndef CONFIG_EXAMPLES_ROMFS_MOUNTPOINT
#  define CONFIG_EXAMPLES_ROMFS_MOUNTPOINT "/usr/local/share"
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "Mountpoint support is disabled"
#endif

#ifndef CONFIG_FS_ROMFS
#  error "ROMFS support not enabled"
#endif

#define NSECTORS(b)        (((b)+CONFIG_EXAMPLES_ROMFS_SECTORSIZE-1)/CONFIG_EXAMPLES_ROMFS_SECTORSIZE)
#define STR_RAMDEVNO(m)    #m
#define MKMOUNT_DEVNAME(m) "/dev/ram" STR_RAMDEVNO(m)
#define MOUNT_DEVNAME      MKMOUNT_DEVNAME(CONFIG_EXAMPLES_ROMFS_RAMDEVNO)

#define SCRATCHBUFFER_SIZE 1024

/* Test directory stuff */

#define WRITABLE_MODE      (S_IWOTH|S_IWGRP|S_IWUSR)
#define READABLE_MODE      (S_IROTH|S_IRGRP|S_IRUSR)
#define EXECUTABLE_MODE    (S_IXOTH|S_IXGRP|S_IXUSR)

#define DIRECTORY_MODE     (S_IFDIR|READABLE_MODE|EXECUTABLE_MODE)
#define FILE_MODE          (S_IFREG|READABLE_MODE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct node_s
{
  struct node_s *peer;      /* Next node in this directory */
  bool           directory; /* True: directory */
  bool           found;     /* True: found and verified */
  const char    *name;      /* Node name */
  mode_t         mode;      /* Expected permissions */
  size_t         size;      /* Expected size */
  union
    {
      const char    *filecontent; /* Context of text file */
      struct node_s *child;       /* Subdirectory start */
    } u;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_afilecontent[]       = "This is a file\n";
static const char g_anotherfilecontent[] = "This is another file\n";
static const char g_yafilecontent[]      = "This is yet another file\n";
static const char g_subdirfilecontent[]  = "File in subdirectory\n";

#define g_hfilecontent g_subdirfilecontent

static struct node_s g_adir;
static struct node_s g_afile;
static struct node_s g_ldir;
static struct node_s g_hfile;

static struct node_s g_anotherfile;
static struct node_s g_subdir;
static struct node_s g_yafile;

static struct node_s g_subdirfile;

static int g_nerrors = 0;

static char g_scratchbuffer[SCRATCHBUFFER_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: connectem
 ****************************************************************************/

static void connectem(void)
{
  g_adir.peer                  = &g_afile;
  g_adir.directory             = true;
  g_adir.found                 = false;
  g_adir.name                  = "adir";
  g_adir.mode                  = DIRECTORY_MODE;
  g_adir.size                  = 0;
  g_adir.u.child               = &g_anotherfile;

  g_afile.peer                 = &g_ldir;
  g_afile.directory            = false;
  g_afile.found                = false;
  g_afile.name                 = "afile.txt";
  g_afile.mode                 = FILE_MODE;
  g_afile.size                 = strlen(g_afilecontent);
  g_afile.u.filecontent        = g_afilecontent;

  g_ldir.peer                  = &g_hfile;
  g_ldir.directory             = true;
  g_ldir.found                 = false;
  g_ldir.name                  = "ldir";
  g_ldir.mode                  = DIRECTORY_MODE;
  g_ldir.size                  = 0;
  g_ldir.u.child               = &g_subdirfile;

  g_hfile.peer                 = NULL;
  g_hfile.directory            = false; /* Actually a hard link */
  g_hfile.found                = false;
  g_hfile.name                 = "hfile";
  g_hfile.mode                 = FILE_MODE;
  g_hfile.size                 = strlen(g_hfilecontent);
  g_hfile.u.filecontent        = g_hfilecontent;

  g_anotherfile.peer           = &g_yafile;
  g_anotherfile.directory      = false;
  g_anotherfile.found          = false;
  g_anotherfile.name           = "anotherfile.txt";
  g_anotherfile.mode           = FILE_MODE;
  g_anotherfile.size           = strlen(g_anotherfilecontent);
  g_anotherfile.u.filecontent  = g_anotherfilecontent;

  g_yafile.peer                = &g_subdir;
  g_yafile.directory           = false;
  g_yafile.found               = false;
  g_yafile.name                = "yafile.txt";
  g_yafile.mode                = FILE_MODE;
  g_yafile.size                = strlen(g_yafilecontent);
  g_yafile.u.filecontent       = g_yafilecontent;

  g_subdir.peer                = NULL;
  g_subdir.directory           = true;
  g_subdir.found               = false;
  g_subdir.name                = "subdir";
  g_subdir.mode                = DIRECTORY_MODE;
  g_subdir.size                = 0;
  g_subdir.u.child             = &g_subdirfile;

  g_subdirfile.peer            = NULL;
  g_subdirfile.directory       = false;
  g_subdirfile.found           = false;
  g_subdirfile.name            = "subdirfile.txt";
  g_subdirfile.mode            = FILE_MODE;
  g_subdirfile.size            = strlen(g_subdirfilecontent);
  g_subdirfile.u.filecontent   = g_subdirfilecontent;
}

/****************************************************************************
 * Name: findindirectory
 ****************************************************************************/

static struct node_s *findindirectory(struct node_s *entry, const char *name)
{
  for (; entry; entry = entry->peer)
    {
      if (!entry->found && strcmp(entry->name, name) == 0)
        {
          entry->found = true;
          return entry;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: checkattributes
 ****************************************************************************/

static void checkattributes(const char *path, mode_t mode, size_t size)
{
  struct stat buf;
  int ret;

  ret = stat(path, &buf);
  if (ret != 0)
    {
      printf("  -- ERROR: Failed to stat %s: %d\n", path, errno);
      g_nerrors++;
      return;
    }

  if (mode != buf.st_mode)
    {
      printf("  -- ERROR: Expected mode %08x, got %08x\n", mode,
             buf.st_mode);
      g_nerrors++;
    }

  if (size != buf.st_size)
    {
      printf("  -- ERROR: Expected size %zu, got %ju\n", size,
             (uintmax_t)buf.st_size);
      g_nerrors++;
    }
}

/****************************************************************************
 * Name: checkfile
 ****************************************************************************/

static void checkfile(const char *path, struct node_s *node)
{
  ssize_t nbytesread;
  char *filedata;
  int fd;

  /* Open the file */

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      printf("  -- ERROR: Failed to open %s: %d\n", path, errno);
      g_nerrors++;
      return;
    }

  /* Read and verify the file contents */

  nbytesread = read(fd, g_scratchbuffer, SCRATCHBUFFER_SIZE);
  if (nbytesread < 0)
    {
      printf("  -- ERROR: Failed to read from %s: %d\n", path, errno);
      g_nerrors++;
    }
  else if (nbytesread != node->size)
    {
      printf("  -- ERROR: Read %ld bytes, expected %lu\n",
             (long)nbytesread, (unsigned long)node->size);
      g_nerrors++;
    }
  else if (memcmp(g_scratchbuffer, node->u.filecontent, node->size) != 0)
    {
      g_scratchbuffer[nbytesread] = '\0';
      printf("  -- ERROR: File content read does not match expectation:\n");
      printf("  --        Read:     [%s]\n", g_scratchbuffer);
      printf("  --        Expected: [%s]\n", node->u.filecontent);
      g_nerrors++;
    }

  /* Memory map and verify the file contents */

  filedata = (char *)mmap(NULL, node->size, PROT_READ, MAP_SHARED | MAP_FILE,
                          fd, 0);
  if (!filedata || filedata == (char *)MAP_FAILED)
    {
      printf("  -- ERROR: mmap of %s failed: %d\n", path, errno);
      g_nerrors++;
    }
  else
    {
      if (memcmp(filedata, node->u.filecontent, node->size) != 0)
        {
          memcpy(g_scratchbuffer, filedata, node->size);
          g_scratchbuffer[node->size] = '\0';
          printf("  -- ERROR: Mapped file content read does not match "
                 "expectation:\n");
          printf("  --        Memory:   [%s]\n", filedata);
          printf("  --        Expected: [%s]\n", node->u.filecontent);
          g_nerrors++;
        }

      munmap(filedata, node->size);
    }

  /* Close the file */

  if (close(fd) != OK)
    {
      printf("  -- ERROR: Failed to close %s: %d\n", path, errno);
      g_nerrors++;
    }
}

/****************************************************************************
 * Name: readdirectories
 ****************************************************************************/

static void readdirectories(const char *path, struct node_s *entry)
{
  DIR *dirp;
  struct node_s *node;
  struct dirent *direntry;
  char *fullpath;

  printf("Traversing directory: %s\n", path);
  dirp = opendir(path);
  if (!dirp)
    {
      printf("  ERROR opendir(\"%s\") failed: %d\n", path, errno);
      g_nerrors++;
      return;
    }

  for (direntry = readdir(dirp); direntry; direntry = readdir(dirp))
    {
      if (strcmp(direntry->d_name, ".") == 0 ||
          strcmp(direntry->d_name, "..") == 0)
        {
           printf("  Skipping %s\n", direntry->d_name);
           continue;
        }

      node = findindirectory(entry, direntry->d_name);
      if (!node)
        {
          printf("  ERROR: No node found for %s\n", direntry->d_name);
          g_nerrors++;
          continue;
        }

      /* Get the full path to the entry */

      sprintf(g_scratchbuffer, "%s/%s", path, direntry->d_name);
      fullpath = strdup(g_scratchbuffer);

      if (DIRENT_ISDIRECTORY(direntry->d_type))
        {
          printf("  DIRECTORY: %s/\n", fullpath);
          if (!node->directory)
            {
              printf("  -- ERROR: Expected type directory\n");
              g_nerrors++;
            }
          else
            {
              checkattributes(fullpath, node->mode, 0);
              readdirectories(fullpath, node->u.child);
              printf("Continuing directory: %s\n", path);
            }
        }
      else if (!DIRENT_ISLINK(direntry->d_type))
        {
          printf("  FILE: %s/\n", fullpath);
          if (node->directory)
            {
              printf("  -- ERROR: Expected type file\n");
              g_nerrors++;
            }
          else
            {
              checkattributes(fullpath, node->mode, node->size);
              checkfile(fullpath, node);
            }
        }

      free(fullpath);
    }

  closedir(dirp);
}

/****************************************************************************
 * Name: checkdirectories
 ****************************************************************************/

static void checkdirectories(struct node_s *entry)
{
  for (; entry; entry = entry->peer)
    {
      if (!entry->found)
        {
          printf("ERROR: %s never found\n", entry->name);
          g_nerrors++;
        }

      if (entry->directory)
        {
          checkdirectories(entry->u.child);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: romfs_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  struct boardioc_romdisk_s desc;

  /* Create a RAM disk for the test */

  desc.minor    = CONFIG_EXAMPLES_ROMFS_RAMDEVNO;         /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(testdir_img_len);              /* The number of sectors in the ROM disk */
  desc.sectsize = CONFIG_EXAMPLES_ROMFS_SECTORSIZE;       /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)testdir_img;             /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);

  if (ret < 0)
    {
      printf("ERROR: Failed to create RAM disk: %s\n", strerror(errno));
      return 1;
    }

  /* Mount the test file system */

  printf("Mounting ROMFS filesystem at target=%s with source=%s\n",
         CONFIG_EXAMPLES_ROMFS_MOUNTPOINT, MOUNT_DEVNAME);

  ret = mount(MOUNT_DEVNAME, CONFIG_EXAMPLES_ROMFS_MOUNTPOINT, "romfs",
              MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: Mount failed: %s\n", strerror(errno));
      return 1;
    }

  /* Perform the test */

  connectem();
  readdirectories(CONFIG_EXAMPLES_ROMFS_MOUNTPOINT, &g_adir);
  checkdirectories(&g_adir);

  if (g_nerrors)
    {
      printf("Finished with  %d errors\n", g_nerrors);
      return g_nerrors;
    }

  printf("PASSED\n");
  return 0;
}
