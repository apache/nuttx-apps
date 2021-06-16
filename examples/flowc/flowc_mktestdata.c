/****************************************************************************
 * apps/examples/flowc/flowc_mktestdata.c
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **envp)
{
  int ch;
  int i;

  for (i = 0; i < 100; i++)
    {
      for (ch = 0x20; ch < 0x7F; ch++)
        {
          putchar(ch);
        }
    }

  return 0;
}
