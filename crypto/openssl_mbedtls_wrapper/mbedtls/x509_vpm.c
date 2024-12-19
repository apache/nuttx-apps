/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/x509_vpm.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2015-2016 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/types.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int X509_VERIFY_PARAM_set_hostflags(X509_VERIFY_PARAM *param,
                                    unsigned long flags)
{
  return 0;
}

int X509_VERIFY_PARAM_clear_hostflags(X509_VERIFY_PARAM *param,
                                      unsigned long flags)
{
  return 0;
}
