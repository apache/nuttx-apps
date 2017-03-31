/****************************************************************************
 * apps/tests/unity_ramtest/unity_ramtest_main.c
 * Main function for Unity test application
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
#include <debug.h>

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
static void runAllTests(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: runAllTests
 *
 * Description:
 *   Sequentially runs all included test groups
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
static void runAllTests(void)
{
  RUN_TEST_GROUP(MarchingZeroes);
  RUN_TEST_GROUP(MarchingOnes);
  RUN_TEST_GROUP(Pattern);
  RUN_TEST_GROUP(AddrInAddr);
}

/****************************************************************************
 * Name: number_of_transfers
 *
 * Description:
 *   Calculate number of transfers to cover the chunk of memory
 *
 * Input Parameters:
 *   width - access width
 *   size - memory size
 *
 * Returned Value:
 *   Number of transfers
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
static size_t number_of_transfers(enum memory_access_width width, size_t size)
{
  if (width == MEMORY_ACCESS_BYTE)
    {
      return size;
    }
  else if (width == MEMORY_ACCESS_WORD)
    {
      return size >> 2;
    }
  else if (width == MEMORY_ACCESS_HALFWORD)
    {
      return size >> 1;
    }
  else
    {
      DEBUGASSERT(0);
      return 0;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: unity_ramtest_write_memory
 *
 * Description:
 *   Write memory with specific pattern and access width
 *
 * Input Parameters:
 *   value - pattern to write
 *   width - access width
 *   start - memory region starting address
 *   size - number of bytes allocated for the region
 *
 * Returned Value:
 *   none, fails the testcase
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
void unity_ramtest_write_memory(uint32_t value, enum memory_access_width width, void *start, size_t size)
{
  size_t i, nxfrs = number_of_transfers(width, size);

  if (width == MEMORY_ACCESS_WORD)
    {
      uint32_t *ptr = (uint32_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          *ptr++ = value;
        }
    }
  else if (width == MEMORY_ACCESS_HALFWORD)
    {
      uint16_t *ptr = (uint16_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          *ptr++ = (uint16_t)value;
        }
    }
  else if (width == MEMORY_ACCESS_BYTE)
    {
      uint8_t *ptr = (uint8_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          *ptr++ = (uint8_t)value;
        }
    }
  else
    {
      TEST_FAIL_MESSAGE("Invalid width value");
    }
}

/****************************************************************************
 * Name: unity_ramtest_verify_memory
 *
 * Description:
 *   Verifies memory has specific pattern and reading it with access width
 *
 * Input Parameters:
 *   value - pattern to expect
 *   width - access width
 *   start - memory region starting address
 *   size - number of bytes allocated for the region
 *
 * Returned Value:
 *   none, fails the testcase
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
void unity_ramtest_verify_memory(uint32_t value, enum memory_access_width width, void *start, size_t size)
{
  size_t i, nxfrs = number_of_transfers(width, size);

  if (width == MEMORY_ACCESS_WORD)
    {
      uint32_t *ptr = (uint32_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
	  TEST_ASSERT_EQUAL_HEX32(value, *ptr);
          ptr++;
        }
    }
  else if (width == MEMORY_ACCESS_HALFWORD)
    {
      uint16_t value16 = (uint16_t)(value & 0x0000ffff);
      uint16_t *ptr = (uint16_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
	  TEST_ASSERT_EQUAL_HEX16(value16, *ptr);
          ptr++;
        }
    }
  else if (width == MEMORY_ACCESS_BYTE)
    {
      uint8_t value8 = (uint8_t)(value & 0x000000ff);
      uint8_t *ptr = (uint8_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
	  TEST_ASSERT_EQUAL_HEX(value8, *ptr);
          ptr++;
        }
    }
  else
    {
      TEST_FAIL_MESSAGE("Invalid width value");
    }
}

/****************************************************************************
 * Name: unity_ramtest_write_memory2
 *
 * Description:
 *   Writes memory with two alternating patterns
 *
 * Input Parameters:
 *   value_one - first pattern to write
 *   value_two - second pattern to write
 *   width - access width
 *   start - memory region starting address
 *   size - number of bytes allocated for the region
 *
 * Returned Value:
 *   none, fails the testcase
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
void unity_ramtest_write_memory2(uint32_t value_one, uint32_t value_two, enum memory_access_width width, void *start, size_t size)
{
  size_t even_nxfrs = number_of_transfers(width, size) & ~1;
  size_t i;

  if (width == MEMORY_ACCESS_WORD)
    {
      uint32_t *ptr = (uint32_t*)start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          *ptr++ = value_one;
          *ptr++ = value_two;
        }
    }
  else if (width == MEMORY_ACCESS_HALFWORD)
    {
      uint16_t *ptr = (uint16_t*)start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          *ptr++ = (uint16_t)value_one;
          *ptr++ = (uint16_t)value_two;
        }
    }
  else if (width == MEMORY_ACCESS_BYTE)
    {
      uint8_t *ptr = (uint8_t*)start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          *ptr++ = (uint8_t)value_one;
          *ptr++ = (uint8_t)value_two;
        }
    }
  else
    {
      TEST_FAIL_MESSAGE("Invalid width value");
    }
}

/****************************************************************************
 * Name: unity_ramtest_verify_memory2
 *
 * Description:
 *   Verifies memory is filled with two alternating patterns
 *
 * Input Parameters:
 *   value_one - first pattern to expect
 *   value_two - second pattern to expect
 *   width - access width
 *   start - memory region starting address
 *   size - number of bytes allocated for the region
 *
 * Returned Value:
 *   none, fails the testcase
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
void unity_ramtest_verify_memory2(uint32_t value_one, uint32_t value_two, enum memory_access_width width, void *start, size_t size)
{
  size_t even_nxfrs = number_of_transfers(width, size) & ~1;
  size_t i;

  if (width == MEMORY_ACCESS_WORD)
    {
      uint32_t *ptr = (uint32_t*)start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          TEST_ASSERT_EQUAL_HEX32(value_one, ptr[0]);
          TEST_ASSERT_EQUAL_HEX32(value_two, ptr[1]);
          ptr += 2;
        }
    }
  else if (width == MEMORY_ACCESS_HALFWORD)
    {
      uint16_t value16_one = (uint16_t)(value_one & 0x0000ffff);
      uint16_t value16_two = (uint16_t)(value_two & 0x0000ffff);
      uint16_t *ptr = (uint16_t*)start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          TEST_ASSERT_EQUAL_HEX16(value16_one, ptr[0]);
          TEST_ASSERT_EQUAL_HEX16(value16_two, ptr[1]);
          ptr += 2;
        }
    }
  else if (width == MEMORY_ACCESS_BYTE)
    {
      uint8_t value8_one = (uint8_t)(value_one & 0x000000ff);
      uint8_t value8_two = (uint8_t)(value_two & 0x000000ff);
      uint8_t *ptr = (uint8_t*)start;
      for (i = 0; i < even_nxfrs; i += 2)
        {
          TEST_ASSERT_EQUAL_HEX(value8_one, ptr[0]);
          TEST_ASSERT_EQUAL_HEX(value8_two, ptr[1]);
          ptr += 2;
        }
    }
  else
    {
      TEST_FAIL_MESSAGE("Invalid width value");
    }
}

/****************************************************************************
 * Name: unity_ramtest_write_addrinaddr
 *
 * Description:
 *   Writes every next memory location with the value from the previous location
 *
 * Input Parameters:
 *   width - access width
 *   start - memory region starting address
 *   size - number of bytes allocated for the region
 *
 * Returned Value:
 *   none, fails the testcase
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
void unity_ramtest_write_addrinaddr(enum memory_access_width width, void *start, size_t size)
{
  size_t i, nxfrs = number_of_transfers(width, size);

  if (width == MEMORY_ACCESS_WORD)
    {
      uint32_t *ptr = (uint32_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          uint32_t value32 = (uint32_t)((uintptr_t)ptr);
          *ptr++ = value32;
        }
    }
  else if (width == MEMORY_ACCESS_HALFWORD)
    {
      uint16_t *ptr = (uint16_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          uint16_t value16 = (uint16_t)((uintptr_t)ptr & 0x0000ffff);
          *ptr++ = value16;
        }
    }
  else if (width == MEMORY_ACCESS_BYTE)
    {
      uint8_t *ptr = (uint8_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          uint8_t value8 = (uint8_t)((uintptr_t)ptr & 0x000000ff);
          *ptr++ = value8;
        }
    }
  else
    {
      TEST_FAIL_MESSAGE("Invalid width value");
    }
}

/****************************************************************************
 * Name: unity_ramtest_verify_addrinaddr
 *
 * Description:
 *   Verifies every memory location is filled with the value from the previous location
 *
 * Input Parameters:
 *   width - access width
 *   start - memory region starting address
 *   size - number of bytes allocated for the region
 *
 * Returned Value:
 *   none, fails the testcase
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
void unity_ramtest_verify_addrinaddr(enum memory_access_width width, void *start, size_t size)
{
  size_t i, nxfrs = number_of_transfers(width, size);

  if (width == MEMORY_ACCESS_WORD)
    {
      uint32_t *ptr = (uint32_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          uint32_t value32 = (uint32_t)((uintptr_t)ptr);
          TEST_ASSERT_EQUAL_HEX32(*ptr, value32);
          ptr++;
        }
    }
  else if (width == MEMORY_ACCESS_HALFWORD)
    {
      uint16_t *ptr = (uint16_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          uint16_t value16 = (uint16_t)((uintptr_t)ptr & 0x0000ffff);
          TEST_ASSERT_EQUAL_HEX16(*ptr, value16);
          ptr++;
        }
    }
  else if (width == MEMORY_ACCESS_BYTE)
    {
      uint8_t *ptr = (uint8_t*)start;
      for (i = 0; i < nxfrs; i++)
        {
          uint16_t value8 = (uint8_t)((uintptr_t)ptr & 0x000000ff);
          TEST_ASSERT_EQUAL_HEX(*ptr, value8);
          ptr++;
        }
    }
  else
    {
      TEST_FAIL_MESSAGE("Invalid width value");
    }
}

/****************************************************************************
 * Name: unity_ramtest_main
 *
 * Description:
 *   Application entry point
 *
 * Input Parameters:
 *   argc - number of arguments
 *   argv - arguments themselves
 *
 * Returned Value:
 *   exit status
 *
 * Assumptions/Limitations:
 *   None
 *
 ****************************************************************************/
int unity_ramtest_main(int argc, const char* argv[])
{
  return UnityMain(argc, argv, runAllTests);
}
