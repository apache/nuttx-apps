apps/system/zoninfo/README.txt
Author: Gregory Nutt <gnutt@nuttx.org>

This directory contains logic to create a version of the TZ/Olson database.
This base is required if localtime() support is selected via
CONFIG_LIBC_LOCALTIME.  This logic in this directory does the following:

  - It downloads the current TZ database from the IANA website
  - It downloads the current timezone tools from the same location
  - It builds the tools and constructs the binary TZ database
  - It will then, optionally, build a ROMFS filesystem image containing
    the data base.

The ROMFS filesystem image can that be mounted during the boot-up sequence
so that it is available for the localtime logic.  There are two steps to
doing this:

  - First, a ROM disk device must be created.  This is done by calling
    the function romdisk_register() as described in
    nuttx/include/nuttx/fs/ramdisk.h.  This is an OS level operation
    and must be done in the board-level logic before your appliction
    starts.

    romdisk_register() will create a block driver at /dev/ramN where N
    is the device minor number that was provdied to romdisk_regsiter.

  - The second step is to mount the file system.  This step can be
    performed either in your board configuration logic or by your
    application using the mount() interface described in
    uttx/include/sys/mount.h.

Both of these steps are shown together in the following code sample:

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/mount.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <nuttx/fs/ramdisk.h>
#include <apps/zoneinfo.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_LIBC_TZDIR
#  errror CONFIG_LIBC_TZDIR is not defined
#endif

#ifdef CONFIG_DISABLE_MOUNTPOINT
#  error "Mountpoint support is disabled"
#endif

#if CONFIG_NFILE_DESCRIPTORS < 4
#  error "Not enough file descriptors"
#endif

#ifndef CONFIG_FS_ROMFS
#  error "ROMFS support not enabled"
#endif

#define SECTORSIZE  64
#define NSECTORS(b) (((b)+SECTORSIZE-1)/SECTORSIZE)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mount_zoneinfo(int minor)
{
   char devname[32];
   int  ret;

  /* Create a RAM disk for the test */

  ret = romdisk_register(minor, romfs_zoneinfo_img,
                         NSECTORS(romfs_zoneinfo_img_len), SECTORSIZE);
  if (ret < 0)
    {
      printf("ERROR: Failed to create RAM disk\n");
      return ret;
    }

  /* Use the minor number to create a name for the ROM disk block device */

  snprintf(devname, 32, "/dev/ram%d", minor);

  /* Mount the ROMFS file system */

  printf("Mounting ROMFS filesystem at target=%s with source=%s\n",
         CONFIG_LIBC_TZDIR, devname);

  ret = mount(devname, CONFIG_LIBC_TZDIR, "romfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: Mount failed: %d\n", errno);
      return ret;
    }

  printf("TZ database mounted at %s\n", CONFIG_LIBC_TZDIR);
  return OK;
}
