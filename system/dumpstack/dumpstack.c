/****************************************************************************
 * apps/system/dumpstack/dumpstack.c
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

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

#include <execinfo.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int spid = -1;
  int epid = -1;

  if (argc == 2)
    {
      spid = epid = atoi(argv[1]);
    }
  else if (argc >= 3)
    {
      spid = atoi(argv[1]);
      epid = atoi(argv[2]);
    }

  if (spid < 0 || epid < 0)
    {
      spid = epid = gettid();
    }

  do
    {
      sched_dumpstack(spid);
    }
  while (spid++ != epid);

  return 0;
}
