/****************************************************************************
 * apps/testing/irtest/enum.hpp
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

#ifndef __APPS_TESTING_IRTEST_ENUM_HPP
#define __APPS_TESTING_IRTEST_ENUM_HPP

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* macro to define the enum object */

#define ENUM_START(type)                                           \
  static const enum_value g_##type##_value[] =                     \
  {

#define ENUM_VALUE(value)                                          \
    {#value, value},

#define ENUM_END(type, fmt)                                        \
    {0, 0}                                                         \
  };                                                               \
  static struct enum_type g_##type##_type =                        \
  {                                                                \
    #type, fmt, g_##type##_value                                   \
  };                                                               \

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct enum_value
{
  const char *name;
  unsigned int value;
};

struct enum_type
{
  const char *type;
  const char *fmt;
  const enum_value *value;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const struct enum_type *g_enum_table[];

#endif /* __APPS_TESTING_IRTEST_ENUM_H */
