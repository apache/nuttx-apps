/****************************************************************************
 * apps/system/fastboot/fastboot.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <nuttx/memoryregion.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/version.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/boardctl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/wait.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FASTBOOT_USBDEV             "/dev/fastboot"
#define FASTBOOT_BLKDEV             "/dev/%s"

#define FASTBOOT_EP_BULKIN_IDX      1
#define FASTBOOT_EP_BULKOUT_IDX     2
#define FASTBOOT_EP_RETRY_TIMES     100
#define FASTBOOT_EP_RETRY_DELAY_MS  10

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

#define fb_info(...)                syslog(LOG_INFO, ##__VA_ARGS__);
#define fb_err(...)                 syslog(LOG_ERR, ##__VA_ARGS__);

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

struct fastboot_mem_s
{
  FAR void *addr;
};

struct fastboot_file_s
{
  char path[PATH_MAX];
  off_t offset;
};

struct fastboot_ctx_s
{
  int usbdev_in;
  int usbdev_out;
  int flash_fd;
  size_t download_max;
  size_t download_size;
  size_t download_offset;
  size_t total_imgsize;
  int wait_ms;
  FAR void *download_buffer;
  FAR struct fastboot_var_s *varlist;
  CODE int (*upload_func)(FAR struct fastboot_ctx_s *context);
  struct
    {
      size_t size;
      union
        {
          struct fastboot_mem_s mem;
          struct fastboot_file_s file;
        } u;
    } upload_param;
};

struct fastboot_cmd_s
{
  FAR const char *prefix;
  CODE void (*handle)(FAR struct fastboot_ctx_s *context,
                      FAR const char *arg);
};

typedef void (*memdump_print_t)(FAR void *, FAR const char *);

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
static void fastboot_oem(FAR struct fastboot_ctx_s *context,
                         FAR const char *arg);
static void fastboot_upload(FAR struct fastboot_ctx_s *context,
                            FAR const char *arg);

static void fastboot_memdump(FAR struct fastboot_ctx_s *context,
                             FAR const char *arg);
static void fastboot_filedump(FAR struct fastboot_ctx_s *context,
                              FAR const char *arg);
#ifdef CONFIG_SYSTEM_FASTBOOTD_SHELL
static void fastboot_shell(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg);
#endif

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
  { "reboot",             fastboot_reboot           },
  { "oem",                fastboot_oem              },
  { "upload",             fastboot_upload           }
};

static const struct fastboot_cmd_s g_oem_cmd[] =
{
  { "filedump",           fastboot_filedump         },
  { "memdump",            fastboot_memdump          },
#ifdef CONFIG_SYSTEM_FASTBOOTD_SHELL
  { "shell",              fastboot_shell            },
#endif
};

#ifdef CONFIG_BOARD_MEMORY_RANGE
static const struct memory_region_s g_memory_region[] =
{
  CONFIG_BOARD_MEMORY_RANGE
};
#endif

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
                          FAR const char *fmt, ...)
{
  char reason[FASTBOOT_MSG_LEN];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(reason, sizeof(reason), fmt, ap);
  fastboot_ack(context, "FAIL", reason);
  va_end(ap);
}

static void fastboot_okay(FAR struct fastboot_ctx_s *context,
                          FAR const char *info)
{
  fastboot_ack(context, "OKAY", info);
}

static int fastboot_flash_open(FAR const char *name)
{
  int fd = open(name, O_RDWR | O_CLOEXEC);
  if (fd < 0)
    {
      fb_err("Open %s error\n", name);
      return -errno;
    }

  return fd;
}

static void fastboot_flash_close(int fd)
{
  if (fd >= 0)
    {
      fsync(fd);
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
      fb_err("Seek error:%d\n", errno);
      return -errno;
    }

  ret = fastboot_write(fd, data, size);
  if (ret < 0)
    {
      fb_err("Flash write error:%d\n", -ret);
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
      fb_err("Flash bwrite malloc fail\n");
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
  int ret;

  ret = ioctl(fd, MTDIOC_BULKERASE, 0);
  if (ret < 0)
    {
      fb_err("Erase device failed\n");
    }

  return ret < 0 ? -errno : ret;
}

static int
fastboot_flash_program(FAR struct fastboot_ctx_s *context, int fd)
{
  FAR char *chunk_ptr = context->download_buffer;
  FAR char *end_ptr = chunk_ptr + context->download_size;
  FAR struct fastboot_sparse_header_s *sparse;
  uint32_t chunk_num;
  int ret = OK;

  /* No sparse header, write flash directly */

  sparse = (FAR struct fastboot_sparse_header_s *)chunk_ptr;
  if (sparse->magic != FASTBOOT_SPARSE_MAGIC)
    {
      ret = fastboot_flash_write(fd, 0, context->download_buffer,
                                 context->download_size);
      goto end;
    }

  if (context->total_imgsize == 0)
    {
      context->total_imgsize = sparse->blk_sz * sparse->total_blks;
    }

  chunk_num = sparse->total_chunks;
  chunk_ptr += FASTBOOT_SPARSE_HEADER;

  while (chunk_ptr < end_ptr && chunk_num--)
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
                  goto end;
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
                  goto end;
                }

              context->download_offset += chunk_size;
              chunk_ptr += 4;
            }
            break;
          case FASTBOOT_CHUNK_DONT_CARE:
          case FASTBOOT_CHUNK_CRC32:
            break;
          default:
            fb_err("Error chunk type:%d, skip\n", chunk->chunk_type);
            break;
        }
    }

  if (context->download_offset < context->total_imgsize)
    {
      return 1;
    }

end:
  context->total_imgsize = 0;
  context->download_offset = 0;
  return ret;
}

static void fastboot_flash(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg)
{
  char blkdev[PATH_MAX];
  int ret;

  snprintf(blkdev, PATH_MAX, FASTBOOT_BLKDEV, arg);

  if (context->flash_fd < 0)
    {
      context->flash_fd = fastboot_flash_open(blkdev);
      if (context->flash_fd < 0)
        {
          fastboot_fail(context, "Flash open failure");
          return;
        }
    }

  ret = fastboot_flash_program(context, context->flash_fd);
  if (ret < 0)
    {
      fastboot_fail(context, "Image flash failure");
    }
  else
    {
      fastboot_okay(context, "");
    }

  if (ret <= 0)
    {
      fastboot_flash_close(context->flash_fd);
      context->flash_fd = -1;
    }
}

static void fastboot_erase(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg)
{
  char blkdev[PATH_MAX];
  int ret;
  int fd;

  snprintf(blkdev, PATH_MAX, FASTBOOT_BLKDEV, arg);
  fb_info("Erase %s\n", blkdev);

  fd = fastboot_flash_open(blkdev);
  if (fd < 0)
    {
      fastboot_fail(context, "Flash open failure");
      return;
    }

  ret = fastboot_flash_erase(fd);
  if (ret == -ENOTTY)
    {
      struct stat sb;

      ret = fstat(fd, &sb);
      if (ret >= 0)
        {
          memset(context->download_buffer, 0xff, context->download_max);

          while (sb.st_size > 0)
            {
              size_t len = MIN(sb.st_size, context->download_max);

              ret = fastboot_write(fd, context->download_buffer, len);
              if (ret < 0)
                {
                  break;
                }

              sb.st_size -= len;
            }
        }
    }

  if (ret < 0)
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
      fb_err("Reponse error [%d]\n", -ret);
      return;
    }

  download = context->download_buffer;
  context->download_size = len;

  while (len > 0)
    {
      ssize_t r = fastboot_read(context->usbdev_in,
                                download, len);
      if (r < 0)
        {
          context->download_size = 0;
          fb_err("fastboot_download usb read error\n");
          return;
        }

      len -= r;
      download += r;
    }

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
  fastboot_okay(context, "");
  boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_USER_REBOOT);
#else
  fastboot_fail(context, "Operation not supported");
#endif
}

static void fastboot_reboot_bootloader(FAR struct fastboot_ctx_s *context,
                                       FAR const char *arg)
{
#ifdef CONFIG_BOARDCTL_RESET
  fastboot_okay(context, "");
  boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_ENTER_BOOTLOADER);
#else
  fastboot_fail(context, "Operation not supported");
#endif
}

static int fastboot_memdump_upload(FAR struct fastboot_ctx_s *context)
{
  return fastboot_write(context->usbdev_out,
                        context->upload_param.u.mem.addr,
                        context->upload_param.size);
}

static void fastboot_memdump_region(memdump_print_t memprint, FAR void *priv)
{
#ifdef CONFIG_BOARD_MEMORY_RANGE
  char response[FASTBOOT_MSG_LEN - 4];
  size_t index;

  for (index = 0; index < nitems(g_memory_region); index++)
    {
      snprintf(response, sizeof(response),
               "fastboot oem memdump 0x%" PRIxPTR " 0x%" PRIxPTR "\n",
               g_memory_region[index].start,
               g_memory_region[index].end - g_memory_region[index].start);
      memprint(priv, response);
      snprintf(response, sizeof(response),
               "fastboot get_staged 0x%" PRIxPTR ".bin\n",
               g_memory_region[index].start);
      memprint(priv, response);
    }
#endif
}

static void fastboot_memdump_syslog(FAR void *priv, FAR const char *response)
{
  fb_err("    %s", response);
}

static void fastboot_memdump_response(FAR void *priv,
                                      FAR const char *response)
{
  fastboot_ack((FAR struct fastboot_ctx_s *)priv, "TEXT", response);
}

/* Usage(host):
 *   fastboot oem memdump <addr> <size>
 *
 * Example
 *   fastboot oem memdump 0x44000000 0xb6c00
 *   fastboot get_staged mem_44000000_440b6c00.bin
 */

static void fastboot_memdump(FAR struct fastboot_ctx_s *context,
                             FAR const char *arg)
{
  if (!arg ||
      sscanf(arg, "%p %zx",
             &context->upload_param.u.mem.addr,
             &context->upload_param.size) != 2)
    {
      fastboot_memdump_region(fastboot_memdump_response, context);
      fastboot_fail(context, "Invalid argument");
      return;
    }

  fb_info("Memdump Addr: %p, Size: 0x%zx\n",
          context->upload_param.u.mem.addr,
          context->upload_param.size);
  context->upload_func = fastboot_memdump_upload;
  fastboot_okay(context, "");
}

static int fastboot_filedump_upload(FAR struct fastboot_ctx_s *context)
{
  size_t size = context->upload_param.size;
  int fd;

  fd = open(context->upload_param.u.file.path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      fb_err("No such file or directory %d\n", errno);
      return -errno;
    }

  if (context->upload_param.u.file.offset &&
      lseek(fd, context->upload_param.u.file.offset,
            context->upload_param.u.file.offset > 0 ? SEEK_SET :
                                                      SEEK_END) < 0)
    {
      fb_err("Invalid argument, offset: %" PRIdOFF "\n",
             context->upload_param.u.file.offset);
      close(fd);
      return -errno;
    }

  while (size > 0)
    {
      ssize_t nread = fastboot_read(fd, context->download_buffer,
                                    MIN(size, context->download_max));
      if (nread == 0)
        {
          break;
        }
      else if (nread < 0 ||
               fastboot_write(context->usbdev_out,
                              context->download_buffer,
                              nread) < 0)
        {
          fb_err("Upload failed (%zu bytes left)\n", size);
          close(fd);
          return -errno;
        }

      size -= nread;
    }

  close(fd);
  return 0;
}

/* Usage(host):
 *   fastboot oem filedump <PATH> [<offset> <size>]
 *
 * Example
 *   a. Upload the entire file:
 *      fastboot oem filedump /dev/bootloader
 *      fastboot get_staged bl_all.bin
 *
 *   b. Upload 4096 bytes of /dev/mem from offset 2048:
 *      fastboot oem filedump /dev/mem 2048 4096
 *      fastboot get_staged bl_2048_6144.bin
 *
 *   c. Get 2048 bytes from offset -1044480
 *      fastboot oem "filedump /dev/bootloader -1044480 2048"
 *      fastboot get_staged bl_l1044480_l1042432.txt
 */

static void fastboot_filedump(FAR struct fastboot_ctx_s *context,
                              FAR const char *arg)
{
  struct stat sb;
  int ret;

  if (!arg)
    {
      fastboot_fail(context, "Invalid argument");
      return;
    }

  ret = sscanf(arg, "%s %" PRIdOFF " %zu",
               context->upload_param.u.file.path,
               &context->upload_param.u.file.offset,
               &context->upload_param.size);
  if (ret != 1 && ret != 3)
    {
      fastboot_fail(context, "Failed to parse arguments");
      return;
    }
  else if (ret == 1)
    {
      ret = stat(context->upload_param.u.file.path, &sb);
      if (ret < 0)
        {
          fastboot_fail(context, "No such file or directory");
          return;
        }

      context->upload_param.size = sb.st_size;
      context->upload_param.u.file.offset = 0;
    }

  fb_info("Filedump Path: %s, Offset: %" PRIdOFF ", Size: %zu\n",
          context->upload_param.u.file.path,
          context->upload_param.u.file.offset,
          context->upload_param.size);
  context->upload_func = fastboot_filedump_upload;
  fastboot_okay(context, "");
}

#ifdef CONFIG_SYSTEM_FASTBOOTD_SHELL
static void fastboot_shell(FAR struct fastboot_ctx_s *context,
                           FAR const char *arg)
{
  char response[FASTBOOT_MSG_LEN - 4];
  FILE *fp;
  int ret;

  fp = popen(arg, "r");
  if (fp == NULL)
    {
      fastboot_fail(context, "popen() fails %d", errno);
      return;
    }

  while (fgets(response, sizeof(response), fp))
    {
      fastboot_ack(context, "TEXT", response);
    }

  ret = pclose(fp);
  if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
    {
      fastboot_okay(context, "");
      return;
    }

  fastboot_fail(context, "error detected 0x%x %d", ret, errno);
}
#endif

static void fastboot_upload(FAR struct fastboot_ctx_s *context,
                            FAR const char *arg)
{
  char response[FASTBOOT_MSG_LEN];
  int ret;

  if (!context->upload_param.size || !context->upload_func)
    {
      fastboot_fail(context, "No data staged by the last command");
      return;
    }

  snprintf(response, FASTBOOT_MSG_LEN, "DATA%08zx",
           context->upload_param.size);

  ret = fastboot_write(context->usbdev_out, response, strlen(response));
  if (ret < 0)
    {
      fb_err("Reponse error [%d]\n", -ret);
      goto done;
    }

  ret = context->upload_func(context);
  if (ret < 0)
    {
      fb_err("Upload failed, [%d]\n", -ret);
      fastboot_fail(context, "Upload failed");
    }
  else
    {
      fastboot_okay(context, "");
    }

done:
  context->upload_param.size = 0;
  context->upload_func = NULL;
}

static void fastboot_oem(FAR struct fastboot_ctx_s *context,
                         FAR const char *arg)
{
  size_t ncmds = nitems(g_oem_cmd);
  size_t index;

  arg++;

  for (index = 0; index < ncmds; index++)
    {
      size_t len = strlen(g_oem_cmd[index].prefix);
      if (memcmp(arg, g_oem_cmd[index].prefix, len) == 0)
        {
          arg += len;
          g_oem_cmd[index].handle(context, *arg == ' ' ? ++arg : NULL);
          break;
        }
    }

  if (index == ncmds)
    {
      fastboot_fail(context, "Unknown command");
    }
}

static void fastboot_command_loop(FAR struct fastboot_ctx_s *context)
{
  if (context->wait_ms > 0)
    {
      struct pollfd fds[1];

      fds[0].fd = context->usbdev_in;
      fds[0].events = POLLIN;

      if (poll(fds, 1, context->wait_ms) <= 0)
        {
          return;
        }
    }

  while (1)
    {
      char buffer[FASTBOOT_MSG_LEN + 1];
      size_t ncmds = nitems(g_fast_cmd);
      size_t index;

      ssize_t r = fastboot_read(context->usbdev_in,
                                buffer, FASTBOOT_MSG_LEN);
      if (r < 0)
        {
          fb_err("USB read error\n");
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
  if (var == NULL)
    {
      fb_err("ERROR: Could not allocate the memory.\n");
      return;
    }

  var->name = name;
  var->string = string;
  var->data = data;
  var->next = context->varlist;
  context->varlist = var;
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

static int fastboot_open_usb(int index, int flags)
{
  int try = FASTBOOT_EP_RETRY_TIMES;
  char usbdev[32];
  int ret;

  snprintf(usbdev, sizeof(usbdev),
           "%s/ep%d", FASTBOOT_USBDEV, index);
  do
    {
      ret = open(usbdev, flags);
      if (ret >= 0)
        {
          return ret;
        }

      usleep(FASTBOOT_EP_RETRY_DELAY_MS * 1000);
    }
  while (try--);

  fb_err("open [%s] error %d\n", usbdev, errno);

  return -errno;
}

static int fastboot_usbdev_initialize(void)
{
#ifdef CONFIG_SYSTEM_FASTBOOTD_USB_BOARDCTL
  struct boardioc_usbdev_ctrl_s ctrl;
#  ifdef CONFIG_USBDEV_COMPOSITE
    uint8_t dev = BOARDIOC_USBDEV_COMPOSITE;
#  else
    uint8_t dev = BOARDIOC_USBDEV_FASTBOOT;
#  endif
  FAR void *handle;
  int ret;

  ctrl.usbdev   = dev;
  ctrl.action   = BOARDIOC_USBDEV_INITIALIZE;
  ctrl.instance = 0;
  ctrl.config   = 0;
  ctrl.handle   = NULL;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      fb_err("boardctl(BOARDIOC_USBDEV_INITIALIZE) failed: %d\n", ret);
      return ret;
    }

  ctrl.usbdev   = dev;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.config   = 0;
  ctrl.handle   = &handle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      fb_err("boardctl(BOARDIOC_USBDEV_CONNECT) failed: %d\n", ret);
      return ret;
    }
#endif /* SYSTEM_FASTBOOTD_USB_BOARDCTL */

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  struct fastboot_ctx_s context;
  FAR void *buffer = NULL;
  int ret = OK;

  if (argc > 1)
    {
      if (strcmp(argv[1], "-h") == 0)
        {
          fb_err("Usage: fastbootd [wait_ms]\n");
          fb_err("\nmemdump: \n");
          fastboot_memdump_region(fastboot_memdump_syslog, NULL);
          return 0;
        }

      context.wait_ms = atoi(argv[1]);
    }
  else
    {
      context.wait_ms = 0;
    }

  ret = fastboot_usbdev_initialize();
  if (ret < 0)
    {
      return ret;
    }

  buffer = malloc(CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX);
  if (buffer == NULL)
    {
      fb_err("ERROR: Could not allocate the memory.\n");
      return -ENOMEM;
    }

  context.usbdev_in =
      fastboot_open_usb(FASTBOOT_EP_BULKOUT_IDX, O_RDONLY | O_CLOEXEC);
  if (context.usbdev_in < 0)
    {
      ret = -errno;
      goto err_with_mem;
    }

  context.usbdev_out =
      fastboot_open_usb(FASTBOOT_EP_BULKIN_IDX, O_WRONLY | O_CLOEXEC);
  if (context.usbdev_out < 0)
    {
      ret = -errno;
      goto err_with_in;
    }

  context.varlist         = NULL;
  context.flash_fd        = -1;
  context.download_buffer = buffer;
  context.download_size   = 0;
  context.download_offset = 0;
  context.download_max    = CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX;
  context.total_imgsize   = 0;

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
