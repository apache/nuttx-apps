/****************************************************************************
 * apps/examples/lely_slave/sdev_slave.c
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

#include "sdev.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CO_SDEV_STRING(s) s

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Object 0x1000 sub-objects */

static const struct co_ssub g_1000_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Device type"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = CO_UNSIGNED32_MIN,
    .access = CO_ACCESS_RO,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1001 sub-objects */

static const struct co_ssub g_1001_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Error register"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED8,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u8 = CO_UNSIGNED8_MIN,
    .max.u8 = CO_UNSIGNED8_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u8 = CO_UNSIGNED8_MIN,
#endif
    .val.u8 = CO_UNSIGNED8_MIN,
    .access = CO_ACCESS_RO,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1005 sub-objects */

static const struct co_ssub g_1005_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("COB-ID SYNC message"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = 0x00000080lu,
#endif
    .val.u32 = 0x00000080lu,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1012 sub-objects */

static const struct co_ssub g_1012_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("COB-ID time stamp object"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = 0x80000100lu,
#endif
    .val.u32 = 0x80000100lu,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1017 sub-objects */

static const struct co_ssub g_1017_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Producer heartbeat time"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED16,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u16 = CO_UNSIGNED16_MIN,
    .max.u16 = CO_UNSIGNED16_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u16 = 0x0032,
#endif
    .val.u16 = 0x0032,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1018 sub-objects */

static const struct co_ssub g_1018_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Highest sub-index supported"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED8,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u8 = CO_UNSIGNED8_MIN,
    .max.u8 = CO_UNSIGNED8_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u8 = 0x04,
#endif
    .val.u8 = 0x04,
    .access = CO_ACCESS_CONST,
    .pdo_mapping = 0,
    .flags = 0
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Vendor-ID"),
#endif
    .subidx = 0x01,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = 0x00000360lu,
#endif
    .val.u32 = 0x00000360lu,
    .access = CO_ACCESS_RO,
    .pdo_mapping = 0,
    .flags = 0
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Product code"),
#endif
    .subidx = 0x02,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = CO_UNSIGNED32_MIN,
    .access = CO_ACCESS_RO,
    .pdo_mapping = 0,
    .flags = 0
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Revision number"),
#endif
    .subidx = 0x03,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = CO_UNSIGNED32_MIN,
    .access = CO_ACCESS_RO,
    .pdo_mapping = 0,
    .flags = 0
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Serial number"),
#endif
    .subidx = 0x04,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = CO_UNSIGNED32_MIN,
    .access = CO_ACCESS_RO,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1800 sub-objects */

static const struct co_ssub g_1800_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Highest sub-index supported"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED8,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u8 = CO_UNSIGNED8_MIN,
    .max.u8 = CO_UNSIGNED8_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u8 = 0x02,
#endif
    .val.u8 = 0x02,
    .access = CO_ACCESS_CONST,
    .pdo_mapping = 0,
    .flags = 0
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("COB-ID"),
#endif
    .subidx = 0x01,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = 0x00000182lu,
#endif
    .val.u32 = 0x00000182lu,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Transmission type"),
#endif
    .subidx = 0x02,
    .type = CO_DEFTYPE_UNSIGNED8,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u8 = CO_UNSIGNED8_MIN,
    .max.u8 = CO_UNSIGNED8_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u8 = 0x01,
#endif
    .val.u8 = 0x01,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1a00 sub-objects */

static const struct co_ssub g_1a00_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Number of mapped objects"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED8,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u8 = CO_UNSIGNED8_MIN,
    .max.u8 = CO_UNSIGNED8_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u8 = 0x01,
#endif
    .val.u8 = 0x01,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Mapping 1"),
#endif
    .subidx = 0x01,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = 0x21000020lu,
#endif
    .val.u32 = 0x21000020lu,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x1f80 sub-objects */

static const struct co_ssub g_1f80_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("NMT startup"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = 0x00000004lu,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
      | CO_OBJ_FLAGS_PARAMETER_VALUE
  }
};

/* Object 0x2000 sub-objects */

static const struct co_ssub g_2000_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Object with custom SDO download callback"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = CO_UNSIGNED32_MIN,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x2001 sub-objects */

static const struct co_ssub g_2001_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Object with custom SDO upload callback"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = CO_UNSIGNED32_MIN,
    .access = CO_ACCESS_RO,
    .pdo_mapping = 0,
    .flags = 0
  }
};

/* Object 0x2100 sub-objects */

static const struct co_ssub g_2100_sub[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Counter"),
#endif
    .subidx = 0x00,
    .type = CO_DEFTYPE_UNSIGNED32,
#if !LELY_NO_CO_OBJ_LIMITS
    .min.u32 = CO_UNSIGNED32_MIN,
    .max.u32 = CO_UNSIGNED32_MAX,
#endif
#if !LELY_NO_CO_OBJ_DEFAULT
    .def.u32 = CO_UNSIGNED32_MIN,
#endif
    .val.u32 = CO_UNSIGNED32_MIN,
    .access = CO_ACCESS_RW,
    .pdo_mapping = 1,
    .flags = 0
  }
};

/* Object dictionary */

static const struct co_sobj g_g_sdev_slave_objs[] =
{
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Device type"),
#endif
    .idx = 0x1000,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_1000_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Error register"),
#endif
    .idx = 0x1001,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_1001_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("COB-ID SYNC message"),
#endif
    .idx = 0x1005,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_1005_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("COB-ID time stamp object"),
#endif
    .idx = 0x1012,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_1012_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Producer heartbeat time"),
#endif
    .idx = 0x1017,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_1017_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Identity object"),
#endif
    .idx = 0x1018,
    .code = CO_OBJECT_RECORD,
    .nsub = 5,
    .subs = g_1018_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("TPDO communication parameter"),
#endif
    .idx = 0x1800,
    .code = CO_OBJECT_RECORD,
    .nsub = 3,
    .subs = g_1800_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("TPDO mapping parameter"),
#endif
    .idx = 0x1a00,
    .code = CO_OBJECT_RECORD,
    .nsub = 2,
    .subs = g_1a00_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("NMT startup"),
#endif
    .idx = 0x1f80,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_1f80_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Object with custom SDO download callback"),
#endif
    .idx = 0x2000,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_2000_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Object with custom SDO upload callback"),
#endif
    .idx = 0x2001,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_2001_sub
  },
  {
#if !LELY_NO_CO_OBJ_NAME
    .name = CO_SDEV_STRING("Counter"),
#endif
    .idx = 0x2100,
    .code = CO_OBJECT_VAR,
    .nsub = 1,
    .subs = g_2100_sub
  }
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct co_sdev g_sdev_slave =
{
  .id = 0x02,
  .name = NULL,
  .vendor_name = CO_SDEV_STRING("Apache NuttX RTOS"),
  .vendor_id = 0x00000000,
  .product_name = NULL,
  .product_code = 0x00000000,
  .revision = 0x00000000,
  .order_code = NULL,
  .baud = CO_BAUD_125,
  .rate = 0,
  .lss = 0,
  .dummy = 0x000000fe,
  .nobj = 12,
  .objs = g_g_sdev_slave_objs
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
