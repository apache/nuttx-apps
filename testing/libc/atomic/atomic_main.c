/****************************************************************************
 * apps/testing/libc/atomic/atomic_main.c
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
#include <stdatomic.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ATOMIC_CHECK(value, expected)                       \
if ((value) != (expected))                                  \
{                                                           \
    printf("atomic test fail,line:%d\n",__LINE__);          \
}

#define ATOMIC_TEST(type, init)                             \
{                                                           \
    atomic_##type object = init;                            \
    atomic_##type expected = 2;                             \
                                                            \
    atomic_##type old_value = atomic_fetch_add(&object, 1); \
    ATOMIC_CHECK(old_value, 1);                             \
    ATOMIC_CHECK(object, 2);                                \
                                                            \
    atomic_store(&object, 1);                               \
    ATOMIC_CHECK(object, 1);                                \
                                                            \
    old_value = atomic_load(&object);                       \
    ATOMIC_CHECK(object, 1)                                 \
                                                            \
    old_value = atomic_fetch_or(&object, 4);                \
    ATOMIC_CHECK(old_value, 1);                             \
    ATOMIC_CHECK(object, 5);                                \
                                                            \
    old_value = atomic_fetch_xor(&object, 7);               \
    ATOMIC_CHECK(old_value, 5);                             \
    ATOMIC_CHECK(object, 2);                                \
                                                            \
    old_value = atomic_fetch_and(&object, 3);               \
    ATOMIC_CHECK(old_value, 2);                             \
    ATOMIC_CHECK(object, 2);                                \
                                                            \
    old_value = atomic_exchange(&object, 5);                \
    ATOMIC_CHECK(old_value, 2);                             \
    ATOMIC_CHECK(object, 5);                                \
                                                            \
    old_value = atomic_fetch_sub(&object, 3);               \
    ATOMIC_CHECK(old_value, 5);                             \
    ATOMIC_CHECK(object, 2);                                \
                                                            \
    atomic_compare_exchange_weak(&object, &expected, 5);    \
    ATOMIC_CHECK(object, 5);                                \
                                                            \
    expected = 5;                                           \
    atomic_compare_exchange_strong(&object, &expected, 2);  \
    ATOMIC_CHECK(object, 2);                                \
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * atomic_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  ATOMIC_TEST(int, 1);
  ATOMIC_TEST(uint, 1U);
  ATOMIC_TEST(long, 1L);
  ATOMIC_TEST(ulong, 1UL);
  ATOMIC_TEST(short, 1);
  ATOMIC_TEST(ushort, 1);
  ATOMIC_TEST(char, 1);

  printf("atomic test complete!\n");
  return 0;
}
