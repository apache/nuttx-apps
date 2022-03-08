/****************************************************************************
 * apps/examples/wgetjson/wgetjson_main.c
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

#include <stdbool.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include "netutils/netlib.h"
#include "netutils/webclient.h"
#include "netutils/cJSON.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_WGETJSON_MAXSIZE
# define CONFIG_EXAMPLES_WGETJSON_MAXSIZE 1024
#endif

#ifndef CONFIG_EXAMPLES_WGETJSON_URL
# define CONFIG_EXAMPLES_WGETJSON_URL "http://10.0.0.1/wgetjson/json_cmd.php"
#endif

#ifndef CONFIG_EXAMPLES_WGETPOST_URL
# define CONFIG_EXAMPLES_WGETPOST_URL "http://10.0.0.1/wgetjson/post_cmd.php"
#endif

#define MULTI_POST_NDATA 3

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR char *g_json_buff = NULL;
static int  g_json_bufflen   = 0;
static bool g_has_json       = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int wgetjson_postdebug_callback(FAR char **buffer, int offset,
                                       int datend, FAR int *buflen,
                                       FAR void *arg)
{
  int len = datend - offset;
  if (len <= 0)
    {
      printf("Callback No Data!\n");
      return 0;
    }

  ((*buffer)[datend]) = '\0';
  printf("Callback Data(Length:%d):\n%s\n", len, &((*buffer)[offset]));
  return 0;
}

/****************************************************************************
 * Name: wgetjson_callback
 ****************************************************************************/

static int wgetjson_callback(FAR char **buffer, int offset, int datend,
                             FAR int *buflen, FAR void *arg)
{
  FAR char *new_json_buff;
  int len = datend - offset;
  int org = len;

  if (len <= 0)
    {
      return 0;
    }

  if (!g_json_buff)
    {
      g_json_buff = malloc(len + 1);
      memcpy(g_json_buff, &((*buffer)[offset]), len);
      g_json_buff[len] = 0;
      g_json_bufflen = len;
    }
  else
    {
      if (g_json_bufflen >= CONFIG_EXAMPLES_WGETJSON_MAXSIZE)
        {
          g_json_bufflen += org;
          return 0;
        }

      if (g_json_bufflen + len > CONFIG_EXAMPLES_WGETJSON_MAXSIZE)
        {
          len = CONFIG_EXAMPLES_WGETJSON_MAXSIZE - g_json_bufflen;
        }

      new_json_buff = (FAR char *)realloc(g_json_buff,
                                          g_json_bufflen + len + 1);
      if (new_json_buff)
        {
          g_json_buff = new_json_buff;
          memcpy(&g_json_buff[g_json_bufflen], &((*buffer)[offset]),
                 len);
          g_json_buff[g_json_bufflen + len] = 0;
          g_json_bufflen += org;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: wgetjson_json_release
 ****************************************************************************/

static void wgetjson_json_release(void)
{
  if (g_json_buff)
    {
      free(g_json_buff);
      g_json_buff = NULL;
    }

  g_json_bufflen = 0;
}

/****************************************************************************
 * Name: wgetjson_doit
 ****************************************************************************/

#if 0 /* Not used */
static void wgetjson_doit(char *text)
{
  char *out;
  cJSON *json;

  json = cJSON_Parse(text);
  if (!json)
    {
      printf("Error before: [%s]\n", cJSON_GetErrorPtr());
    }
  else
    {
      out = cJSON_Print(json);
      cJSON_Delete(json);
      printf("%s\n", out);
      free(out);
    }
}
#endif

/****************************************************************************
 * Name: wgetjson_json_item_callback
 ****************************************************************************/

static int wgetjson_json_item_callback(const char *name, int type,
                                       cJSON *item)
{
  if (strlen(name) > 8 && !memcmp(name, "/(null)", 7))
    {
      name += 8;
      g_has_json = true;
    }

  if (!strcmp(name, "name"))
    {
      printf("name:\t\t\t%s\n", item->valuestring);

      /* todo something.... */
    }
  else if (strcmp(name, "format/type") == 0)
    {
      printf("format/type:\t\t%s\n", item->valuestring);

      /* todo something.... */
    }
  else if (!strcmp(name, "format/width"))
    {
      printf("format/width:\t\t%d\n", item->valueint);

      /* todo something.... */
    }
  else if (!strcmp(name, "format/height"))
    {
      printf("format/height:\t\t%d\n", item->valueint);

      /* todo something.... */
    }
  else if (!strcmp(name, "format/interlace"))
    {
      printf("format/interlace:\t%s\n",
             (item->valueint) ? "true" : "false");

      /* todo something.... */
    }
  else if (!strcmp(name, "format/frame rate"))
    {
      printf("format/frame rate:\t%d\n", item->valueint);

      /* todo something.... */
    }

  return 1;
}

/****************************************************************************
 * Name: wgetjson_json_item_scan
 ****************************************************************************/

static void wgetjson_json_item_scan(cJSON *item, const char *prefix)
{
  char *newprefix;
  int dorecurse;

  while (item)
    {
      const char *string = item->string ? item->string : "(null)";
      newprefix = malloc(strlen(prefix) + strlen(string) + 2);
      sprintf(newprefix, "%s/%s", prefix, string);

      dorecurse = wgetjson_json_item_callback(newprefix, item->type, item);
      if (item->child && dorecurse)
        {
          wgetjson_json_item_scan(item->child, newprefix);
        }

      item = item->next;
      free(newprefix);
    }
}

/****************************************************************************
 * Name: wgetjson_json_parse
 ****************************************************************************/

static int wgetjson_json_parse(char *text)
{
  cJSON *json;
  char *path = "";

  json = cJSON_Parse(text);
  if (!json)
    {
      printf("Error before: [%s]\n", cJSON_GetErrorPtr());
      return ERROR;
    }
  else
    {
      wgetjson_json_item_scan(json, path);
      cJSON_Delete(json);
      return OK;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wgetjson_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char *buffer = NULL;
  int buffer_len = 512;
  char *url = CONFIG_EXAMPLES_WGETJSON_URL;
  int ret = -1;
  int option;
  bool is_post = false;
  bool is_post_multi = false;
  bool badarg = false;
  bool is_debug = false;
  char *post_buff = NULL;
  int post_buff_len = 0;
  char *post_single_name  = "type";
  char *post_single_value = "string";
  char *post_multi_names[MULTI_POST_NDATA]  =
    {
      "name", "gender", "country"
    };

  char *post_multi_values[MULTI_POST_NDATA] =
    {
      "darcy", "man", "china"
    };

  webclient_sink_callback_t wget_cb = wgetjson_callback;

  while ((option = getopt(argc, argv, ":pPD")) != ERROR)
    {
      switch (option)
        {
          case 'p':
            is_post = true;
            break;

          case 'P':
            is_post = true;
            is_post_multi = true;
            break;

          case 'D':
            is_debug = true;
            break;

          case ':':
            badarg = true;
            break;

          case '?':
          default:
            badarg = true;
            break;
        }
    }

  if (badarg)
    {
      printf("usage: wgetjson -p(single post) -P(multi post) "
             "-D(debug wget callback)\n");
      return -1;
    }

  if (is_debug)
    {
      wget_cb = wgetjson_postdebug_callback;
    }

  if (is_post)
    {
      buffer_len = 512 * 2;
    }

  buffer = malloc(buffer_len);
  wgetjson_json_release();

  struct webclient_context ctx;
  webclient_set_defaults(&ctx);
  ctx.buffer = buffer;
  ctx.buflen = buffer_len;
  ctx.sink_callback = wget_cb;
  ctx.sink_callback_arg = NULL;
  if (is_post)
    {
      url = CONFIG_EXAMPLES_WGETPOST_URL;
      printf("URL: %s\n", url);
      if (is_post_multi)
        {
          post_buff_len = web_posts_strlen(post_multi_names,
                                           post_multi_values,
                                           MULTI_POST_NDATA);
          post_buff = malloc(post_buff_len);
          web_posts_str(post_buff, &post_buff_len, post_multi_names,
                        post_multi_values, MULTI_POST_NDATA);
        }
      else
        {
          post_buff_len = web_post_strlen(post_single_name,
                                          post_single_value);
          post_buff = malloc(post_buff_len);
          web_post_str(post_buff, &post_buff_len, post_single_name,
                       post_single_value);
        }

      if (post_buff)
        {
          const char *header = "Content-Type: "
                               "application/x-www-form-urlencoded";
          ctx.method = "POST";
          ctx.url = url;
          ctx.headers = &header;
          ctx.nheaders = 1;
          webclient_set_static_body(&ctx, post_buff, strlen(post_buff));
          ret = webclient_perform(&ctx);
        }
    }
  else
    {
      printf("URL: %s\n", url);
      ctx.method = "GET";
      ctx.url = url;
      ret = webclient_perform(&ctx);
    }

  if (ret < 0)
    {
      printf("get json size: %d\n", g_json_bufflen);
    }
  else if (!is_debug)
    {
      g_has_json = false;
      if (wgetjson_json_parse(g_json_buff) == OK && g_has_json)
        {
          printf("Parse OK\n");
        }
      else
        {
          printf("Parse error\n");
        }

      g_has_json = false;
    }

  wgetjson_json_release();
  free(buffer);
  if (post_buff)
    {
      free(post_buff);
    }

  return 0;
}
