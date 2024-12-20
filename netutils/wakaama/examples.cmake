# ##############################################################################
# apps/netutils/wakaama/examples.cmake
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

set(CLIENT_DIR wakaama/examples/client)
set(CLIENT_LIGHT_DIR wakaama/examples/lightclient)
set(SHARED_DIR wakaama/examples/shared)

set(CLIENT_SOURCES
    ${CLIENT_DIR}/lwm2mclient.c
    ${CLIENT_DIR}/lwm2mclient.h
    ${CLIENT_DIR}/object_access_control.c
    ${CLIENT_DIR}/object_connectivity_moni.c
    ${CLIENT_DIR}/object_connectivity_stat.c
    ${CLIENT_DIR}/object_device.c
    ${CLIENT_DIR}/object_firmware.c
    ${CLIENT_DIR}/object_location.c
    ${CLIENT_DIR}/object_security.c
    ${CLIENT_DIR}/object_server.c
    ${CLIENT_DIR}/object_test.c
    ${CLIENT_DIR}/system_api.c
    ${SHARED_DIR}/commandline.c
    ${SHARED_DIR}/platform.c)

set(CLIENT_LIGHT_SOURCES
    ${CLIENT_LIGHT_DIR}/lightclient.c
    ${CLIENT_LIGHT_DIR}/object_device.c
    ${CLIENT_LIGHT_DIR}/object_security.c
    ${CLIENT_LIGHT_DIR}/object_server.c
    ${CLIENT_LIGHT_DIR}/object_test.c
    ${SHARED_DIR}/commandline.c
    ${SHARED_DIR}/platform.c)

if(CONFIG_WAKAAMA_EXAMPLE_CLIENT)
  nuttx_add_application(
    NAME
    lwm2mclient
    SRCS
    ${CLIENT_SOURCES}
    ${SHARED_DIR}/connection.c
    STACKSIZE
    ${CONFIG_WAKAAMA_EXAMPLE_CLIENT_STACKSIZE}
    INCLUDE_DIRECTORIES
    ${SHARED_DIR}
    DEPENDS
    wakaama)
endif()

if(CONFIG_WAKAAMA_EXAMPLE_CLIENT_DTLS)
  nuttx_add_application(
    NAME
    lwm2mclient_dtls
    SRCS
    ${CLIENT_SOURCES}
    ${SHARED_DIR}/dtlsconnection.c
    STACKSIZE
    ${CONFIG_WAKAAMA_EXAMPLE_CLIENT_STACKSIZE}
    INCLUDE_DIRECTORIES
    ${SHARED_DIR}
    DEFINITIONS
    WITH_TINYDTLS
    DEPENDS
    wakaama
    tinydtls)
endif()

if(CONFIG_WAKAAMA_EXAMPLE_CLIENT_LIGHT)
  nuttx_add_application(
    NAME
    lwm2mclient_light
    SRCS
    ${CLIENT_LIGHT_SOURCES}
    ${SHARED_DIR}/connection.c
    STACKSIZE
    ${CONFIG_WAKAAMA_EXAMPLE_CLIENT_STACKSIZE}
    INCLUDE_DIRECTORIES
    ${SHARED_DIR}
    DEPENDS
    wakaama)
endif()
