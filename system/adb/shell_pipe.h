/****************************************************************************
 * apps/system/adb/shell_pipe.h
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

#include <uv.h>
#include "adb.h"

/****************************************************************************
 * Private types
 ****************************************************************************/

struct shell_pipe_s
{
  uv_poll_t handle;
  int write_fd;
  void (*close_cb)(struct shell_pipe_s *);
  void (*on_data_cb)(struct shell_pipe_s *, struct apacket_s *);
};

typedef struct shell_pipe_s shell_pipe_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int shell_pipe_setup(adb_client_t *client, shell_pipe_t *pipe);
int shell_pipe_start(shell_pipe_t *pipe,
                     void (*on_data_cb)(shell_pipe_t *, apacket *));
void shell_pipe_destroy(shell_pipe_t *pipe,
                        void (*close_cb)(shell_pipe_t *));
int shell_pipe_write(shell_pipe_t *pipe, const void *buf, size_t count);

int shell_pipe_exec(char * const argv[], shell_pipe_t *pipe,
                    void (*on_data_cb)(shell_pipe_t *, apacket *));
int shell_exec_builtin(const char *appname, FAR char *const *argv,
                       shell_pipe_t *apipe);
