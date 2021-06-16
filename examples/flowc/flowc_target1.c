/****************************************************************************
 * apps/examples/flowc/flowc_target1.c
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

#include "config.h"
#include "flowc.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * flowc1_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Run the receiver or sender, depending upon how we are configured */

#ifdef CONFIG_EXAMPLES_FLOWC_SENDER1
  return flowc_sender(argc, argv);
#else
  return flowc_receiver(argc, argv);
#endif
}
