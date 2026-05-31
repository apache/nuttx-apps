/****************************************************************************
 * apps/examples/microros_pub/microros_pub_main.c
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
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  rcl_publisher_t       publisher;
  std_msgs__msg__Int32  msg;
  rcl_allocator_t       allocator;
  rclc_support_t        support;
  rcl_node_t            node;
  int                   i;

  printf("microros_pub: starting\n");

  if (microros_transport_init() != 0)
    {
      printf("microros_pub: transport init failed\n");
      return 1;
    }

  allocator = rcl_get_default_allocator();

  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK)
    {
      printf("microros_pub: rclc_support_init failed\n");
      return 1;
    }

  if (rclc_node_init_default(&node, "nuttx_node", "", &support)
      != RCL_RET_OK)
    {
      printf("microros_pub: node init failed\n");
      return 1;
    }

  if (rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "nuttx_pub") != RCL_RET_OK)
    {
      printf("microros_pub: publisher init failed\n");
      return 1;
    }

  msg.data = 0;

  for (i = 0; i < 30; i++)
    {
      if (rcl_publish(&publisher, &msg, NULL) == RCL_RET_OK)
        {
          printf("microros_pub: sent %d\n", (int)msg.data);
        }
      else
        {
          printf("microros_pub: publish failed\n");
        }

      msg.data++;
      sleep(1);
    }

  rcl_publisher_fini(&publisher, &node);
  rcl_node_fini(&node);
  rclc_support_fini(&support);

  printf("microros_pub: done\n");
  return 0;
}
