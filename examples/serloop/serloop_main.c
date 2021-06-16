/****************************************************************************
 * apps/examples/serloop/serloop_main.c
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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * serloop_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_EXAMPLES_SERLOOP_BUFIO
  int ch;

  for (;;)
    {
      ch = getchar();
      if (ch < 1)
        {
          ch = '!';
        }
      else if ((ch < 0x20 || ch > 0x7e) && ch != '\n')
        {
          ch = '.';
        }

      putchar(ch);
    }
#else
  uint8_t ch;
  int ret;

  for (;;)
    {
      ret = read(0, &ch, 1);
      if (ret < 1)
        {
          ch = '!';
        }
      else if ((ch < 0x20 || ch > 0x7e) && ch != '\n')
        {
          ch = '.';
        }

      ret = write(1, &ch, 1);
    }
#endif
  return 0;
}
