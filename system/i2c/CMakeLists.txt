# ##############################################################################
# apps/system/i2c/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_SYSTEM_I2CTOOL)
  set(SRCS i2c_main.c)
  list(
    APPEND
    SRCS
    i2c_bus.c
    i2c_common.c
    i2c_dev.c
    i2c_get.c
    i2c_set.c
    i2c_verf.c
    i2c_devif.c
    i2c_dump.c
    i2c_hexdump.c)
  if(CONFIG_I2C_RESET)
    list(APPEND SRCS i2c_reset.c)
  endif()
  nuttx_add_application(NAME i2c SRCS ${SRCS} STACKSIZE
                        ${CONFIG_DEFAULT_TASK_STACKSIZE})
endif()
