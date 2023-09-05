/****************************************************************************
 * apps/system/fastboot/fastboot.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/version.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/statfs.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FASTBOOT_USBDEV             "/dev/fastboot"
#define FASTBOOT_BLKDEV             "/dev/%s"

#define FASTBOOT_EP_BULKIN_IDX      0
#define FASTBOOT_EP_BULKOUT_IDX     1

#define FASTBOOT_MSG_LEN            64

#define FASTBOOT_SPARSE_MAGIC       0xed26ff3a
#define FASTBOOT_CHUNK_RAW          0xcac1
#define FASTBOOT_CHUNK_FILL         0xcac2
#define FASTBOOT_CHUNK_DONT_CARE    0xcac3
#define FASTBOOT_CHUNK_CRC32        0xcac4

#define FASTBOOT_SPARSE_HEADER      sizeof(struct fastboot_sparse_header_s)
#define FASTBOOT_CHUNK_HEADER       sizeof(struct fastboot_chunk_header_s)

#define FASTBOOT_GETUINT32(p)       (((uint32_t)(p)[3] << 24) | \
                                     ((uint32_t)(p)[2] << 16) | \
                                     ((uint32_t)(p)[1] << 8) | \
                                     (uint32_t)(p)[0])

/****************************************************************************
 * Private types
 ****************************************************************************/

struct fastboot_var_s
{
  FAR struct fastboot_var_s *next;
  FAR const char *name;
  FAR const char *string;
  int data;
};

struct fastboot_sparse_header_s
{
  uint32_t magic;           /* 0xed26ff3a */
  uint16_t major_version;   /* (0x1) - reject images with higher major versions */
  uint16_t minor_version;   /* (0x0) - allow images with higer minor versions */
  uint16_t file_hdr_sz;     /* 28 bytes for first revision of the file format */
  uint16_t chunk_hdr_sz;    /* 12 bytes for first revision of the file format */
  uint32_t blk_sz;          /* block size in bytes, must be a multiple of 4 (4096) */
  uint32_t total_blks;      /* total blocks in the non-sparse output image */
  uint32_t total_chunks;    /* total chunks in the sparse input image */
  uint32_t image_checksum;  /* CRC32 checksum of the original data, counting "don't care" */
                            /* as 0. Standard 802.3 polynomial, use a Public Domain */
                            /* table implementation */
};

struct fastboot_chunk_header_s
{
  uint16_t chunk_type;      /* 0xcac1 -> raw; 0xcac2 -> fill; 0xcac3 -> don't care */
  uint16_t reserved1;
  uint32_t chunk_sz;        /* in blocks in output image */
  uint32_t total_sz;        /* in bytes of chunk input file including chunk header and data */
};

struct fastboot_ctx_s
{
  int usbdev_in;
  int usbdev_out;
  size_t download_max;
  size_t download_size;
  size_t download_offset;
  size_t total_imgsize;
  FAR void *download_buffer;
  FAR struct fastboot_var_s *varlist;
};

struct fastboot_cmd_s
{
  FAR const char *prefix;
  CODE void (*handle)(FAR struct fastboot_ctx_s *context,
                      FAR const char *arg);
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void fastboot_getvar(FAR struct fastboot_ctx_s *context,
                            FAR const char *arg);
static void fastboot_download(FAR struct fastboot_ctx_s *context,
                              FAR const char *arg);
static void fastboot_erase(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg);
static void fastboot_flash(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg);
static void fastboot_reboot(FAR struct fastboot_ctx_s *context,
                            FAR const char *arg);
static void fastboot_reboot_bootloader(FAR struct fastboot_ctx_s *context,
                                       FAR const char *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct fastboot_cmd_s g_fast_cmd[] =
{
  { "getvar:",            fastboot_getvar           },
  { "download:",          fastboot_download         },
  { "erase:",             fastboot_erase            },
  { "flash:",             fastboot_flash            },
  { "reboot-bootloader",  fastboot_reboot_bootloader},
  { "reboot",             fastboot_reboot           }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *fastboot_memset32(FAR void *m, uint32_t val, size_t count)
{
  FAR uint32_t *buf = m;

  while (count--)
    {
      *buf++ = val;
    }

  return m;
}

static ssize_t fastboot_read(int fd, FAR void *buf, size_t len)
{
  ssize_t r = read(fd, buf, len);
  return r < 0 ? -errno : r;
}

static int fastboot_write(int fd, FAR void *buf, size_t len)
{
  FAR char *data = buf;

  while (len > 0)
    {
      ssize_t r = write(fd, data, len);
      if (r < 0)
        {
          return -errno;
        }

      data += r;
      len -= r;
    }

  return OK;
}

static void fastboot_ack(FAR struct fastboot_ctx_s *context,
                         FAR const char *code,
                         FAR const char *reason)
{
  char response[FASTBOOT_MSG_LEN];

  if (reason == NULL)
    {
      reason = "";
    }

  snprintf(response, FASTBOOT_MSG_LEN, "%s%s", code, reason);
  fastboot_write(context->usbdev_out, response, strlen(response));
}

static void fastboot_fail(FAR struct fastboot_ctx_s *context,
                          FAR const char *reason)
{
  fastboot_ack(context, "FAIL", reason);
}

static void fastboot_okay(FAR struct fastboot_ctx_s *context,
                          FAR const char *info)
{
  fastboot_ack(context, "OKAY", info);
}

static int fastboot_flash_open(FAR const char *name)
{
  int fd = open(name, O_RDWR);
  if (fd < 0)
    {
      printf("Open %s error\n", name);
      return -errno;
    }

  return fd;
}

static void fastboot_flash_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

static int fastboot_flash_write(int fd, off_t offset,
                                FAR void *data,
                                size_t size)
{
  int ret;

  offset = lseek(fd, offset, SEEK_SET);
  if (offset < 0)
    {
      printf("Seek error:%d\n", errno);
      return -errno;
    }

  ret = fastboot_write(fd, data, size);
  if (ret < 0)
    {
      printf("Flash write error:%d\n", -ret);
    }

  return ret;
}

static int ffastboot_flash_fill(int fd, off_t offset,
                                uint32_t fill_data,
                                uint32_t blk_sz,
                                uint32_t blk_num)
{
  FAR void *buffer;
  int ret = OK;
  int i;

  buffer = malloc(blk_sz);
  if (buffer == NULL)
    {
      printf("Flash bwrite malloc fail\n");
      return -ENOMEM;
    }

  fastboot_memset32(buffer, fill_data, blk_sz / 4);

  for (i = 0; i < blk_num; i++)
    {
      ret = fastboot_flash_write(fd, offset, buffer, blk_sz);
      if (ret < 0)
        {
          goto out;
        }

      offset += blk_sz;
    }

out:
  free(buffer);
  return ret;
}

static int fastboot_flash_erase(int fd)
{
  return OK;
}

static int
fastboot_flash_program(FAR struct fastboot_ctx_s *context, int fd)
{
  FAR char *chunk_ptr = context->download_buffer;
  FAR struct fastboot_sparse_header_s *sparse;
  uint32_t chunk_num;
  int ret = OK;

  /* No sparse header, write flash directly */

  sparse = (FAR struct fastboot_sparse_header_s *)chunk_ptr;
  if (sparse->magic != FASTBOOT_SPARSE_MAGIC)
    {
      return fastboot_flash_write(fd, 0, context->download_buffer,
                                  context->download_size);
    }

  if (context->total_imgsize == 0)
    {
      context->total_imgsize = sparse->blk_sz * sparse->total_blks;
    }

  chunk_num = sparse->total_chunks;

  chunk_ptr += FASTBOOT_SPARSE_HEADER;

  while (chunk_num--)
    {
      FAR struct fastboot_chunk_header_s *chunk =
        (FAR struct fastboot_chunk_header_s *)chunk_ptr;

      chunk_ptr += FASTBOOT_CHUNK_HEADER;

      switch (chunk->chunk_type)
        {
          case FASTBOOT_CHUNK_RAW:
            {
              uint32_t chunk_size = chunk->chunk_sz * sparse->blk_sz;
              ret = fastboot_flash_write(fd, context->download_offset,
                                         chunk_ptr, chunk_size);
              if (ret < 0)
                {
                  return ret;
                }

              context->download_offset += chunk_size;
              chunk_ptr += chunk_size;
            }
            break;
          case FASTBOOT_CHUNK_FILL:
            {
              uint32_t fill_data = FASTBOOT_GETUINT32(chunk_ptr);
              uint32_t chunk_size = chunk->chunk_sz * sparse->blk_sz;
              ret = ffastboot_flash_fill(fd, context->download_offset,
                                         fill_data, sparse->blk_sz,
                                         chunk->chunk_sz);
              if (ret < 0)
                {
                  return ret;
                }

              context->download_offset += chunk_size;
              chunk_ptr += 4;
            }
            break;
          case FASTBOOT_CHUNK_DONT_CARE:
          case FASTBOOT_CHUNK_CRC32:
            break;
          default:
            printf("Error chunk type:%d, skip\n", chunk->chunk_type);
            break;
        }
    }

  if (context->download_offset >= context->total_imgsize)
    {
      context->total_imgsize = 0;
      context->download_offset = 0;
    }

  return ret;
}

static void fastboot_flash(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg)
{
  char blkdev[PATH_MAX];
  int fd;

  snprintf(blkdev, PATH_MAX, FASTBOOT_BLKDEV, arg);

  fd = fastboot_flash_open(blkdev);
  if (fd < 0)
    {
      fastboot_fail(context, "Flash open failure");
      return;
    }

  if (fastboot_flash_program(context, fd) < 0)
    {
      fastboot_fail(context, "Image flash failure");
    }
  else
    {
      fastboot_okay(context, "");
    }

  fastboot_flash_close(fd);
}

static void fastboot_erase(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg)
{
  char blkdev[PATH_MAX];
  int fd;

  snprintf(blkdev, PATH_MAX, FASTBOOT_BLKDEV, arg);
  printf("Erase %s\n", blkdev);

  fd = fastboot_flash_open(blkdev);
  if (fd < 0)
    {
      fastboot_fail(context, "Flash open failure");
      return;
    }

  if (fastboot_flash_erase(fd) < 0)
    {
      fastboot_fail(context, "Flash erase failure");
    }
  else
    {
      fastboot_okay(context, "");
    }

  fastboot_flash_close(fd);
}

static void fastboot_download(FAR struct fastboot_ctx_s *context,
                              FAR const char *arg)
{
  FAR char *download;
  char response[FASTBOOT_MSG_LEN];
  unsigned long len;
  int ret;

  len = strtoul(arg, NULL, 16);
  if (len > context->download_max)
    {
      fastboot_fail(context, "Data too large");
      return;
    }

  snprintf(response, FASTBOOT_MSG_LEN, "DATA%08lx", len);
  ret = fastboot_write(context->usbdev_out, response, strlen(response));
  if (ret < 0)
    {
      printf("Reponse error [%d]\n", -ret);
      return;
    }

  download = context->download_buffer;

  while (len > 0)
    {
      ssize_t r = fastboot_read(context->usbdev_in,
                                download, len);
      if (r < 0)
        {
          printf("fastboot_download usb read error\n");
          return;
        }

      len -= r;
      download += r;
    }

  context->download_size = len;
  fastboot_okay(context, "");
}

static void fastboot_getvar(FAR struct fastboot_ctx_s *context,
                            FAR const char *arg)
{
  FAR struct fastboot_var_s *var;
  char buffer[FASTBOOT_MSG_LEN];

  for (var = context->varlist; var != NULL; var = var->next)
    {
      if (!strcmp(var->name, arg))
        {
          if (var->string == NULL)
            {
              itoa(var->data, buffer, 10);
              fastboot_okay(context, buffer);
            }
          else
            {
              fastboot_okay(context, var->string);
            }

          return;
        }
    }

  fastboot_okay(context, "");
}

static void fastboot_reboot(FAR struct fastboot_ctx_s *context,
                            FAR const char *arg)
{
#ifdef CONFIG_BOARDCTL_RESET
  boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_USER_REBOOT);
#endif
}

static void fastboot_reboot_bootloader(FAR struct fastboot_ctx_s *context,
                                       FAR const char *arg)
{
#ifdef CONFIG_BOARDCTL_RESET
  boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_ENTER_BOOTLOADER);
#endif
}

static void fastboot_command_loop(FAR struct fastboot_ctx_s *context)
{
  while (1)
    {
      char buffer[FASTBOOT_MSG_LEN];
      size_t ncmds = nitems(g_fast_cmd);
      size_t index;

      ssize_t r = fastboot_read(context->usbdev_in,
                                buffer, FASTBOOT_MSG_LEN);
      if (r < 0)
        {
          printf("USB read error\n");
          break;
        }

      buffer[r] = '\0';
      for (index = 0; index < ncmds; index++)
        {
          size_t len = strlen(g_fast_cmd[index].prefix);
          if (memcmp(buffer, g_fast_cmd[index].prefix, len) == 0)
            {
              g_fast_cmd[index].handle(context, buffer + len);
              break;
            }
        }

      if (index == ncmds)
        {
          fastboot_fail(context, "Unknown command");
        }
    }
}

static void fastboot_publish(FAR struct fastboot_ctx_s *context,
                             FAR const char *name,
                             FAR const char *string,
                             int data)
{
  FAR struct fastboot_var_s *var;

  var = malloc(sizeof(*var));
  if (var)
    {
      var->name = name;
      var->string = string;
      var->data = data;
      var->next = context->varlist;
      context->varlist = var;
    }
}

static void fastboot_create_publish(FAR struct fastboot_ctx_s *context)
{
  fastboot_publish(context, "product", "NuttX", 0);
  fastboot_publish(context, "kernel", "NuttX", 0);
  fastboot_publish(context, "version", CONFIG_VERSION_STRING, 0);
  fastboot_publish(context, "slot-count", "1", 0);
  fastboot_publish(context, "max-download-size", NULL,
                   CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX);
}

static void fastboot_free_publish(FAR struct fastboot_ctx_s *context)
{
  FAR struct fastboot_var_s *next;

  while (context->varlist)
    {
      next = context->varlist->next;
      free(context->varlist);
      context->varlist = next;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  FAR struct fastboot_ctx_s context;
  FAR void *buffer = NULL;
  char usbdev[32];
  int ret = OK;

  buffer = malloc(CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX);
  if (buffer == NULL)
    {
      printf("ERROR: Could not allocate the memory.\n");
      return -ENOMEM;
    }

  snprintf(usbdev, sizeof(usbdev), "%s/ep%d",
           FASTBOOT_USBDEV, FASTBOOT_EP_BULKOUT_IDX + 1);
  context.usbdev_in = open(usbdev, O_RDONLY);
  if (context.usbdev_in < 0)
    {
      printf("open [%s] error\n", usbdev);
      ret = -errno;
      goto err_with_mem;
    }

  snprintf(usbdev, sizeof(usbdev), "%s/ep%d",
           FASTBOOT_USBDEV, FASTBOOT_EP_BULKIN_IDX + 1);
  context.usbdev_out = open(usbdev, O_WRONLY);
  if (context.usbdev_out < 0)
    {
      printf("open [%s] error\n", usbdev);
      ret = -errno;
      goto err_with_in;
    }

  context.download_buffer = buffer;
  context.download_size   = 0;
  context.download_offset = 0;
  context.download_max    = CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX;

  fastboot_create_publish(&context);
  fastboot_command_loop(&context);
  fastboot_free_publish(&context);

  close(context.usbdev_out);
  context.usbdev_out = -1;

err_with_in:
  close(context.usbdev_in);
  context.usbdev_in = -1;

err_with_mem:
  free(buffer);
  return ret;
}
