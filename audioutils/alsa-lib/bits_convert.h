/****************************************************************************
 * apps/audioutils/alsa-lib/bits_convert.h
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

#ifndef __BITS_CONVERT_H
#define __BITS_CONVERT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <alsa/pcm.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct bitsconv_data
{
  FAR void *bitsconv_buf;
  int buf_size;

  snd_pcm_format_t src_format;
  snd_pcm_format_t dst_format;
  int channels;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int bitsconv_init(FAR struct bitsconv_data **bc, int channels,
                  snd_pcm_format_t src_format, snd_pcm_format_t dst_format,
                  int buf_size);

int bitsconv_process(FAR struct bitsconv_data *bc, FAR const void *in_data,
                     int in_size, FAR void **out_data, FAR int *out_size);

int bitsconv_release(FAR struct bitsconv_data *bc);

#endif /* __BITS_CONVERT_H */