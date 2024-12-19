/****************************************************************************
 * apps/include/audioutils/nxaudio.h
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

#ifndef __APPS_INCLUDE_AUDIOUTILS_NXAUDIO_H
#define __APPS_INCLUDE_AUDIOUTILS_NXAUDIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <mqueue.h>
#include <nuttx/audio/audio.h>

/****************************************************************************
 * Public Data Types
 ****************************************************************************/

struct nxaudio_s
{
  int fd;

  int abufnum;
  FAR struct ap_buffer_s **abufs;
  mqd_t mq;

  int chnum;
};

struct nxaudio_callbacks_s
{
  void CODE (*dequeue)(unsigned long arg, FAR struct ap_buffer_s *apb);
  void CODE (*complete)(unsigned long arg);
  void CODE (*user)(unsigned long arg, FAR struct audio_msg_s *msg,
                    FAR bool *running);
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

int init_nxaudio(FAR struct nxaudio_s *nxaudio,
                 int fs, int bps, int chnum);
int init_nxaudio_devname(FAR struct nxaudio_s *nxaudio,
                 int fs, int bps, int chnum,
                 const char *devname, const char *mqname);
void fin_nxaudio(FAR struct nxaudio_s *nxaudio);
int nxaudio_enqbuffer(FAR struct nxaudio_s *nxaudio,
                      FAR struct ap_buffer_s *apb);
int nxaudio_pause(FAR struct nxaudio_s *nxaudio);
int nxaudio_resume(FAR struct nxaudio_s *nxaudio);
int nxaudio_setvolume(FAR struct nxaudio_s *nxaudio, uint16_t vol);
int nxaudio_start(FAR struct nxaudio_s *nxaudio);
int nxaudio_msgloop(FAR struct nxaudio_s *nxaudio,
                    FAR struct nxaudio_callbacks_s *cbs, unsigned long arg);
int nxaudio_stop(FAR struct nxaudio_s *nxaudio);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_AUDIOUTILS_NXAUDIO_H */
