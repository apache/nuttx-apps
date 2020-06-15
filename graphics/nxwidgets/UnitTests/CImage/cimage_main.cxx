/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CImage/cimage_main.cxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <nuttx/init.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <unistd.h>
#include <debug.h>

#include <nuttx/nx/nx.h>

#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxglyphs.hxx"
#include "graphics/nxwidgets/cimagetest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Classes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

static struct mallinfo g_mmInitial;
static struct mallinfo g_mmprevious;

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Suppress name-mangling

extern "C" int main(int argc, char *argv[]);

/////////////////////////////////////////////////////////////////////////////
// Private Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Name: showMemoryUsage
/////////////////////////////////////////////////////////////////////////////

static void showMemoryUsage(FAR struct mallinfo *mmbefore,
                            FAR struct mallinfo *mmafter)
{
  printf("VARIABLE  BEFORE   AFTER\n");
  printf("======== ======== ========\n");
  printf("arena    %8d %8d\n", mmbefore->arena,    mmafter->arena);
  printf("ordblks  %8d %8d\n", mmbefore->ordblks,  mmafter->ordblks);
  printf("mxordblk %8d %8d\n", mmbefore->mxordblk, mmafter->mxordblk);
  printf("uordblks %8d %8d\n", mmbefore->uordblks, mmafter->uordblks);
  printf("fordblks %8d %8d\n", mmbefore->fordblks, mmafter->fordblks);
}

/////////////////////////////////////////////////////////////////////////////
// Name: updateMemoryUsage
/////////////////////////////////////////////////////////////////////////////

static void updateMemoryUsage(FAR struct mallinfo *previous,
                              FAR const char *msg)
{
  struct mallinfo mmcurrent;

  /* Get the current memory usage */

  mmcurrent = mallinfo();

  /* Show the change from the previous time */

  printf("\n%s:\n", msg);
  showMemoryUsage(previous, &mmcurrent);

  /* Set up for the next test */

  g_mmprevious = mmcurrent;
}

/////////////////////////////////////////////////////////////////////////////
// Name: initMemoryUsage
/////////////////////////////////////////////////////////////////////////////

static void initMemoryUsage(void)
{
  g_mmInitial  = mallinfo();
  g_mmprevious = g_mmInitial;
}

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Name: nxheaders_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Initialize memory monitor logic

  initMemoryUsage();

  // Create an instance of the font test

  printf("cimage_main: Create CImageTest instance\n");
  CImageTest *test = new CImageTest();
  updateMemoryUsage(&g_mmprevious, "After creating CImageTest");

  // Connect the NX server

  printf("cimage_main: Connect the CImageTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("cimage_main: Failed to connect the CImageTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(&g_mmprevious, "After connecting to the server");

  // Create a window to draw into

  printf("cimage_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("cimage_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(&g_mmprevious, "After creating a window");

  // Create an instance of the NuttX logo

  CRlePaletteBitmap *nuttxBitmap = new CRlePaletteBitmap(&g_nuttxBitmap160x160);
  updateMemoryUsage(&g_mmprevious, "After creating the bitmap");

  // Create a CImage instance

  CImage *image = test->createImage(static_cast<IBitmap*>(nuttxBitmap));
  if (!image)
    {
      printf("cimage_main: Failed to create a image\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(&g_mmprevious, "After creating CImage");

  // Show the image

  test->showImage(image);
  updateMemoryUsage(&g_mmprevious, "After showing the image");
  sleep(5);

  // Clean up and exit

  printf("cimage_main: Clean-up and exit\n");
  delete image;
  updateMemoryUsage(&g_mmprevious, "After deleting CImage");

  delete nuttxBitmap;
  updateMemoryUsage(&g_mmprevious, "After deleting the bitmap");

  delete test;
  updateMemoryUsage(&g_mmprevious, "After deleting the test");
  updateMemoryUsage(&g_mmInitial, "Final memory usage");
  return 0;
}
