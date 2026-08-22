/****************************************************************************
 * apps/include/system/zbus_macros.h
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

/* Compact variadic macro engine used by <system/zbus.h>.  Replaces the
 * subset of Zephyr's util_macro.h needed by the zbus definition macros.
 * Supports observer lists with 0 to 16 entries per channel.  Relies on the
 * GNU ", ## __VA_ARGS__" extension (available on all NuttX toolchains).
 */

#ifndef __APPS_INCLUDE_SYSTEM_ZBUS_MACROS_H
#define __APPS_INCLUDE_SYSTEM_ZBUS_MACROS_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define _ZB_CAT(a, b)  _ZB_CAT_(a, b)
#define _ZB_CAT_(a, b) a##b

/* Empty argument list detection (P99 ISEMPTY technique).  Needed because
 * ZBUS_OBSERVERS_EMPTY expands to nothing, producing an empty-but-present
 * argument, and the GNU ", ## __VA_ARGS__" comma deletion is not reliable
 * for that case across compiler versions.  Only valid for lists of plain
 * identifiers, which is what the zbus macros take.
 */

#define _ZB_ARG18(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, \
                  a14, a15, a16, a17, a18, ...) a18
#define _ZB_HAS_COMMA(...) \
  _ZB_ARG18(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
            0)
#define _ZB_TRIGGER_PARENTHESIS_(...) ,
#define _ZB_PASTE5(a1, a2, a3, a4, a5) a1##a2##a3##a4##a5
#define _ZB_IS_EMPTY(...)                                                 \
  _ZB_IS_EMPTY_I(_ZB_HAS_COMMA(__VA_ARGS__),                              \
                 _ZB_HAS_COMMA(_ZB_TRIGGER_PARENTHESIS_ __VA_ARGS__),     \
                 _ZB_HAS_COMMA(__VA_ARGS__ ()),                           \
                 _ZB_HAS_COMMA(_ZB_TRIGGER_PARENTHESIS_ __VA_ARGS__ ()))
#define _ZB_IS_EMPTY_I(c1, c2, c3, c4) \
  _ZB_HAS_COMMA(_ZB_PASTE5(_ZB_IS_EMPTY_CASE_, c1, c2, c3, c4))
#define _ZB_IS_EMPTY_CASE_0001 ,

/* Count 1..16 variadic arguments (the list must NOT be empty; the
 * dispatchers below guarantee that).
 */

#define _ZB_NARG(...) \
  _ZB_NARG_(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, \
            4, 3, 2, 1)
#define _ZB_NARG_(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, \
                  a13, a14, a15, a16, n, ...) n

/* ZBUS_FOR_EACH(F, ...): expand F(arg) for each argument */

#define _ZB_FE1_0(f)
#define _ZB_FE1_1(f, o1) f(o1)
#define _ZB_FE1_2(f, o1, o2) f(o1) f(o2)
#define _ZB_FE1_3(f, o1, o2, o3) f(o1) f(o2) f(o3)
#define _ZB_FE1_4(f, o1, o2, o3, o4) f(o1) f(o2) f(o3) f(o4)
#define _ZB_FE1_5(f, o1, o2, o3, o4, o5) f(o1) f(o2) f(o3) f(o4) f(o5)
#define _ZB_FE1_6(f, o1, o2, o3, o4, o5, o6) \
  _ZB_FE1_5(f, o1, o2, o3, o4, o5) f(o6)
#define _ZB_FE1_7(f, o1, o2, o3, o4, o5, o6, o7) \
  _ZB_FE1_6(f, o1, o2, o3, o4, o5, o6) f(o7)
#define _ZB_FE1_8(f, o1, o2, o3, o4, o5, o6, o7, o8) \
  _ZB_FE1_7(f, o1, o2, o3, o4, o5, o6, o7) f(o8)
#define _ZB_FE1_9(f, o1, o2, o3, o4, o5, o6, o7, o8, o9) \
  _ZB_FE1_8(f, o1, o2, o3, o4, o5, o6, o7, o8) f(o9)
#define _ZB_FE1_10(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10) \
  _ZB_FE1_9(f, o1, o2, o3, o4, o5, o6, o7, o8, o9) f(o10)
#define _ZB_FE1_11(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11) \
  _ZB_FE1_10(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10) f(o11)
#define _ZB_FE1_12(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12) \
  _ZB_FE1_11(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11) f(o12)
#define _ZB_FE1_13(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, \
                   o13) \
  _ZB_FE1_12(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12) f(o13)
#define _ZB_FE1_14(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, \
                   o13, o14) \
  _ZB_FE1_13(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, o13) \
  f(o14)
#define _ZB_FE1_15(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, \
                   o13, o14, o15) \
  _ZB_FE1_14(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, o13, \
             o14) f(o15)
#define _ZB_FE1_16(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, \
                   o13, o14, o15, o16) \
  _ZB_FE1_15(f, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, o13, \
             o14, o15) f(o16)

#define _ZB_FE1_DISPATCH_1(f, ...)
#define _ZB_FE1_DISPATCH_0(f, ...) \
  _ZB_CAT(_ZB_FE1_, _ZB_NARG(__VA_ARGS__))(f, __VA_ARGS__)

#define ZBUS_FOR_EACH(f, ...) \
  _ZB_CAT(_ZB_FE1_DISPATCH_, _ZB_IS_EMPTY(__VA_ARGS__))(f, __VA_ARGS__)

/* ZBUS_OBS_FOR_EACH(F, fixed, ...): expand F(idx2, arg, fixed) for each
 * argument, where idx2 is the two-digit position of the argument in the
 * list (00, 01, ... 15).  The two-digit index is what makes the linker's
 * SORT_BY_NAME() order the channel observations by observer priority.
 */

#define _ZB_FE2_0(f, x)
#define _ZB_FE2_1(f, x, o1) f(00, o1, x)
#define _ZB_FE2_2(f, x, o1, o2) f(00, o1, x) f(01, o2, x)
#define _ZB_FE2_3(f, x, o1, o2, o3) f(00, o1, x) f(01, o2, x) f(02, o3, x)
#define _ZB_FE2_4(f, x, o1, o2, o3, o4) \
  _ZB_FE2_3(f, x, o1, o2, o3) f(03, o4, x)
#define _ZB_FE2_5(f, x, o1, o2, o3, o4, o5) \
  _ZB_FE2_4(f, x, o1, o2, o3, o4) f(04, o5, x)
#define _ZB_FE2_6(f, x, o1, o2, o3, o4, o5, o6) \
  _ZB_FE2_5(f, x, o1, o2, o3, o4, o5) f(05, o6, x)
#define _ZB_FE2_7(f, x, o1, o2, o3, o4, o5, o6, o7) \
  _ZB_FE2_6(f, x, o1, o2, o3, o4, o5, o6) f(06, o7, x)
#define _ZB_FE2_8(f, x, o1, o2, o3, o4, o5, o6, o7, o8) \
  _ZB_FE2_7(f, x, o1, o2, o3, o4, o5, o6, o7) f(07, o8, x)
#define _ZB_FE2_9(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9) \
  _ZB_FE2_8(f, x, o1, o2, o3, o4, o5, o6, o7, o8) f(08, o9, x)
#define _ZB_FE2_10(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10) \
  _ZB_FE2_9(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9) f(09, o10, x)
#define _ZB_FE2_11(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11) \
  _ZB_FE2_10(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10) f(10, o11, x)
#define _ZB_FE2_12(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, \
                   o12) \
  _ZB_FE2_11(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11) \
  f(11, o12, x)
#define _ZB_FE2_13(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, \
                   o12, o13) \
  _ZB_FE2_12(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12) \
  f(12, o13, x)
#define _ZB_FE2_14(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, \
                   o12, o13, o14) \
  _ZB_FE2_13(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, \
             o13) f(13, o14, x)
#define _ZB_FE2_15(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, \
                   o12, o13, o14, o15) \
  _ZB_FE2_14(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, \
             o13, o14) f(14, o15, x)
#define _ZB_FE2_16(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, \
                   o12, o13, o14, o15, o16) \
  _ZB_FE2_15(f, x, o1, o2, o3, o4, o5, o6, o7, o8, o9, o10, o11, o12, \
             o13, o14, o15) f(15, o16, x)

#define _ZB_FE2_DISPATCH_1(f, fixed, ...)
#define _ZB_FE2_DISPATCH_0(f, fixed, ...) \
  _ZB_CAT(_ZB_FE2_, _ZB_NARG(__VA_ARGS__))(f, fixed, __VA_ARGS__)

#define ZBUS_OBS_FOR_EACH(f, fixed, ...)                            \
  _ZB_CAT(_ZB_FE2_DISPATCH_, _ZB_IS_EMPTY(__VA_ARGS__))(f, fixed,   \
                                                        __VA_ARGS__)

#endif /* __APPS_INCLUDE_SYSTEM_ZBUS_MACROS_H */
