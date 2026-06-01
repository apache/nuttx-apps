/****************************************************************************
 * apps/examples/microros_sub/microros_sub_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <unistd.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

#include <system/microros_transport.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static std_msgs__msg__Int32 g_rx_msg;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void sub_callback(const void *msgin)
{
  const std_msgs__msg__Int32 *msg = (const std_msgs__msg__Int32 *)msgin;

  printf("microros_sub: got %d\n", (int)msg->data);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  rcl_subscription_t   subscriber;
  rcl_allocator_t      allocator;
  rclc_support_t       support;
  rcl_node_t           node;
  rclc_executor_t      executor;

  printf("microros_sub: starting\n");

  if (microros_transport_init() != 0)
    {
      printf("microros_sub: transport init failed\n");
      return 1;
    }

  allocator = rcl_get_default_allocator();

  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK)
    {
      printf("microros_sub: rclc_support_init failed\n");
      return 1;
    }

  if (rclc_node_init_default(&node, "nuttx_sub_node", "", &support)
      != RCL_RET_OK)
    {
      printf("microros_sub: node init failed\n");
      return 1;
    }

  if (rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "nuttx_sub") != RCL_RET_OK)
    {
      printf("microros_sub: subscription init failed\n");
      return 1;
    }

  executor = rclc_executor_get_zero_initialized_executor();
  if (rclc_executor_init(&executor, &support.context, 1, &allocator)
      != RCL_RET_OK)
    {
      printf("microros_sub: executor init failed\n");
      return 1;
    }

  if (rclc_executor_add_subscription(&executor,
                                     &subscriber,
                                     &g_rx_msg,
                                     &sub_callback,
                                     ON_NEW_DATA) != RCL_RET_OK)
    {
      printf("microros_sub: add_subscription failed\n");
      return 1;
    }

  printf("microros_sub: spinning on /nuttx_sub\n");

  rclc_executor_spin(&executor);

  rclc_executor_fini(&executor);
  rcl_subscription_fini(&subscriber, &node);
  rcl_node_fini(&node);
  rclc_support_fini(&support);

  printf("microros_sub: done\n");
  return 0;
}
