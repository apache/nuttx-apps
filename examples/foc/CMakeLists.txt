# ##############################################################################
# apps/examples/foc/CMakeLists.txt
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

if(CONFIG_EXAMPLES_FOC)
  nuttx_add_application(
    NAME
    foc
    SRCS
    foc_main.c
    STACKSIZE
    ${CONFIG_EXAMPLES_FOC_STACKSIZE}
    PRIORITY
    ${CONFIG_EXAMPLES_FOC_PRIORITY})

  set(CSRCS foc_device.c foc_mq.c foc_thr.c foc_intf.c)

  if(CONFIG_BUILTIN)
    list(APPEND CSRCS foc_parseargs.c)
  endif()

  # fixed16 support

  if(CONFIG_INDUSTRY_FOC_FIXED16)
    list(APPEND CSRCS foc_fixed16_thr.c foc_motor_b16.c)
  endif()

  # float32 support

  if(CONFIG_INDUSTRY_FOC_FLOAT)
    list(APPEND CSRCS foc_float_thr.c foc_motor_f32.c)
  endif()

  # NxScope support

  if(CONFIG_EXAMPLES_FOC_NXSCOPE)
    list(APPEND CSRCS foc_nxscope.c)
  endif()

  # perf support

  if(CONFIG_EXAMPLES_FOC_PERF)
    list(APPEND CSRCS foc_perf.c)
  endif()

  target_sources(apps PRIVATE ${CSRCS})

endif()
