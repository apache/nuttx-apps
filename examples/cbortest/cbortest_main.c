/****************************************************************************
 * apps/examples/cbortest/cbortest_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <fcntl.h>
#include <wchar.h>
#include <syslog.h>
#include <unistd.h>

#include "tinycbor/cbor.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MINMEA_MAX_LENGTH    256

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * cbortest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  CborError res;
  CborEncoder encoder;
  CborEncoder map_encoder;
  uint8_t output[50];
  size_t output_len;
  int i;

  printf("TinyCBOR test: Encoding { \"t\": 1234 }\n");

  /*  Init our CBOR Encoder */

  cbor_encoder_init(&encoder, output, sizeof(output), 0);

  /* Create a Map Encoder that maps keys to values, 1 = Key-Value Pairs */

  res = cbor_encoder_create_map(&encoder, &map_encoder, 1);

  /* Check for any error */

  assert(res == CborNoError);

  /*  First Key-Value Pair: Map the Key */

  res = cbor_encode_text_stringz(&map_encoder, "t");

  /* Check for any error */

  assert(res == CborNoError);

  /*  First Key-Value Pair: Map the Value */

  res = cbor_encode_int(&map_encoder, 1234);

  /* Check for any error */

  assert(res == CborNoError);

  /*  Close the Map Encoder */

  res = cbor_encoder_close_container(&encoder, &map_encoder);

  /* Check for any error */

  assert(res == CborNoError);

  /* How many bytes were encoded */

  output_len = cbor_encoder_get_buffer_size(&encoder, output);
  printf("CBOR Output: %d bytes\n", output_len);

  /*  Dump the encoded CBOR output (6 bytes): */

  printf("Expected sequence:  0xa1 0x61 0x74 0x19 0x04 0xd2\n");

  for (i = 0; i < output_len; i++)
    {
      printf("  0x%02x\n", output[i]);
    }

  return 0;
}
