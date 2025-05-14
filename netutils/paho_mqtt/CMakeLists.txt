# ##############################################################################
# apps/netutils/paho_mqtt/CMakeLists.txt
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

if(CONFIG_LIB_MQTT5)

  # ############################################################################
  # Config and Fetch Paho MQTT C library
  # ############################################################################

  set(PAHO_MQTT_DIR ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt)

  if(NOT EXISTS ${PAHO_MQTT_DIR})
    # Default version if not specified in config
    if(DEFINED CONFIG_NETUTILS_PAHO_MQTT_VERSION)
      set(PAHO_MQTT_VERSION ${CONFIG_NETUTILS_PAHO_MQTT_VERSION})
    else()
      set(PAHO_MQTT_VERSION "1.3.15")
    endif()

    set(PAHO_MQTT_URL "https://github.com/eclipse-paho/paho.mqtt.c/archive")
    FetchContent_Declare(
      paho_mqtt_fetch
      URL ${PAHO_MQTT_URL}/v${PAHO_MQTT_VERSION}.zip SOURCE_DIR ${PAHO_MQTT_DIR}
          BINARY_DIR ${CMAKE_BINARY_DIR}/apps/netutils/paho_mqtt/paho_mqtt
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(paho_mqtt_fetch)

    if(NOT paho_mqtt_fetch_POPULATED)
      FetchContent_Populate(paho_mqtt_fetch)

      # GitHub ZIP files extract with a versioned top-level directory Move
      # contents from paho.mqtt.c-<version> to paho_mqtt if needed
      file(GLOB extracted_dirs "${PAHO_MQTT_DIR}/paho.mqtt.c-*")
      if(extracted_dirs)
        list(GET extracted_dirs 0 extracted_dir)
        # Move all contents from the versioned directory to paho_mqtt
        file(GLOB extracted_contents "${extracted_dir}/*")
        foreach(item ${extracted_contents})
          get_filename_component(item_name ${item} NAME)
          execute_process(
            COMMAND ${CMAKE_COMMAND} -E rename ${item}
                    ${PAHO_MQTT_DIR}/${item_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
        endforeach()
        # Remove the empty versioned directory
        execute_process(
          COMMAND ${CMAKE_COMMAND} -E remove_directory ${extracted_dir}
          WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
        message("Moved contents from versioned directory to paho_mqtt")
      endif()

      # Remove downloaded zip file if exists in current directory
      file(GLOB zip_files "${CMAKE_CURRENT_LIST_DIR}/v${PAHO_MQTT_VERSION}.zip")
      if(zip_files)
        file(REMOVE ${zip_files})
        message("Removed downloaded zip file")
      endif()
    endif()

    # Apply paho_mqtt_01 patch if exists
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt_01.patch)
      execute_process(
        COMMAND
          sh -c
          "patch -p1 --forward --ignore-whitespace < ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt_01.patch || true"
        WORKING_DIRECTORY ${PAHO_MQTT_DIR})
      message("paho_mqtt_01 patching done")
    endif()

    message("paho_mqtt download done")
  endif()

  configure_file(paho_mqtt/src/VersionInfo.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.h @ONLY)

  set(MQTT5_INCDIR ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt/src)
  list(APPEND MQTT5_INCDIR ${CMAKE_CURRENT_BINARY_DIR})

  file(GLOB CSRCS paho_mqtt/src/*.c)

  if(CONFIG_UTILS_MQTT5)
    list(APPEND CSRCS
         ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt/src/samples/pubsub_opts.c)
  endif()

  if(CONFIG_OPENSSL_MBEDTLS_WRAPPER)
    list(REMOVE_ITEM CSRCS ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt/src/SHA1.c)
  endif()

  list(REMOVE_ITEM CSRCS ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt/src/MQTTClient.c
       ${CMAKE_CURRENT_LIST_DIR}/paho_mqtt/src/MQTTVersion.c)

  nuttx_add_library(mqtt5)

  target_sources(mqtt5 PRIVATE ${CSRCS})

  target_include_directories(mqtt5 PRIVATE ${MQTT5_INCDIR})

  target_compile_options(mqtt5 PRIVATE ${MQTT5_FLAGS})

  if(CONFIG_UTILS_MQTT5)

    set(MQTT_PUB_FLAGS
        -DmessageArrived=mqtt_pub_messageArrived
        -DonDisconnect=mqtt_pub_onDisconnect
        -DonConnectFailure5=mqtt_pub_onConnectFailure5
        -DonConnectFailure=mqtt_pub_onConnectFailure
        -DonConnect5=mqtt_pub_onConnect5
        -DonConnect=mqtt_pub_onConnect
        -Dmysleep=mqtt_pub_mysleep
        -DtoStop=mqtt_pub_toStop
        -Dopts=mqtt_pub_opts
        -Dmyconnect=mqtt_pub_myconnect
        -Dcfinish=mqtt_pub_cfinish
        -Dtrace_callback=mqtt_pub_trace_callback)

    nuttx_add_application(
      NAME
      mqtt_pub
      SRCS
      paho_mqtt/src/samples/paho_c_pub.c
      DEPENDS
      mqtt5
      INCLUDE_DIRECTORIES
      ${MQTT5_INCDIR}
      COMPILE_FLAGS
      ${MQTT_PUB_FLAGS}
      STACKSIZE
      ${CONFIG_UTILS_MQTT5_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_MQTT5_PRIORITY})

    nuttx_add_application(
      NAME
      mqtt_sub
      SRCS
      paho_mqtt/src/samples/paho_c_sub.c
      DEPENDS
      mqtt5
      INCLUDE_DIRECTORIES
      ${MQTT5_INCDIR}
      STACKSIZE
      ${CONFIG_UTILS_MQTT5_STACKSIZE}
      PRIORITY
      ${CONFIG_UTILS_MQTT5_PRIORITY})
  endif()
endif()
