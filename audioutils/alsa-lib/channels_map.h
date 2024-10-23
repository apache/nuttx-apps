/****************************************************************************
 * apps/audioutils/alsa-lib/channels_map.h
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

#ifndef __CHANNELS_MAP_H
#define __CHANNELS_MAP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <alsa/pcm.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct chmap_data
{
  FAR void *chmap_buf;
  int buf_size;

  snd_pcm_format_t format;
  int src_ch;
  int dest_ch;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int chmap_init(FAR struct chmap_data **cm, snd_pcm_format_t format,
               int src_ch, int dst_ch, int chmap_buf_size);

int chmap_process(FAR struct chmap_data *cm, FAR const void *in_data,
                  int in_size, FAR void **out_data, FAR int *out_size);

int chmap_release(FAR struct chmap_data *cm);

#endif /* __CHANNELS_MAP_H */