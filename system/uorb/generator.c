/****************************************************************************
 * apps/system/uorb/generator.c
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

#include <nuttx/signal.h>
#include <nuttx/streams.h>

#include <ctype.h>
#include <uORB/uORB.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GENERATOR_CACHE_BUFF  4096

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sensor_gen_info_s
{
  struct orb_object   obj;
  FAR FILE           *file;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_gen_should_exit = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(void)
{
  uorbinfo_raw("\n\
The tool publishes topic data via uorb.\n\
Notice:NSH_LINELEN must be set to 128 or more.\n\
\n\
generator <command> [arguments...]\n\
  Commands:\n\
\t<topics_name> The playback topic name.\n\
\t[-h       ]  Listener commands help.\n\
\t[-f <val> ]  File path to be played back(absolute path).\n\
\t[-n <val> ]  Number of playbacks(fake model), default: 1\n\
\t[-r <val> ]  The rate for playing fake data is only valid\
 when parameter 's' is used. default:10hz.\n\
\t[-s <val> ]  Playback fake data.\n\
\t[-t <val> ]  Playback topic.\n\
\t e.g.:\n\
\t\tsim - sensor_accel0:\n\
\t\t  uorb_generator -n 100 -r 5 -s -t sensor_accel0 timestamp:23191100,\
x:0.1,y:9.7,z:0.81,temperature:22.15\n\n\
\t\tsim - sensor_baro0:\n\
\t\t  uorb_generator -n 100 -r 5 -s -t sensor_baro0 timestamp:23191100,\
pressure:999.12,temperature:26.34\n\n\
\t\tfiles - sensor_accel1\n\
\t\t  uorb_generator -f /data/uorb/20240823061723/sensor_accel0.csv\
 -t sensor_accel1\
\t\t\n\
  ");
}

static void exit_handler(int signo)
{
  (void)signo;
  g_gen_should_exit = true;
}

static int get_play_orb_id(FAR const char *filter,
                           FAR struct sensor_gen_info_s *info)
{
  struct orb_object object;
  int len;
  int idx;

  if (filter == NULL)
    {
      uorbinfo_raw("The entered built-in topic name is NULL.");
      return ERROR;
    }

  len = strlen(filter);
  if (len > 0)
    {
      object.meta = orb_get_meta(filter);
      if (object.meta)
        {
          object.instance = 0;
          for (idx = 0; idx < len; idx++)
            {
              if (isdigit(filter[idx]))
                {
                  object.instance = filter[idx] - '0';
                  break;
                }
            }
        }
      else
        {
          uorbinfo_raw("The entered built-in topic name is invalid.");
          return ERROR;
        }

      info->obj = object;
      return OK;
    }

  uorbinfo_raw("The entered built-in topic name is empty.");
  return ERROR;
}

/****************************************************************************
 * Name: replay_worker
 *
 * Description:
 *   Playback data files.
 *
 * Input Parameters:
 *   sensor_gen     Generator objects.
 *
 * Returned Value:
 *   0 on success, otherwise -1
 ****************************************************************************/

static int replay_worker(FAR struct sensor_gen_info_s *sensor_gen)
{
  struct lib_meminstream_s meminstream;
  bool is_first = true;
  uint64_t last_time;
  uint64_t tmp_time;
  FAR uint8_t *data;
  FAR char *line;
  int sleep_time;
  int lastc;
  int ret;
  int fd;

  line = zalloc(GENERATOR_CACHE_BUFF);
  if (line == NULL)
    {
      return -ENOMEM;
    }

  if (fgets(line, GENERATOR_CACHE_BUFF, sensor_gen->file) != NULL)
    {
      if (strstr(line, sensor_gen->obj.meta->o_name) == NULL)
        {
          uorbinfo_raw("Topic and file do not match!");
          ret = -EINVAL;
          goto out_line;
        }
    }
  else
    {
      uorbinfo_raw("Playback file format error!");
      ret = -EINVAL;
      goto out_line;
    }

  data = zalloc(sensor_gen->obj.meta->o_size);
  if (data == NULL)
    {
      ret = -ENOMEM;
      goto out_line;
    }

  fd = orb_advertise_multi_queue_persist(sensor_gen->obj.meta,
                                         data, &sensor_gen->obj.instance, 1);
  if (fd < 0)
    {
      uorbinfo_raw("Playback orb advertise failed[%d]!", fd);
      ret = fd;
      goto out_data;
    }

  while (fgets(line, GENERATOR_CACHE_BUFF, sensor_gen->file) != NULL &&
         !g_gen_should_exit)
    {
      lib_meminstream(&meminstream, line, GENERATOR_CACHE_BUFF);
      ret = lib_bscanf(&meminstream.common, &lastc,
                       sensor_gen->obj.meta->o_format, data);
      if (ret >= 0)
        {
          tmp_time = *(uint64_t *)data;
          if (is_first)
            {
              last_time = *(uint64_t *)data;
              is_first = false;
            }
          else
            {
              sleep_time = tmp_time - last_time;
              if (sleep_time > 0)
                {
                  nxsig_usleep(sleep_time);
                }

              last_time = tmp_time;
            }

          if (OK != orb_publish(sensor_gen->obj.meta, fd, data))
            {
              uorbinfo_raw("Topic publish error!");
              ret = ERROR;
              break;
            }
        }
      else
        {
          uorbinfo_raw("[ignore] The text data for this line is wrong![%s]!",
                       line);
        }
    }

  orb_unadvertise(fd);

out_data:
  free(data);
out_line:
  free(line);
  return ret;
}

/****************************************************************************
 * Name: fake_worker
 *
 * Description:
 *    Publish fake data.
 *
 * Input Parameters:
 *   sensor_gen     Generator objects.
 *   nb_cycle       Number of publications.
 *   topic_rate     Publish frequency.
 *   buffer         String data to publish.
 *
 * Returned Value:
 *   0 on success, otherwise -1
 ****************************************************************************/

static int fake_worker(FAR struct sensor_gen_info_s *sensor_gen,
                       int nb_cycle, float topic_rate, FAR char *buffer)
{
  struct lib_meminstream_s meminstream;
  FAR uint8_t *data;
  uint64_t interval;
  int lastc;
  int fd;
  int i;

  if (buffer == NULL)
    {
      return -EINVAL;
    }

  data = malloc(sensor_gen->obj.meta->o_size);
  if (data == NULL)
    {
      return -ENOMEM;
    }

  lib_meminstream(&meminstream, buffer, strlen(buffer));
  if (lib_bscanf(&meminstream.common, &lastc,
                 sensor_gen->obj.meta->o_format, data) < 0)
    {
      uorbinfo_raw("Input string data parsing error![%s]", buffer);
      goto error;
    }

  fd = orb_advertise_multi_queue_persist(sensor_gen->obj.meta,
                                         data, &sensor_gen->obj.instance, 1);
  if (fd < 0)
    {
      uorbinfo_raw("Fake orb advertise failed[%d]!", fd);
      goto error;
    }

  interval = topic_rate ? (1000000 / topic_rate) : 500000;

  for (i = 0; i < nb_cycle && !g_gen_should_exit; ++i)
    {
      *(uint64_t *)data = orb_absolute_time();
      if (OK != orb_publish(sensor_gen->obj.meta, fd, data))
        {
          uorbinfo_raw("Topic publish error!");
          goto error_fd;
        }

      nxsig_usleep(interval);
    }

  free(data);
  return 0;

error_fd:
  orb_unadvertise(fd);
error:
  free(data);
  return -1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct sensor_gen_info_s sensor_tmp;
  float topic_rate = 0.0f;
  FAR char *filter = NULL;
  FAR char *topic  = NULL;
  FAR char *path   = NULL;
  int nb_cycle     = 1;
  bool sim         = false;
  int opt;
  int ret;

  g_gen_should_exit = false;
  if (signal(SIGINT, exit_handler) == SIG_ERR)
    {
      return 1;
    }

  while ((opt = getopt(argc, argv, "f:t:r:n:sh")) != -1)
    {
      switch (opt)
        {
          case 'f':
            path = optarg;
            break;

          case 't':
            topic = optarg;
            break;

          case 'r':
            topic_rate = atof(optarg);
            if (topic_rate < 0)
              {
                goto error;
              }
            break;

          case 'n':
            nb_cycle = strtol(optarg, NULL, 0);
            if (nb_cycle < 1)
              {
                goto error;
              }
            break;

          case 's':
            sim = true;
            break;

          case 'h':
          default:
            goto error;
        }
    }

  if (optind < argc)
    {
      filter = argv[optind];
    }

  ret = get_play_orb_id(topic, &sensor_tmp);
  if (ret < 0)
    {
      return ERROR;
    }

  if (sim)
    {
      ret = fake_worker(&sensor_tmp, nb_cycle, topic_rate, filter);
    }
  else
    {
      sensor_tmp.file = fopen(path, "r");
      if (sensor_tmp.file == NULL)
        {
          uorbinfo_raw("Failed to open file:[%s]!", path);
          return ERROR;
        }

      ret = replay_worker(&sensor_tmp);
      fclose(sensor_tmp.file);
    }

  return ret;

error:
  usage();
  return ERROR;
}
