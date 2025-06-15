/****************************************************************************
 * apps/testing/libc/wcstombs/wcstombs_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <stdint.h>
#include <string.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  /* Local variable declarations */

  const wchar_t *src;
  size_t dst_size;
  char *dst;
  size_t ret;
  size_t i;

  printf("wcstombs Test application:\n");

  /* Set the locale to the user's default locale */

  setlocale(LC_ALL, "");

  /* Example wide character array (source) */

  src = L"Hello, world!";

  /* Calculate the required size for the dst buffer */

  dst_size = wcstombs(NULL, src, 0) + 1; /* +1 for the null terminator */
  dst = (char *)malloc(dst_size);

  if (dst == NULL)
    {
      printf("ERROR: malloc failed.\n");
      return EXIT_FAILURE;
    }

  printf("\nTest Scenario: len is bigger than the size of the converted "
         "string. Expected the null-terminator at the end of the converted "
         "string.\n");

  /* Initialize dst with a known value (0xaa) */

  memset(dst, 0xaa, dst_size);

  /* Convert wide characters to multibyte characters */

  ret = wcstombs(dst, src, dst_size);

  /* Check if the conversion was successful */

  if (ret == (size_t)-1)
    {
      printf("ERROR: wcstombs failed.\n");
      free(dst);
      return EXIT_FAILURE;
    }

  /* Print the return code */

  printf("Return code: %zu\n", ret);

  /* Print the dst buffer as an array of uint8_t elements */

  printf("dst buffer (as uint8_t array): ");
  for (i = 0; i < dst_size; i++) /* Include the null terminator in the output */
    {
      printf("%02x ", (uint8_t)dst[i]);
    }

  printf("\n");

  /* Check if the dst value just after the return value is as expected */

  if (dst[ret] == '\0')
    {
      printf("The character just after the return value is the null "
             "terminating character.\n");
    }
  else
    {
      printf("The character just after the return value is not the expected "
             "null-terminator (value: %02x). This is a bug!\n", dst[ret]);
      free(dst);
      return EXIT_FAILURE;
    }

  printf("\nTest Scenario: len is exactly the size of the converted string. "
         "Do not expected the null-terminator at the end of the converted "
         "string.\n");

  /* Initialize dst with a known value (0xaa) */

  memset(dst, 0xaa, dst_size);

  /* Convert wide characters to multibyte characters */

  ret = wcstombs(dst, src, dst_size - 1);

  /* Check if the conversion was successful */

  if (ret == (size_t)-1)
    {
      printf("ERROR: wcstombs failed.\n");
      free(dst);
      return EXIT_FAILURE;
    }

  /* Print the return code */

  printf("Return code: %zu\n", ret);

  /* Print the dst buffer as an array of uint8_t elements */

  printf("dst buffer (as uint8_t array): ");
  for (i = 0; i < dst_size; i++) /* Include the null terminator in the output */
    {
      printf("%02x ", (uint8_t)dst[i]);
    }

  printf("\n");

  /* Check if the dst value just after the return value is as expected */

  if ((uint8_t)dst[ret] == 0xaa)
    {
      printf("The character just after the return value is the expected "
             "0xaa value. No null-terminator.\n");
    }
  else
    {
      printf("The character just after the return value is not the expected "
             "0xaa (value: %02x). This is a bug!\n", dst[ret]);
      free(dst);
      return EXIT_FAILURE;
    }

  printf("\nTest Scenario: len is smaller than the size of the converted "
         " string. Do not expected the null-terminator at the end of the "
         "converted string.\n");

  /* Initialize dst with a known value (0xaa) */

  memset(dst, 0xaa, dst_size);

  /* Convert wide characters to multibyte characters */

  ret = wcstombs(dst, src, dst_size - 2);

  /* Check if the conversion was successful */

  if (ret == (size_t)-1)
    {
      printf("ERROR: wcstombs failed.\n");
      free(dst);
      return EXIT_FAILURE;
    }

  /* Print the return code */

  printf("Return code: %zu\n", ret);

  /* Print the dst buffer as an array of uint8_t elements */

  printf("dst buffer (as uint8_t array): ");
  for (i = 0; i < dst_size; i++) /* Include the null terminator in the output */
    {
      printf("%02x ", (uint8_t)dst[i]);
    }

  printf("\n");

  /* Check if the dst value just after the return value is as expected */

  if ((uint8_t)dst[ret] == 0xaa)
    {
      printf("The character just after the return value is the expected "
             "0xaa value. No null-terminator.\n");
    }
  else
    {
      printf("The character just after the return value is not the expected "
             "0xaa (value: %02x). This is a bug!\n", dst[ret]);
      free(dst);
      return EXIT_FAILURE;
    }

  /* Free the allocated memory */

  free(dst);

  return EXIT_SUCCESS;
}
