# ##############################################################################
# apps/testing/nest/CMakeLists.txt
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

if(CONFIG_TESTING_NEST)

  set(SRCS ) # TODO: empty

  if(CONFIG_TESTING_NEST_COLLECTIONS)

    if(CONFIG_TESTING_NEST_COLLECTIONS_LIST)
      list(APPEND SRCS collections/list.c)
    endif()

    if(CONFIG_TESTING_NEST_COLLECTIONS_CBUF)
      list(APPEND SRCS collections/cbuf.c)
    endif()

    if(CONFIG_TESTING_NEST_COLLECTIONS_HMAP)
      list(APPEND SRCS collections/hmap.c)
    endif()
         
  endif() # CONFIG_TESTING_NEST_COLLECTIONS

  if(CONFIG_TESTING_NEST_DEVICES)

    if(CONFIG_TESTING_NEST_DEVICES_DEVNULL)
      list(APPEND SRCS collections/devnull.c)
    endif()

    if(CONFIG_TESTING_NEST_DEVICES_DEVZERO)
      list(APPEND SRCS collections/devzero.c)
    endif()

    if(CONFIG_TESTING_NEST_DEVICES_DEVASCII)
      list(APPEND SRCS collections/devascii.c)
    endif()

    if(CONFIG_TESTING_NEST_DEVICES_DEVCONSOLE)
      list(APPEND SRCS collections/devconsole.c)
    endif()

    if(CONFIG_TESTING_NEST_DEVICES_DEVURANDOM)
      list(APPEND SRCS collections/devurandom.c)
    endif()
         
  endif() # CONFIG_TESTING_NEST_DEVICES

  set(NEST_SRCS nest_main.c ${SRCS})
  nuttx_add_application(NAME ${CONFIG_TESTING_NEST_PROGNAME} SRCS ${NEST_SRCS})

endif()
