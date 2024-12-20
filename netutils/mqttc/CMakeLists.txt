############################################################################
# apps/netutils/mqttc/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

if(CONFIG_NETUTILS_MQTTC)

  # ############################################################################
  # Config and Fetch MQTTC lib
  # ############################################################################

  set(MQTTC_DIR ${CMAKE_CURRENT_LIST_DIR}/MQTT-C)

  if(NOT EXISTS ${MQTTC_DIR})
    set(MQTTC_URL "https://github.com/LiamBindle/MQTT-C/archive")
    FetchContent_Declare(
      mqttc_fetch
      URL ${MQTTC_URL}/${CONFIG_NETUTILS_MQTTC_VERSION}.tar.gz SOURCE_DIR
          ${MQTTC_DIR} BINARY_DIR ${CMAKE_BINARY_DIR}/apps/netutils/mqttc/MQTT-C
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(mqttc_fetch)

    if(NOT mqttc_fetch_POPULATED)
      FetchContent_Populate(mqttc_fetch)
    endif()

    execute_process(
      COMMAND
        sh -c
        "cat 0001_add_connection_status.patch | patch -s -N -d $(MQTTC_DIR) -p1"
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  endif()

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${MQTTC_DIR}/include)
  set(CFLAGS -Wno-return-type)
  if(CONFIG_NETUTILS_MQTTC_WITH_MBEDTLS)
    list(APPEND CFLAGS -DMQTT_USE_MBEDTLS)
  endif()

  file(GLOB CSRCS ${MQTTC_DIR}/src/*.c)

  nuttx_add_library(mqttc STATIC)
  target_sources(mqttc PRIVATE ${CSRCS})
  target_compile_options(mqttc PRIVATE ${CFLAGS})

  if(CONFIG_NETUTILS_MQTTC_EXAMPLE)
    if(CONFIG_NETUTILS_MQTTC_WITH_MBEDTLS)
      nuttx_add_application(
        NAME
        mqttc_mbedtls_pub
        STACKSIZE
        ${CONFIG_NETUTILS_MQTTC_EXAMPLE_STACKSIZE}
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        SRCS
        ${MQTTC_DIR}/examples/mbedtls_publisher.c
        COMPILE_FLAGS
        ${CFLAGS}
        DEPENDS
        mqttc)
    else()
      set(MQTT_PUB_CFLAGS
          ${CFLAGS}
          -Dopen_nb_socket=pub_open_nb_socket
          -Dexit_example=pub_exit_example
          -Dpublish_callback=pub_publish_callback
          -Dclient_refresher=pub_client_refresher)

      set(MQTT_SUB_CFLAGS
          ${CFLAGS}
          -Dopen_nb_socket=sub_open_nb_socket
          -Dexit_example=sub_exit_example
          -Dpublish_callback=sub_publish_callback
          -Dclient_refresher=sub_client_refresher)

      nuttx_add_application(
        NAME
        mqttc_posix_pub
        STACKSIZE
        ${CONFIG_NETUTILS_MQTTC_EXAMPLE_STACKSIZE}
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        SRCS
        ${MQTTC_DIR}/examples/simple_publisher.c
        COMPILE_FLAGS
        ${MQTT_PUB_CFLAGS}
        DEPENDS
        mqttc)

      nuttx_add_application(
        NAME
        mqttc_posix_sub
        STACKSIZE
        ${CONFIG_NETUTILS_MQTTC_EXAMPLE_STACKSIZE}
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        SRCS
        ${MQTTC_DIR}/examples/simple_subscriber.c
        COMPILE_FLAGS
        ${MQTT_SUB_CFLAGS}
        DEPENDS
        mqttc)
    endif()
  endif()
endif()
