# ##############################################################################
# apps/testing/drivers/crypto/CMakeLists.txt
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

if(CONFIG_TESTING_CRYPTO)
  if(CONFIG_TESTING_CRYPTO_3DES_CBC)
    nuttx_add_application(
      NAME
      des3cbc
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      3descbc.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_AES_XTS)
    nuttx_add_application(
      NAME
      aesxts
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      aesxts.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_AES_CTR)
    nuttx_add_application(
      NAME
      aesctr
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      aesctr.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_AES_CBC)
    nuttx_add_application(
      NAME
      aescbc
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      aescbc.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_HMAC)
    nuttx_add_application(
      NAME
      hmac
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      hmac.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_HASH)
    nuttx_add_application(
      NAME
      hash
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      hash.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_CRC32)
    nuttx_add_application(
      NAME
      crc32
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      crc32.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_AES_CMAC)
    nuttx_add_application(
      NAME
      aescmac
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      aescmac.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_ECDSA)
    nuttx_add_application(
      NAME
      ecdsa
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      ecdsa.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_ECDH)
    nuttx_add_application(
      NAME
      dhm
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      dhm.c)
  endif()

  if(CONFIG_TESTING_CRYPTO_RSA)
    nuttx_add_application(
      NAME
      rsa
      PRIORITY
      ${CONFIG_TESTING_CRYPTO_PRIORITY}
      STACKSIZE
      ${CONFIG_TESTING_CRYPTO_STACKSIZE}
      MODULE
      ${CONFIG_TESTING_CRYPTO}
      SRCS
      rsa.c)
  endif()

endif()
