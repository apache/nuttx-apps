# ##############################################################################
# apps/crypto/controlse/CMakeLists.txt
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

if(CONFIG_CRYPTO_CONTROLSE)

  set(MBEDTLS_DIR ${CMAKE_BINARY_DIR}/apps/include/mbedtls)

  nuttx_add_application(
    NAME
    ${CONFIG_CRYPTO_CONTROLSE_PROGNAME}
    STACKSIZE
    ${CONFIG_CRYPTO_CONTROLSE_STACKSIZE}
    MODULE
    ${CONFIG_CRYPTO_CONTROLSE}
    SRCS
    controlse_main.cxx
    chex_util.cxx
    csecure_element.cxx
    cstring.cxx
    ccertificate.cxx
    ccsr.cxx
    cpublic_key.cxx
    cserial_number.cxx
    csan_builder.cxx
    INCLUDE_DIRECTORIES
    ${MBEDTLS_DIR})

endif()
