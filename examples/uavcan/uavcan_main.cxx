/****************************************************************************
 * examples/uavcan/uavcan_main.cxx
 *
 *   Copyright (C) 2015 Omni Hoverboards Inc. All rights reserved.
 *   Author: Paul Alexander Patience <paul-a.patience@polymtl.ca>
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

#include <nuttx/config.h>

#include <cstdio>
#include <cstdlib>

#include <nuttx/arch.h>

#include <uavcan/uavcan.hpp>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

extern uavcan::ICanDriver &getCanDriver(void);
extern uavcan::ISystemClock &getSystemClock(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uavcan_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
extern "C" int uavcan_main(int argc, FAR char *argv[])
#endif
{
  up_cxxinitialize();

  static uavcan::Node<CONFIG_EXAMPLES_UAVCAN_NODE_MEM_POOL_SIZE>
    node(getCanDriver(), getSystemClock());

  node.setNodeID(CONFIG_EXAMPLES_UAVCAN_NODE_ID);
  node.setName(CONFIG_EXAMPLES_UAVCAN_NODE_NAME);

  int ret = node.start();
  if (ret < 0)
    {
      std::fprintf(stderr, "ERROR: node.start failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  node.setModeOperational();

  for (;;)
    {
      ret = node.spin(uavcan::MonotonicDuration::fromMSec(100));
      if (ret < 0)
        {
          std::fprintf(stderr, "ERROR: node.spin failed: %d\n", ret);
        }
    }

  return EXIT_SUCCESS;
}
