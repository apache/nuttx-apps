# ##############################################################################
# apps/testing/nuts/CMakeLists.txt
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

if(CONFIG_TESTING_NUTS)

  set(SRCS)

  if(CONFIG_TESTING_NUTS_DSTRUCTS)
    file(GLOB DSTRUCTS dstructs/*.c)
    list(APPEND SRCS ${DSTRUCTS})
  endif() # CONFIG_TESTING_NUTS_DSTRUCTS

  if(CONFIG_TESTING_NUTS_DEVICES)
    file(GLOB DEVICES devices/*.c)
    list(APPEND SRCS ${DEVICES})
  endif() # CONFIG_TESTING_NUTS_DEVICES

  set(NUTS_SRCS nuts_main.c ${SRCS})
  nuttx_add_application(NAME ${CONFIG_TESTING_NUTS_PROGNAME} SRCS ${NUTS_SRCS})

endif()
