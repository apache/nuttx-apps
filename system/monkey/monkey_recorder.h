/****************************************************************************
 * apps/system/monkey/monkey_recorder.h
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

#ifndef __APPS_SYSTEM_MONKEY_MONKEY_RECORDER_H
#define __APPS_SYSTEM_MONKEY_MONKEY_RECORDER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "monkey_type.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum monkey_recorder_mode_e
{
  MONKEY_RECORDER_MODE_RECORD,
  MONKEY_RECORDER_MODE_PLAYBACK
};

struct monkey_recorder_s
{
  int fd;
  enum monkey_recorder_mode_e mode;
};

enum monkey_recorder_res_e
{
  MONKEY_RECORDER_RES_OK,
  MONKEY_RECORDER_RES_END_OF_FILE,
  MONKEY_RECORDER_RES_DEV_TYPE_ERROR,
  MONKEY_RECORDER_RES_WRITE_ERROR,
  MONKEY_RECORDER_RES_READ_ERROR,
  MONKEY_RECORDER_RES_PARSER_ERROR
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: monkey_recorder_create
 ****************************************************************************/

FAR struct monkey_recorder_s *monkey_recorder_create(FAR const char *path,
                                          enum monkey_recorder_mode_e mode);

/****************************************************************************
 * Name: monkey_recorder_delete
 ****************************************************************************/

void monkey_recorder_delete(FAR struct monkey_recorder_s *recorder);

/****************************************************************************
 * Name: monkey_recorder_write
 ****************************************************************************/

enum monkey_recorder_res_e monkey_recorder_write(
                                FAR struct monkey_recorder_s *recorder,
                                FAR const struct monkey_dev_state_s *state);

/****************************************************************************
 * Name: monkey_recorder_read
 ****************************************************************************/

enum monkey_recorder_res_e monkey_recorder_read(
                                    FAR struct monkey_recorder_s *recorder,
                                    FAR struct monkey_dev_state_s *state,
                                    FAR uint32_t *time_stamp);

/****************************************************************************
 * Name: monkey_recorder_reset
 ****************************************************************************/

enum monkey_recorder_res_e monkey_recorder_reset(
                                    FAR struct monkey_recorder_s *recorder);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_MONKEY_MONKEY_RECORDER_H */
