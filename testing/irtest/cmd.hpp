/****************************************************************************
 * apps/testing/irtest/cmd.hpp
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

#ifndef __APPS_TESTING_IRTEST_CMD_HPP
#define __APPS_TESTING_IRTEST_CMD_HPP

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CMD0(func)                                         \
  static int func();                                       \
  static const arg g_##func##_args[] =                     \
  {                                                        \
    {0, 0}                                                 \
  };                                                       \
  static struct cmd g_##func##_cmd =                       \
  {                                                        \
    #func, g_##func##_args, func                           \
  };                                                       \
  static int func()

#define CMD1(func, type1, arg1)                            \
  static int func(type1 arg1);                             \
  static int func##_exec()                                 \
  {                                                        \
    type1 arg1 = get_next_arg<type1>();                    \
    return func(arg1);                                     \
  }                                                        \
  static const arg g_##func##_args[] =                     \
  {                                                        \
    {#type1, #arg1},                                       \
    {0, 0}                                                 \
  };                                                       \
  static struct cmd g_##func##_cmd =                       \
  {                                                        \
    #func, g_##func##_args, func##_exec                    \
  };                                                       \
  static int func(type1 arg1)

#define CMD2(func, type1, arg1, type2, arg2)               \
  static int func(type1 arg1, type2 arg2);                 \
  static int func##_exec()                                 \
  {                                                        \
    type1 arg1 = get_next_arg<type1>();                    \
    type2 arg2 = get_next_arg<type2>();                    \
    return func(arg1, arg2);                               \
  }                                                        \
  static const arg g_##func##_args[] = {                   \
    {#type1, #arg1},                                       \
    {#type2, #arg2},                                       \
    {0, 0}                                                 \
  };                                                       \
  static struct cmd g_##func##_cmd =                       \
  {                                                        \
    #func, g_##func##_args, func##_exec                    \
  };                                                       \
  static int func(type1 arg1, type2 arg2)

#define CMD3(func, type1, arg1, type2, arg2, type3, arg3)  \
  static int func(type1 arg1, type2 arg2, type3 arg3);     \
  static int func##_exec()                                 \
  {                                                        \
    type1 arg1 = get_next_arg<type1>();                    \
    type2 arg2 = get_next_arg<type2>();                    \
    type3 arg3 = get_next_arg<type3>();                    \
    return func(arg1, arg2, arg3);                         \
  }                                                        \
  static const arg g_##func##_args[] = {                   \
    {#type1, #arg1},                                       \
    {#type2, #arg2},                                       \
    {#type3, #arg3},                                       \
    {0, 0}                                                 \
  };                                                       \
  static struct cmd g_##func##_cmd =                       \
  {                                                        \
    #func, func##_args, func##_exec                        \
  };                                                       \
  static int func(type1 arg1, type2 arg2, type3 arg3)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct arg
{
  const char *type;
  const char *name;
};

struct cmd
{
  const char *name;
  const arg  *args;
  int (*exec)();
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const struct cmd *g_cmd_table[];

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/* the simple command line parser */

static inline const char *get_first_arg(char *cmdline)
{
  return strtok(cmdline, " \t,()\n") ?: "";
}

/* for enumerate type */

template < typename T >
inline T get_next_arg()
{
  return static_cast < T > (get_next_arg < unsigned int > ());
}

template < >
inline const char *get_next_arg()
{
  /* 0 mean from the end of last token */

  return get_first_arg(0);
}

template < >
inline int get_next_arg()
{
  return strtol(get_next_arg < const char * > (), 0, 0);
}

template < >
inline unsigned int get_next_arg()
{
  return strtoul(get_next_arg < const char * > (), 0, 0);
}

template < >
inline float get_next_arg()
{
  return atof(get_next_arg < const char * > ());
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void init_device();

#endif /* __APPS_TESTING_IRTEST_CMD_H */
