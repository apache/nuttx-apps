/****************************************************************************
 * canutils/libuavcan/platform_stm32.cpp
 *
 *   Copyright (C) 2015-2016 Omni Hoverboards Inc. All rights reserved.
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

#include <cunistd>

#include <uavcan_stm32/uavcan_stm32.hpp>

/****************************************************************************
 * Configuration
 ****************************************************************************/

#if CONFIG_LIBUAVCAN_RX_QUEUE_CAPACITY == 0
#  undef CONFIG_LIBUAVCAN_RX_QUEUE_CAPACITY
#  define CONFIG_LIBUAVCAN_RX_QUEUE_CAPACITY
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void delay(void)
{
  std::usleep(uavcan_stm32::
              CanInitHelper<CONFIG_LIBUAVCAN_RX_QUEUE_CAPACITY>::
              getRecommendedListeningDelay().toUSec());
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uavcan::ICanDriver &getCanDriver(void)
{
  static uavcan_stm32::CanInitHelper<CONFIG_LIBUAVCAN_RX_QUEUE_CAPACITY> can;
  static bool initialized = false;

  if (!initialized)
    {
      uavcan::uint32_t bitrate = CONFIG_LIBUAVCAN_BIT_RATE;

#if CONFIG_LIBUAVCAN_INIT_RETRIES > 0
      int retries = 0;
#endif

      while (can.init(delay, bitrate) < 0)
        {
#if CONFIG_LIBUAVCAN_INIT_RETRIES > 0
          retries++;
          if (retries >= CONFIG_LIBUAVCAN_INIT_RETRIES)
            {
              PANIC();
            }
#endif
        }

      initialized = true;
    }

  return can.driver;
}

uavcan::ISystemClock &getSystemClock(void)
{
  return uavcan_stm32::SystemClock::instance();
}
