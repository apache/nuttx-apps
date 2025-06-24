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

#include <endian.h>
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

#include <netinet/in.h>
#include <sys/boardctl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
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

/* Fastboot TCP Protocol v1
 *
 *   handshake: chars "FB" followed by a 2-digit base-10 ASCII version number
 *   data_size: 8-byte big-endian binary value before fastboot packet
 *
 *   https://android.googlesource.com/platform/system/core/+/refs/heads/main\
 *         /fastboot/README.md#tcp-protocol-v1
 */

#define FASTBOOT_TCP_HANDSHAKE      "FB01"
#define FASTBOOT_TCP_HANDSHAKE_LEN  4
#define FASTBOOT_TCP_PORT           5554

#define fb_info(...)                syslog(LOG_INFO, ##__VA_ARGS__);
#define fb_err(...)                 syslog(LOG_ERR, ##__VA_ARGS__);

/****************************************************************************
 * Private types
 ****************************************************************************/

struct fastboot_ctx_s;

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
  uint16_t minor_version;   /* (0x0) - allow images with higher minor versions */
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

struct fastboot_transport_ops_s
{
  CODE int (*init)(FAR struct fastboot_ctx_s *);
  CODE void (*deinit)(FAR struct fastboot_ctx_s *);
  CODE ssize_t (*read)(FAR struct fastboot_ctx_s *, FAR void *, size_t);
  CODE int (*write)(FAR struct fastboot_ctx_s *, FAR const void *, size_t);
};

struct fastboot_ctx_s
{
  /* Transport file descriptors
   *
   * | idx |    USB   |      TCP      | poll |
   * |-----|----------|---------------|------|
   * |   0 |usbdev in |TCP socket     |  Y   |
   * |   1 |usbdev out|accepted socket|  N   |
   */

  int tran_fd[2];
  int flash_fd;
  size_t download_max;
  size_t download_size;
  size_t download_offset;
  size_t total_imgsize;

  /* Store wait_ms argument before poll of fastboot_command_loop, and
   * TCP transport remaining data size later.
   */

  uint64_t left;
  FAR void *handle;
  FAR void *download_buffer;
  FAR struct fastboot_var_s *varlist;
  CODE int (*upload_func)(FAR struct fastboot_ctx_s *);
  FAR const struct fastboot_transport_ops_s *ops;
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
  CODE void (*handle)(FAR struct fastboot_ctx_s *, FAR const char *);
};

typedef void (*memdump_print_t)(FAR void *, FAR const char *);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void fastboot_getvar(FAR struct fastboot_ctx_s *ctx,
                            FAR const char *arg);
static void fastboot_download(FAR struct fastboot_ctx_s *ctx,
                              FAR const char *arg);
static void fastboot_erase(FAR struct fastboot_ctx_s *ctx,
                           FAR const char *arg);
static void fastboot_flash(FAR struct fastboot_ctx_s *ctx,
                           FAR const char *arg);
static void fastboot_reboot(FAR struct fastboot_ctx_s *ctx,
                            FAR const char *arg);
static void fastboot_reboot_bootloader(FAR struct fastboot_ctx_s *ctx,
                                       FAR const char *arg);
static void fastboot_oem(FAR struct fastboot_ctx_s *ctx,
                         FAR const char *arg);
static void fastboot_upload(FAR struct fastboot_ctx_s *ctx,
                            FAR const char *arg);

static void fastboot_memdump(FAR struct fastboot_ctx_s *ctx,
                             FAR const char *arg);
static void fastboot_filedump(FAR struct fastboot_ctx_s *ctx,
                              FAR const char *arg);
#ifdef CONFIG_SYSTEM_FASTBOOTD_SHELL
static void fastboot_shell(FAR struct fastboot_ctx_s *ctx,
                           FAR const char *arg);
#endif

/* USB transport */

#ifdef CONFIG_USBFASTBOOT
static int     fastboot_usbdev_initialize(FAR struct fastboot_ctx_s *ctx);
static void    fastboot_usbdev_deinit(FAR struct fastboot_ctx_s *ctx);
static ssize_t fastboot_usbdev_read(FAR struct fastboot_ctx_s *ctx,
                                    FAR void *buf, size_t len);
static int     fastboot_usbdev_write(FAR struct fastboot_ctx_s *ctx,
                                     FAR const void *buf, size_t len);
#endif

/* TCP transport */

#ifdef CONFIG_NET_TCP
static int     fastboot_tcp_initialize(FAR struct fastboot_ctx_s *ctx);
static void    fastboot_tcp_deinit(FAR struct fastboot_ctx_s *ctx);
static ssize_t fastboot_tcp_read(FAR struct fastboot_ctx_s *ctx,
                                 FAR void *buf, size_t len);
static int     fastboot_tcp_write(FAR struct fastboot_ctx_s *ctx,
                                  FAR const void *buf, size_t len);
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

static const struct fastboot_transport_ops_s g_tran_ops[] =
{
#ifdef CONFIG_USBFASTBOOT
  {
    .init    = fastboot_usbdev_initialize,
    .deinit  = fastboot_usbdev_deinit,
    .read    = fastboot_usbdev_read,
    .write   = fastboot_usbdev_write,
  },
#endif
#ifdef CONFIG_NET_TCP
  {
    .init    = fastboot_tcp_initialize,
    .deinit  = fastboot_tcp_deinit,
    .read    = fastboot_tcp_read,
    .write   = fastboot_tcp_write,
  },
#endif
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

static int fastboot_write(int fd, FAR const void *buf, size_t len)
{
  FAR const char *data = buf;

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

static void fastboot_ack(FAR struct fastboot_ctx_s *ctx,
                         FAR const char *code, FAR const char *reason)
{
  char response[FASTBOOT_MSG_LEN];

  if (reason == NULL)
    {
      reason = "";
    }

  snprintf(response, FASTBOOT_MSG_LEN, "%s%s", code, reason);
  ctx->ops->write(ctx, response, strlen(response));
}

static void fastboot_fail(FAR struct fastboot_ctx_s *ctx,
                          FAR const char *fmt, ...)
{
  char reason[FASTBOOT_MSG_LEN];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(reason, sizeof(reason), fmt, ap);
  fastboot_ack(ctx, "FAIL", reason);
  va_end(ap);
}

static void fastboot_okay(FAR struct fastboot_ctx_s *ctx,
                          FAR const char *info)
{
  fastboot_ack(ctx, "OKAY", info);
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

static int fastboot_flash_program(FAR struct fastboot_ctx_s *ctx, int fd)
{
  FAR char *chunk_ptr = ctx->download_buffer;
  FAR char *end_ptr = chunk_ptr + ctx->download_size;
  FAR struct fastboot_sparse_header_s *sparse;
  uint32_t chunk_num;
  int ret = OK;

  /* No sparse header, write flash directly */

  sparse = (FAR struct fastboot_sparse_header_s *)chunk_ptr;
  if (sparse->magic != FASTBOOT_SPARSE_MAGIC)
    {
      ret = fastboot_flash_write(fd, 0, ctx->download_buffer,
                                 ctx->download_size);
      goto end;
    }

  if (ctx->total_imgsize == 0)
    {
      ctx->total_imgsize = sparse->blk_sz * sparse->total_blks;
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
              ret = fastboot_flash_write(fd, ctx->download_offset,
                                         chunk_ptr, chunk_size);
              if (ret < 0)
                {
                  goto end;
                }

              ctx->download_offset += chunk_size;
              chunk_ptr += chunk_size;
            }
            break;
          case FASTBOOT_CHUNK_FILL:
            {
              uint32_t fill_data = be32toh(*(FAR uint32_t *)chunk_ptr);
              uint32_t chunk_size = chunk->chunk_sz * sparse->blk_sz;
              ret = ffastboot_flash_fill(fd, ctx->download_offset, fill_data,
                                         sparse->blk_sz, chunk->chunk_sz);
              if (ret < 0)
                {
                  goto end;
                }

              ctx->download_offset += chunk_size;
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

  if (ctx->download_offset < ctx->total_imgsize)
    {
      return 1;
    }

end:
  ctx->total_imgsize = 0;
  ctx->download_offset = 0;
  return ret;
}

static void fastboot_flash(FAR struct fastboot_ctx_s *ctx,
                           FAR const char *arg)
{
  char blkdev[PATH_MAX];
  int ret;

  snprintf(blkdev, PATH_MAX, FASTBOOT_BLKDEV, arg);

  if (ctx->flash_fd < 0)
    {
      ctx->flash_fd = fastboot_flash_open(blkdev);
      if (ctx->flash_fd < 0)
        {
          fastboot_fail(ctx, "Flash open failure");
          return;
        }
    }

  ret = fastboot_flash_program(ctx, ctx->flash_fd);
  if (ret < 0)
    {
      fastboot_fail(ctx, "Image flash failure");
    }
  else
    {
      fastboot_okay(ctx, "");
    }

  if (ret <= 0)
    {
      fastboot_flash_close(ctx->flash_fd);
      ctx->flash_fd = -1;
    }
}

static void fastboot_erase(FAR struct fastboot_ctx_s *ctx,
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
      fastboot_fail(ctx, "Flash open failure");
      return;
    }

  ret = fastboot_flash_erase(fd);
  if (ret == -ENOTTY)
    {
      struct stat sb;

      ret = fstat(fd, &sb);
      if (ret >= 0)
        {
          memset(ctx->download_buffer, 0xff, ctx->download_max);

          while (sb.st_size > 0)
            {
              size_t len = MIN(sb.st_size, ctx->download_max);

              ret = fastboot_write(fd, ctx->download_buffer, len);
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
      fastboot_fail(ctx, "Flash erase failure");
    }
  else
    {
      fastboot_okay(ctx, "");
    }

  fastboot_flash_close(fd);
}

static void fastboot_download(FAR struct fastboot_ctx_s *ctx,
                              FAR const char *arg)
{
  FAR char *download;
  char response[FASTBOOT_MSG_LEN];
  unsigned long len;
  int ret;

  len = strtoul(arg, NULL, 16);
  if (len > ctx->download_max)
    {
      fastboot_fail(ctx, "Data too large");
      return;
    }

  snprintf(response, FASTBOOT_MSG_LEN, "DATA%08lx", len);
  ret = ctx->ops->write(ctx, response, strlen(response));
  if (ret < 0)
    {
      fb_err("Response error [%d]\n", -ret);
      return;
    }

  download = ctx->download_buffer;
  ctx->download_size = len;

  while (len > 0)
    {
      ssize_t r = ctx->ops->read(ctx, download, len);
      if (r < 0)
        {
          if (errno == EAGAIN)
            {
              continue;
            }

          ctx->download_size = 0;
          fb_err("fastboot_download usb read error\n");
          return;
        }

      len -= r;
      download += r;
    }

  fastboot_okay(ctx, "");
}

static void fastboot_getvar(FAR struct fastboot_ctx_s *ctx,
                            FAR const char *arg)
{
  FAR struct fastboot_var_s *var;
  char buffer[FASTBOOT_MSG_LEN];

  for (var = ctx->varlist; var != NULL; var = var->next)
    {
      if (!strcmp(var->name, arg))
        {
          if (var->string == NULL)
            {
              itoa(var->data, buffer, 10);
              fastboot_okay(ctx, buffer);
            }
          else
            {
              fastboot_okay(ctx, var->string);
            }

          return;
        }
    }

  fastboot_okay(ctx, "");
}

static void fastboot_reboot(FAR struct fastboot_ctx_s *ctx,
                            FAR const char *arg)
{
#ifdef CONFIG_BOARDCTL_RESET
  fastboot_okay(ctx, "");
  boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_USER_REBOOT);
#else
  fastboot_fail(ctx, "Operation not supported");
#endif
}

static void fastboot_reboot_bootloader(FAR struct fastboot_ctx_s *ctx,
                                       FAR const char *arg)
{
#ifdef CONFIG_BOARDCTL_RESET
  fastboot_okay(ctx, "");
  boardctl(BOARDIOC_RESET, BOARDIOC_SOFTRESETCAUSE_ENTER_BOOTLOADER);
#else
  fastboot_fail(ctx, "Operation not supported");
#endif
}

static int fastboot_memdump_upload(FAR struct fastboot_ctx_s *ctx)
{
  return ctx->ops->write(ctx, ctx->upload_param.u.mem.addr,
                         ctx->upload_param.size);
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

static void fastboot_memdump(FAR struct fastboot_ctx_s *ctx,
                             FAR const char *arg)
{
  if (!arg ||
      sscanf(arg, "%p %zx", &ctx->upload_param.u.mem.addr,
             &ctx->upload_param.size) != 2)
    {
      fastboot_memdump_region(fastboot_memdump_response, ctx);
      fastboot_fail(ctx, "Invalid argument");
      return;
    }

  fb_info("Memdump Addr: %p, Size: 0x%zx\n", ctx->upload_param.u.mem.addr,
          ctx->upload_param.size);
  ctx->upload_func = fastboot_memdump_upload;
  fastboot_okay(ctx, "");
}

static int fastboot_filedump_upload(FAR struct fastboot_ctx_s *ctx)
{
  size_t size = ctx->upload_param.size;
  int fd;

  fd = open(ctx->upload_param.u.file.path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      fb_err("No such file or directory %d\n", errno);
      return -errno;
    }

  if (ctx->upload_param.u.file.offset &&
      lseek(fd, ctx->upload_param.u.file.offset,
            ctx->upload_param.u.file.offset > 0 ? SEEK_SET : SEEK_END) < 0)
    {
      fb_err("Invalid argument, offset: %" PRIdOFF "\n",
             ctx->upload_param.u.file.offset);
      close(fd);
      return -errno;
    }

  while (size > 0)
    {
      ssize_t nread = fastboot_read(fd, ctx->download_buffer,
                                    MIN(size, ctx->download_max));
      if (nread == 0)
        {
          break;
        }
      else if (nread < 0 ||
               ctx->ops->write(ctx, ctx->download_buffer, nread) < 0)
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

static void fastboot_filedump(FAR struct fastboot_ctx_s *ctx,
                              FAR const char *arg)
{
  struct stat sb;
  int ret;

  if (!arg)
    {
      fastboot_fail(ctx, "Invalid argument");
      return;
    }

  ret = sscanf(arg, "%s %" PRIdOFF " %zu", ctx->upload_param.u.file.path,
               &ctx->upload_param.u.file.offset, &ctx->upload_param.size);
  if (ret != 1 && ret != 3)
    {
      fastboot_fail(ctx, "Failed to parse arguments");
      return;
    }
  else if (ret == 1)
    {
      ret = stat(ctx->upload_param.u.file.path, &sb);
      if (ret < 0)
        {
          fastboot_fail(ctx, "No such file or directory");
          return;
        }

      ctx->upload_param.size = sb.st_size;
      ctx->upload_param.u.file.offset = 0;
    }

  fb_info("Filedump Path: %s, Offset: %" PRIdOFF ", Size: %zu\n",
          ctx->upload_param.u.file.path, ctx->upload_param.u.file.offset,
          ctx->upload_param.size);
  ctx->upload_func = fastboot_filedump_upload;
  fastboot_okay(ctx, "");
}

#ifdef CONFIG_SYSTEM_FASTBOOTD_SHELL
static void fastboot_shell(FAR struct fastboot_ctx_s *ctx,
                           FAR const char *arg)
{
  char response[FASTBOOT_MSG_LEN - 4];
  FILE *fp;
  int ret;

  fp = popen(arg, "r");
  if (fp == NULL)
    {
      fastboot_fail(ctx, "popen() fails %d", errno);
      return;
    }

  while (fgets(response, sizeof(response), fp))
    {
      fastboot_ack(ctx, "TEXT", response);
    }

  ret = pclose(fp);
  if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
    {
      fastboot_okay(ctx, "");
      return;
    }

  fastboot_fail(ctx, "error detected 0x%x %d", ret, errno);
}
#endif

static void fastboot_upload(FAR struct fastboot_ctx_s *ctx,
                            FAR const char *arg)
{
  char response[FASTBOOT_MSG_LEN];
  int ret;

  if (!ctx->upload_param.size || !ctx->upload_func)
    {
      fastboot_fail(ctx, "No data staged by the last command");
      return;
    }

  snprintf(response, FASTBOOT_MSG_LEN, "DATA%08zx", ctx->upload_param.size);

  ret = ctx->ops->write(ctx, response, strlen(response));
  if (ret < 0)
    {
      fb_err("Response error [%d]\n", -ret);
      goto done;
    }

  ret = ctx->upload_func(ctx);
  if (ret < 0)
    {
      fb_err("Upload failed, [%d]\n", -ret);
      fastboot_fail(ctx, "Upload failed");
    }
  else
    {
      fastboot_okay(ctx, "");
    }

done:
  ctx->upload_param.size = 0;
  ctx->upload_func = NULL;
}

static void fastboot_oem(FAR struct fastboot_ctx_s *ctx, FAR const char *arg)
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
          g_oem_cmd[index].handle(ctx, *arg == ' ' ? ++arg : NULL);
          break;
        }
    }

  if (index == ncmds)
    {
      fastboot_fail(ctx, "Unknown command");
    }
}

static void fastboot_command_loop(FAR struct fastboot_ctx_s *ctx,
                                  size_t nctx)
{
  FAR struct fastboot_ctx_s *c;
  struct epoll_event ev[nctx];
  int epfd;
  int n;

  epfd = epoll_create(1);
  if (epfd < 0)
    {
      fb_err("open epoll failed %d", errno);
      return;
    }

  for (c = ctx, n = nctx; n-- > 0; c++)
    {
      ev[n].events = EPOLLIN;
      ev[n].data.ptr = c;
      if (epoll_ctl(epfd, EPOLL_CTL_ADD, c->tran_fd[0], &ev[n]) < 0)
        {
          fb_err("err add poll %d", c->tran_fd[0]);
          goto epoll_close;
        }
    }

  if (ctx->left > 0)
    {
      if (epoll_wait(epfd, ev, nitems(ev), ctx->left) <= 0)
        {
          goto epoll_close;
        }
    }

  /* Reinitialize for storing TCP transport remaining data size. */

  for (c = ctx, n = nctx; n-- > 0; c++)
    {
      c->left = 0;
    }

  while (1)
    {
      char buffer[FASTBOOT_MSG_LEN + 1];
      size_t ncmds = nitems(g_fast_cmd);
      size_t index;

      n = epoll_wait(epfd, ev, nitems(ev), -1);
      for (n--; n >= 0; )
        {
          c = (FAR struct fastboot_ctx_s *)ev[n].data.ptr;
          ssize_t r = c->ops->read(c, buffer, FASTBOOT_MSG_LEN);
          if (r <= 0)
            {
              n--;
              continue;
            }

          buffer[r] = '\0';
          for (index = 0; index < ncmds; index++)
            {
              size_t len = strlen(g_fast_cmd[index].prefix);
              if (memcmp(buffer, g_fast_cmd[index].prefix, len) == 0)
                {
                  g_fast_cmd[index].handle(c, buffer + len);
                  break;
                }
            }

          if (index == ncmds)
            {
              fastboot_fail(c, "Unknown command");
            }
        }
    }

epoll_close:
  while (--c >= ctx)
    {
      epoll_ctl(epfd, EPOLL_CTL_DEL, c->tran_fd[0], NULL);
    }

  close(epfd);
}

static void fastboot_publish(FAR struct fastboot_ctx_s *ctx,
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
  var->next = ctx->varlist;
  ctx->varlist = var;
}

static void fastboot_create_publish(FAR struct fastboot_ctx_s *ctx,
                                    size_t nctx)
{
  for (; nctx-- > 0; ctx++)
    {
      fastboot_publish(ctx, "product", "NuttX", 0);
      fastboot_publish(ctx, "kernel", "NuttX", 0);
      fastboot_publish(ctx, "version", CONFIG_VERSION_STRING, 0);
      fastboot_publish(ctx, "slot-count", "1", 0);
      fastboot_publish(ctx, "max-download-size", NULL,
                       CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX);
    }
}

static void fastboot_free_publish(FAR struct fastboot_ctx_s *ctx,
                                  size_t nctx)
{
  FAR struct fastboot_var_s *next;

  for (; nctx-- > 0; ctx++)
    {
      while (ctx->varlist)
        {
          next = ctx->varlist->next;
          free(ctx->varlist);
          ctx->varlist = next;
        }
    }
}

#ifdef CONFIG_USBFASTBOOT
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

static int fastboot_usbdev_initialize(FAR struct fastboot_ctx_s *ctx)
{
#ifdef CONFIG_SYSTEM_FASTBOOTD_USB_BOARDCTL
  struct boardioc_usbdev_ctrl_s ctrl;
#  ifdef CONFIG_USBDEV_COMPOSITE
    uint8_t dev = BOARDIOC_USBDEV_COMPOSITE;
#  else
    uint8_t dev = BOARDIOC_USBDEV_FASTBOOT;
#  endif
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
  ctrl.handle   = &ctx->handle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      fb_err("boardctl(BOARDIOC_USBDEV_CONNECT) failed: %d\n", ret);
      return ret;
    }
#endif /* SYSTEM_FASTBOOTD_USB_BOARDCTL */

  ctx->tran_fd[0]  = fastboot_open_usb(FASTBOOT_EP_BULKOUT_IDX,
                                       O_RDONLY | O_CLOEXEC | O_NONBLOCK);
  if (ctx->tran_fd[0] < 0)
    {
      return ctx->tran_fd[0];
    }

  ctx->tran_fd[1] = fastboot_open_usb(FASTBOOT_EP_BULKIN_IDX,
                                      O_WRONLY | O_CLOEXEC | O_NONBLOCK);
  if (ctx->tran_fd[1] < 0)
    {
      close(ctx->tran_fd[0]);
      ctx->tran_fd[0] = -1;
      return ctx->tran_fd[1];
    }

  return 0;
}

static void fastboot_usbdev_deinit(FAR struct fastboot_ctx_s *ctx)
{
#ifdef CONFIG_SYSTEM_FASTBOOTD_USB_BOARDCTL
  struct boardioc_usbdev_ctrl_s ctrl;
#endif
  int i;

  for (i = 0; i < nitems(ctx->tran_fd); i++)
    {
      close(ctx->tran_fd[i]);
      ctx->tran_fd[i] = -1;
    }

#ifdef CONFIG_SYSTEM_FASTBOOTD_USB_BOARDCTL
  if (ctx->handle)
    {
#  ifdef CONFIG_USBDEV_COMPOSITE
      ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
#  else
      ctrl.usbdev   =  BOARDIOC_USBDEV_FASTBOOT;
#  endif
      ctrl.action   = BOARDIOC_USBDEV_DISCONNECT;
      ctrl.instance = 0;
      ctrl.config   = 0;
      ctrl.handle   = &ctx->handle;

      i = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
      if (i < 0)
        {
          fb_err("boardctl(BOARDIOC_USBDEV_DISCONNECT) failed: %d\n", i);
        }
    }
#endif /* SYSTEM_FASTBOOTD_USB_BOARDCTL */
}

static ssize_t fastboot_usbdev_read(FAR struct fastboot_ctx_s *ctx,
                                    FAR void *buf, size_t len)
{
  return fastboot_read(ctx->tran_fd[0], buf, len);
}

static int fastboot_usbdev_write(FAR struct fastboot_ctx_s *ctx,
                                 FAR const void *buf, size_t len)
{
  return fastboot_write(ctx->tran_fd[1], buf, len);
}
#endif

#ifdef CONFIG_NET_TCP
static int fastboot_tcp_initialize(FAR struct fastboot_ctx_s *ctx)
{
  struct sockaddr_in addr;

  ctx->tran_fd[0] = socket(AF_INET, SOCK_STREAM,
                           SOCK_CLOEXEC | SOCK_NONBLOCK);
  if (ctx->tran_fd[0] < 0)
    {
      fb_err("create socket failed %d", errno);
      return -errno;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(FASTBOOT_TCP_PORT);
  if (bind(ctx->tran_fd[0], (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
      fb_err("bind() failed %d", errno);
      goto error;
    }

  if (listen(ctx->tran_fd[0], 1) < 0)
    {
      fb_err("listen() failed %d", errno);
      goto error;
    }

  return 0;
error:
  close(ctx->tran_fd[0]);
  ctx->tran_fd[0] = -1;
  return -errno;
}

static void fastboot_tcp_disconn(FAR struct fastboot_ctx_s *ctx)
{
  close(ctx->tran_fd[1]);
  ctx->tran_fd[1] = -1;
}

static void fastboot_tcp_deinit(FAR struct fastboot_ctx_s *ctx)
{
  fastboot_tcp_disconn(ctx);
  close(ctx->tran_fd[0]);
  ctx->tran_fd[0] = -1;
}

static ssize_t fastboot_read_all(int fd, FAR void *buf, size_t len)
{
  size_t total = 0;
  ssize_t nread;

  while (total < len)
    {
      nread = fastboot_read(fd, buf, len);
      if (nread <= 0)
        {
          if (total == 0)
            {
              return nread;
            }

          break;
        }

      total += nread;
    }

  return total;
}

static ssize_t fastboot_tcp_read(FAR struct fastboot_ctx_s *ctx,
                                 FAR void *buf, size_t len)
{
  char handshake[FASTBOOT_TCP_HANDSHAKE_LEN];
  uint64_t data_size;
  ssize_t nread;

  if (ctx->tran_fd[1] == -1)
    {
      while (1)
        {
          /* Accept a connection, not care the address of the peer socket */

          ctx->tran_fd[1] = accept(ctx->tran_fd[0], NULL, 0);
          if (ctx->tran_fd[1] < 0)
            {
              continue;
            }

          /* Handshake */

          memset(handshake, 0, sizeof(handshake));
          if (fastboot_read_all(ctx->tran_fd[1], handshake,
                                sizeof(handshake)) != sizeof(handshake) ||
              strncmp(handshake, FASTBOOT_TCP_HANDSHAKE,
                      sizeof(handshake)) != 0 ||
              fastboot_write(ctx->tran_fd[1], handshake,
                             sizeof(handshake)) < 0)
            {
              fb_err("%s err handshake %d 0x%" PRIx32, __func__, errno,
                     *(FAR uint32_t *)handshake);
              fastboot_tcp_disconn(ctx);
              continue;
            }

          break;
        }
    }

  if (ctx->left == 0)
    {
      nread =
          fastboot_read_all(ctx->tran_fd[1], &data_size, sizeof(data_size));
      if (nread != sizeof(data_size))
        {
          /* As normal, end of file if client has closed the connection */

          if (nread != 0)
            {
              fb_err("%s err read data_size %zd %d", __func__, nread, errno);
            }

          fastboot_tcp_disconn(ctx);
          return nread;
        }

      ctx->left = be64toh(data_size);
    }

  if (len > ctx->left)
    {
      len = ctx->left;
    }

  nread = fastboot_read(ctx->tran_fd[1], buf, len);
  if (nread <= 0)
    {
      fastboot_tcp_disconn(ctx);
      ctx->left = 0;
    }
  else
    {
      ctx->left -= nread;
    }

  return nread;
}

static int fastboot_tcp_write(FAR struct fastboot_ctx_s *ctx,
                              FAR const void *buf, size_t len)
{
  uint64_t data_size = htobe64(len);
  int ret;

  ret = fastboot_write(ctx->tran_fd[1], &data_size, sizeof(data_size));
  if (ret < 0)
    {
      return ret;
    }

  return fastboot_write(ctx->tran_fd[1], buf, len);
}
#endif

static int fastboot_context_initialize(FAR struct fastboot_ctx_s *ctx,
                                       size_t nctx)
{
  int ret;

  for (; nctx-- > 0; ctx++)
    {
      ctx->download_max    = CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX;
      ctx->download_offset = 0;
      ctx->download_size   = 0;
      ctx->flash_fd        = -1;
      ctx->total_imgsize   = 0;
      ctx->varlist         = NULL;
      ctx->left            = ctx[0].left;
      ctx->ops             = &g_tran_ops[nctx];
      ctx->tran_fd[0]      = -1;
      ctx->tran_fd[1]      = -1;

      ctx->download_buffer = malloc(CONFIG_SYSTEM_FASTBOOTD_DOWNLOAD_MAX);
      if (ctx->download_buffer == NULL)
        {
          fb_err("ERROR: Could not allocate the memory.\n");
          continue;
        }

      ret = ctx->ops->init(ctx);
      if (ret < 0)
        {
          free(ctx->download_buffer);
          ctx->download_buffer = NULL;
          ctx->ops->deinit(ctx);
        }
    }

  return 0;
}

static void fastboot_context_deinit(FAR struct fastboot_ctx_s *ctx,
                                    size_t nctx)
{
  for (; nctx-- > 0; ctx++)
    {
      if (ctx->download_buffer)
        {
          ctx->ops->deinit(ctx);
          free(ctx->download_buffer);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  struct fastboot_ctx_s context[nitems(g_tran_ops)];
  int ret;

  if (argc > 1)
    {
      if (strcmp(argv[1], "-h") == 0)
        {
          fb_err("Usage: fastbootd [wait_ms]\n");
          fb_err("\nmemdump: \n");
          fastboot_memdump_region(fastboot_memdump_syslog, NULL);
          return 0;
        }

      if (sscanf(argv[1], "%" SCNu64 , &context[0].left) != 1)
        {
          return -EINVAL;
        }
    }

  ret = fastboot_context_initialize(context, nitems(context));
  if (ret < 0)
    {
      return ret;
    }

  fastboot_create_publish(context, nitems(context));
  fastboot_command_loop(context, nitems(context));
  fastboot_free_publish(context, nitems(context));
  fastboot_context_deinit(context, nitems(context));

  return ret;
}
