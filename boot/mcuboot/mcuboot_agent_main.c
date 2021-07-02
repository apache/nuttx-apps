/****************************************************************************
 * apps/boot/mcuboot/mcuboot_agent_main.c
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/boardctl.h>

#include <bootutil/bootutil_public.h>

#ifdef CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_VERIFY_MD5
#include "netutils/md5.h"
#endif
#include "netutils/netlib.h"
#include "netutils/webclient.h"

#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define DL_BUFFER_SIZE  CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_BUFFER_SIZE

#define DL_UPDATE_URL   CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_UPDATE_URL

#ifdef CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_VERIFY_MD5
#  define MD5_HASH_LENGTH     32
#  define MD5_DIGEST_LENGTH   16
#  define MD5_EXPECTED_HASH   CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_MD5_HASH
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct download_context_s
{
  FAR const struct flash_area *fa;
  uint32_t                     fa_offset;
  ssize_t                      image_size;
#ifdef CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_VERIFY_MD5
  MD5_CTX                      md5_ctx;
#endif
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_iobuffer[DL_BUFFER_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: header_callback
 ****************************************************************************/

static int header_callback(FAR const char *line, bool truncated,
                           FAR void *arg)
{
  FAR struct download_context_s *ctx = (FAR struct download_context_s *)arg;

  if (ctx->image_size == -1)
    {
      char *pos = strstr(line, "Content-Length:");
      if (pos != NULL)
        {
          long i;
          int errcode;

          i = strtol(pos + sizeof("Content-Length:"), NULL, 10);
          errcode = errno;
          if (errcode == ERANGE)
            {
              fprintf(stderr, "Error reading \"Content-Length\": %s\n",
                      strerror(errcode));
            }

          printf("Firmware Update size: %ld bytes\n", i);

          ctx->image_size = i;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: sink_callback
 ****************************************************************************/

static int sink_callback(FAR char **buffer, int offset, int datend,
                         FAR int *buflen, FAR void *arg)
{
  FAR struct download_context_s *ctx = (FAR struct download_context_s *)arg;
  uint32_t length = datend - offset;

  if (length > 0)
    {
      uint32_t progress;

      flash_area_write(ctx->fa, ctx->fa_offset, &((*buffer)[offset]),
                       length);

      ctx->fa_offset += length;

#ifdef CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_VERIFY_MD5
      md5_update(&ctx->md5_ctx,
                 (FAR const unsigned char *)&((*buffer)[offset]),
                 length);
#endif

      progress = (ctx->fa_offset * 100) / ctx->image_size;

      printf("Received: %-8" PRIu32 " of %zd bytes [%" PRIu32 "%%]\n",
             ctx->fa_offset, ctx->image_size, progress);
    }

  return 0;
}

/****************************************************************************
 * Name: download_firmware_image
 ****************************************************************************/

static int download_firmware_image(FAR const char *url)
{
  int ret;
  struct webclient_context client_ctx;
  struct download_context_s dl_ctx;
#ifdef CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_VERIFY_MD5
  uint8_t digest[MD5_DIGEST_LENGTH];
  char hash[MD5_HASH_LENGTH + 1];
  int i;
#endif

  dl_ctx.fa = NULL;
  dl_ctx.fa_offset = 0;
  dl_ctx.image_size = -1;

#ifdef CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_VERIFY_MD5
  md5_init(&dl_ctx.md5_ctx);
#endif

  webclient_set_defaults(&client_ctx);
  client_ctx.method = "GET";
  client_ctx.buffer = g_iobuffer;
  client_ctx.buflen = sizeof(g_iobuffer);
  client_ctx.header_callback = header_callback;
  client_ctx.header_callback_arg = &dl_ctx;
  client_ctx.sink_callback = sink_callback;
  client_ctx.sink_callback_arg = &dl_ctx;
  client_ctx.url = url;

  ret = flash_area_open(FLASH_AREA_IMAGE_SECONDARY(0), &dl_ctx.fa);
  if (ret != OK)
    {
      fprintf(stderr, "Failed to open flash area\n");

      return ret;
    }

  ret = webclient_perform(&client_ctx);
  if (ret != OK)
    {
      fprintf(stderr, "webclient_perform failed with %d\n", ret);

      goto exit_close;
    }

#ifdef CONFIG_MCUBOOT_UPDATE_AGENT_EXAMPLE_DL_VERIFY_MD5
  md5_final(digest, &dl_ctx.md5_ctx);

  for (i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
      sprintf(&hash[i * 2], "%02x", digest[i]);
    }

  hash[MD5_HASH_LENGTH] = '\0';

  if (strncmp(hash, MD5_EXPECTED_HASH, sizeof(hash)) != 0)
    {
      fprintf(stderr, "Download checksum verification failure:\n");
      fprintf(stderr, "%s  Expected\n", MD5_EXPECTED_HASH);
      fprintf(stderr, "%s  Got\n", hash);

      ret = ERROR;
    }
  else
    {
      printf("Download checksum verification successful!\n");
    }
#endif

exit_close:
  flash_area_close(dl_ctx.fa);

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mcuboot_agent_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  FAR const char *url;

  printf("MCUboot Update Agent example\n");

  if (argc > 1)
    {
      url = argv[1];
    }
  else
    {
      url = DL_UPDATE_URL;
    }

  if (url[0] == '\0')
    {
      fprintf(stderr, "Usage: mcuboot_agent <URL>\n");

      return ERROR;
    }

  printf("Downloading from %s\n", url);

  ret = download_firmware_image(url);
  if (ret != OK)
    {
      fprintf(stderr, "Application Image download failure.\n");
      fprintf(stderr, "Update aborted.\n");

      return ERROR;
    }

  printf("Application Image successfully downloaded!\n");

  boot_set_pending_multi(0, 0);

  printf("Requested update for next boot. Restarting...\n");

  fflush(stdout);
  fflush(stderr);

  usleep(1000);

  boardctl(BOARDIOC_RESET, EXIT_SUCCESS);

  return OK;
}
