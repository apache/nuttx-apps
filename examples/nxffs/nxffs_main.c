
#include <nuttx/config.h>

#include <sys/mount.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <crc32.h>
#include <debug.h>

#include <nuttx/mtd.h>
#include <nuttx/ioctl.h>
#include <nuttx/nxffs.h>

#if CONFIG_RAMMTD_BLOCKSIZE != 32
#  error "SetCONFIG_RAMMTD_BLOCKSIZE to 32"
#endif

static int8_t random_data[1068];
//static int8_t mtd_ram[4096]; <-- Not enough space for two file copies
static int8_t mtd_ram[4096];

static int brake_nxffs(void)
{
  FAR struct mtd_dev_s * mtd_dev;
  int ret,i;
  FILE *config;
  int32_t size;

  mtd_dev = rammtd_initialize(mtd_ram, sizeof(mtd_ram));
  if (mtd_dev == NULL)
    {
      vdbg("RAM MTD of init failed");
    }

  ret = nxffs_initialize(mtd_dev);
  if (ret < 0)
    {
      vdbg("ERROR: NXFFS initialization failed: %d\n", -ret);
      vdbg("errasing it and trying again\n");
      mtd_dev->ioctl(mtd_dev,MTDIOC_BULKERASE,0);
      ret = nxffs_initialize(mtd_dev);
      if(ret <0) return -1; // broke good
    }

  ret = mount(NULL, "/mnt/nxffs", "nxffs", 0, NULL);
  if (ret < 0)
    {
      dbg("ERROR: Failed to mount the NXFFS volume: %d\n", errno);
      dbg("%s\n\r",strerror(errno));
    }

  for(i = 0; i<10; i++)
    {
      config = fopen("/mnt/nxffs/config","w");
      ret = fwrite(random_data,sizeof(random_data),1,config);
      fclose(config);
      if (ret == 1)
        dbg("wrote all the data\r\n");
      else{
        dbg("ERROR! did not write all the data\r\n");
	sleep(1);
	exit(0);
      }
      config = fopen("/mnt/nxffs/config","r");
      size = fread(random_data,sizeof(random_data),1,config);
      if (size == 1)
        dbg("read the right amount of data\n\r");
      else{
	dbg("ERROR! did not read the right amount of data\n\r");
	sleep(1);
	exit(0);
      }
      fclose(config);
    }
  return 0;
}

int user_start(int argc, char *argv[])
{
 return brake_nxffs();
}

