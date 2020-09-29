
#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <cstdio>

#include <nuttx/kmalloc.h>
#include <nuttx/fs/fs.h>
#include <nuttx/arch.h>

#include <arch/irq.h>

#include "uORBDeviceNode.h"




static int uORBDeviceNode_open(FAR struct file *filep)
{
  printf("Opened uORBDeviceNode\n");
  FAR struct inode *inode = filep->f_inode;
  FAR struct uORBDeviceNode *dev = (uORBDeviceNode *) inode->i_private;

  int ret = OK;
  return ret;
}


static ssize_t uORBDeviceNode_write(FAR struct file *filep, FAR const char *buffer,
                            size_t buflen)
{

  FAR struct inode *inode = filep->f_inode;
  FAR struct uORBDeviceNode *dev = (uORBDeviceNode *) inode->i_private;

  //memcpy(&buffer, &dev->buffer, buflen);

  //printf(dev->buffer);

  //inode->i_private = *uORBDeviceNode;
  //filep->f_inode = *inode;

  return 0;
}


static const struct file_operations uorb_fops =
{
  uORBDeviceNode_open,  /* open */
  NULL, /* close */
  NULL,  /* read */
  uORBDeviceNode_write, /* write */
  NULL,         /* seek */
  NULL,         /* ioctl */
  NULL          /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  , NULL        /* unlink */
#endif
};


int uORBNodeDevice_register(FAR const char *path)
{
  printf("registered uORBDeviceNode\n");
  FAR uORBDeviceNode *uORBDev;

  /* Allocate the upper-half data structure */

  uORBDev = (FAR uORBDeviceNode *)kmm_zalloc(sizeof(uORBDeviceNode));

  if (!uORBDev)
    {
      lcderr("ERROR: Allocation failed\n");
      return -ENOMEM;
    }


  lcdinfo("Registering %s\n", path);
  return register_driver(path, &uorb_fops, 0666, uORBDev);
}
