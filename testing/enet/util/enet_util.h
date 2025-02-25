/****************************************************************************
 * apps/testing/enet/util/enet_util.h
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

#ifndef _H_CM_ENETTEST_UTIL_H
#define _H_CM_ENETTEST_UTIL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <nuttx/config.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "netutils/netlib.h"
#include <poll.h>
#include <pthread.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct enet_if
{
  int fd;
  char *if_name;
};
struct senet_testsuites_params
{
  struct enet_if enet_dev;
  int loopmax;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int enet_test_init (const char *enetif);
int enet_test_teardown (void **state);
int enet_test_setup (void **state);
void enet_loop_send_frame(void **state);

#endif /* CM_MYTEST_UTIL_H */
