/****************************************************************************
 * apps/testing/nettest/utils/utils.h
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

#ifndef __APPS_TESTING_NETTEST_UTILS_UTILS_H
#define __APPS_TESTING_NETTEST_UTILS_UTILS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>
#include <sys/socket.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: nettest_lo_addr
 ****************************************************************************/

#if defined (CONFIG_TESTING_NET_TCP)
socklen_t nettest_lo_addr(FAR struct sockaddr *addr, int family);

/****************************************************************************
 * Name: nettest_destroy_tcp_lo_server
 ****************************************************************************/

int nettest_destroy_tcp_lo_server(pthread_t server_tid);

/****************************************************************************
 * Name: nettest_create_tcp_lo_server
 ****************************************************************************/

pthread_t nettest_create_tcp_lo_server(void);
#endif
#endif /* __APPS_TESTING_NETTEST_UTILS_UTILS_H */
