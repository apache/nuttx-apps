/****************************************************************************
 * apps/examples/unity_ramtest/library.h
 * Common functions
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
#ifndef __TESTS_UNITY_RAMTEST_LIBRARY_H
#define __TESTS_UNITY_RAMTEST_LIBRARY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MEMORY_ALLOCATION_SIZE 0x2000

/****************************************************************************
 * Public Types
 ****************************************************************************/
enum memory_access_width
{
  MEMORY_ACCESS_BYTE = 8,
  MEMORY_ACCESS_HALFWORD = 16,
  MEMORY_ACCESS_WORD = 32
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
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
void unity_ramtest_write_memory(uint32_t value, enum memory_access_width width, void *start, size_t size);

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
void unity_ramtest_verify_memory(uint32_t value, enum memory_access_width width, void *start, size_t size);

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
void unity_ramtest_write_memory2(uint32_t value_one, uint32_t value_two, enum memory_access_width width, void *start, size_t size);

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
void unity_ramtest_verify_memory2(uint32_t value_one, uint32_t value_two, enum memory_access_width width, void *start, size_t size);

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
void unity_ramtest_write_addrinaddr(enum memory_access_width width, void *start, size_t size);

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
void unity_ramtest_verify_addrinaddr(enum memory_access_width width, void *start, size_t size);

#endif /* __TESTS_UNITY_RAMTEST_LIBRARY_H */
