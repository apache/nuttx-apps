/****************************************************************************
 * apps/examples/uGLy_demo/uGLy_demo.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <stdlib.h>

/****************************************************************************
 * External Function Prototypes
 ****************************************************************************/

int tri_run(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * main
 *
 * Description:
 *   The framebuffer setup, pixel conversion and page flipping are done by
 *   the uGLy NuttX backend (graphics/uGLy/uGLy/src/nuttx.c), which also
 *   owns the render buffers used by the library.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  return tri_run() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
