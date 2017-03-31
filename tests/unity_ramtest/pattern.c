/****************************************************************************
 * apps/tests/unity_ramtest/pattern.c
 * Automated RAM test copied from apps/system/ramtest
 *
 *   Copyright (C) 2015 Haltian Ltd. All rights reserved.
 *   Author: Roman Saveljev <roman.saveljev@haltian.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <testing/unity_fixture.h>
#include "library.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static void* g_memory;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pattern(enum memory_access_width width, uint32_t value_one, uint32_t value_two)
{
  unity_ramtest_write_memory2(value_one, value_two, width, g_memory, MEMORY_ALLOCATION_SIZE);
  unity_ramtest_verify_memory2(value_one, value_two, width, g_memory, MEMORY_ALLOCATION_SIZE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

TEST_GROUP(Pattern);

/****************************************************************************
 * Name: Pattern test group setup
 *
 * Description:
 *   Setup function executed before each testcase in this test group
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST_SETUP(Pattern)
{
  g_memory = malloc(MEMORY_ALLOCATION_SIZE);
  TEST_ASSERT_NOT_NULL(g_memory);
}

/****************************************************************************
 * Name: Pattern test group tear down
 *
 * Description:
 *   Tear down function executed after each testcase in this test group
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST_TEAR_DOWN(Pattern)
{
  free(g_memory);
  g_memory = NULL;
}

/****************************************************************************
 * Name: ByteAccess_5_a
 *
 * Description:
 *   Writing pattern 0x55 and 0xaa with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, ByteAccess_5_a)
{
  pattern(MEMORY_ACCESS_BYTE, 0x55, 0xaa);
}

/****************************************************************************
 * Name: ByteAccess_6_9
 *
 * Description:
 *   Writing pattern 0x66 and 0x99 with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, ByteAccess_6_9)
{
  pattern(MEMORY_ACCESS_BYTE, 0x66, 0x99);
}

/****************************************************************************
 * Name: ByteAccess_3_c
 *
 * Description:
 *   Writing pattern 0x33 and 0xcc with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, ByteAccess_3_c)
{
  pattern(MEMORY_ACCESS_BYTE, 0x33, 0xcc);
}

/****************************************************************************
 * Name: HalfAccess_5_a
 *
 * Description:
 *   Writing pattern 0x5555 and 0xaaaa with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, HalfAccess_5_a)
{
  pattern(MEMORY_ACCESS_BYTE, 0x5555, 0xaaaa);
}

/****************************************************************************
 * Name: HalfAccess_6_9
 *
 * Description:
 *   Writing pattern 0x6666 and 0x9999 with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, HalfAccess_6_9)
{
  pattern(MEMORY_ACCESS_BYTE, 0x6666, 0x9999);
}

/****************************************************************************
 * Name: HalfAccess_3_c
 *
 * Description:
 *   Writing pattern 0x3333 and 0xcccc with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, HalfAccess_3_c)
{
  pattern(MEMORY_ACCESS_BYTE, 0x3333, 0xcccc);
}

/****************************************************************************
 * Name: WordAccess_5_a
 *
 * Description:
 *   Writing pattern 0x55555555 and 0xaaaaaaaa with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, WordAccess_5_a)
{
  pattern(MEMORY_ACCESS_BYTE, 0x55555555, 0xaaaaaaaa);
}

/****************************************************************************
 * Name: WordAccess_6_9
 *
 * Description:
 *   Writing pattern 0x66666666 and 0x99999999 with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, WordAccess_6_9)
{
  pattern(MEMORY_ACCESS_BYTE, 0x66666666, 0x99999999);
}

/****************************************************************************
 * Name: WordAccess_3_c
 *
 * Description:
 *   Writing pattern 0x33333333 and 0xcccccccc with byte access
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
TEST(Pattern, WordAccess_3_c)
{
  pattern(MEMORY_ACCESS_BYTE, 0x33333333, 0xcccccccc);
}
