/****************************************************************************
 * apps/examples/unity_usrsock/unity_usrsock_main.c
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
#include <stdlib.h>

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
int usrsocktest_endp_malloc_cnt = 0;
int usrsocktest_dcmd_malloc_cnt = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void get_mallinfo(struct mallinfo *mem)
{
#ifdef CONFIG_CAN_PASS_STRUCTS
  *mem = mallinfo();
#else
  (void)mallinfo(mem);
#endif
}

static void print_mallinfo(const struct mallinfo *mem, const char *title)
{
  if (title)
    printf("%s:\n", title);
  printf("       %11s%11s%11s%11s\n", "total", "used", "free", "largest");
  printf("Mem:   %11d%11d%11d%11d\n",
         mem->arena, mem->uordblks, mem->fordblks, mem->mxordblk);
}

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
  RUN_TEST_GROUP(CharDev);
  RUN_TEST_GROUP(NoDaemon);
  RUN_TEST_GROUP(BasicDaemon);
  RUN_TEST_GROUP(BasicConnect);
  RUN_TEST_GROUP(BasicConnectDelay);
  RUN_TEST_GROUP(NoBlockConnect);
  RUN_TEST_GROUP(BasicSend);
  RUN_TEST_GROUP(NoBlockSend);
  RUN_TEST_GROUP(BlockSend);
  RUN_TEST_GROUP(NoBlockRecv);
  RUN_TEST_GROUP(BlockRecv);
  RUN_TEST_GROUP(RemoteDisconnect);
  RUN_TEST_GROUP(BasicSetSockOpt);
  RUN_TEST_GROUP(BasicGetSockOpt);
  RUN_TEST_GROUP(BasicGetSockName);
  RUN_TEST_GROUP(WakeWithSignal);
  RUN_TEST_GROUP(MultiThread);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: unity_pwrbtn_main
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
int unity_usrsock_main(int argc, const char* argv[])
{
  int ret;
  struct mallinfo mem_before, mem_after;

  get_mallinfo(&mem_before);

  ret = UnityMain(argc, argv, runAllTests);

  get_mallinfo(&mem_after);

  print_mallinfo(&mem_before, "HEAP BEFORE TESTS");
  print_mallinfo(&mem_after, "HEAP AFTER TESTS");

  return ret;
}
