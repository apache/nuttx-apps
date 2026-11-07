/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/sys/iterable_sections.h
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

/* Iterable sections without a linker script.  Every entry of a type goes
 * to one section whose name is a C identifier, so the linker provides
 * __start_<name> and __stop_<name>, and keeps the section with
 * --gc-sections as long as these symbols are referenced.  This works for
 * entries defined by applications as well.  Unlike Zephyr, entries of a
 * type are in link order, not sorted by name.
 */

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SYS_ITERABLE_SECTIONS_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SYS_ITERABLE_SECTIONS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include_next <zephyr/sys/iterable_sections.h>

#undef TYPE_SECTION_ITERABLE
#undef TYPE_SECTION_START
#undef TYPE_SECTION_END

#define TYPE_SECTION_ITERABLE(type, varname, secname, section_postfix) \
  Z_DECL_ALIGN(type) varname \
  __attribute__((__section__("zblue_" STRINGIFY(secname)))) __used __noasan

#define TYPE_SECTION_START(secname) _CONCAT(__start_zblue_, secname)
#define TYPE_SECTION_END(secname)   _CONCAT(__stop_zblue_, secname)

#endif /* __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SYS_ITERABLE_SECTIONS_H */
