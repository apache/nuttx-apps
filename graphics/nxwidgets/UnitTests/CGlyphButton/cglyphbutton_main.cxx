/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CGlyphButton/cglyphbutton_main.cxx
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

#include "graphics/nxwidgets/cglyphbuttontest.hxx"
#include "graphics/nxglyphs.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Classes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

static unsigned int g_mmInitial;
static unsigned int g_mmprevious;

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Suppress name-mangling

extern "C" int main(int argc, char *argv[]);

/////////////////////////////////////////////////////////////////////////////
// Private Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Name: updateMemoryUsage
/////////////////////////////////////////////////////////////////////////////

static void updateMemoryUsage(unsigned int previous,
                              FAR const char *msg)
{
  struct mallinfo mmcurrent;

  /* Get the current memory usage */

  mmcurrent = mallinfo();

  /* Show the change from the previous time */

  printf("\n%s:\n", msg);
  printf("  Before: %8d After: %8d Change: %8d\n\n",
         previous, mmcurrent.uordblks, mmcurrent.uordblks - previous);

  /* Set up for the next test */

  g_mmprevious = mmcurrent.uordblks;
}

/////////////////////////////////////////////////////////////////////////////
// Name: initMemoryUsage
/////////////////////////////////////////////////////////////////////////////

static void initMemoryUsage(void)
{
  struct mallinfo mmcurrent;

  /* Get the current memory usage */

  mmcurrent = mallinfo();

  g_mmInitial  = mmcurrent.uordblks;
  g_mmprevious = mmcurrent.uordblks;
}

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// nxheaders_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Initialize memory monitor logic

  initMemoryUsage();

  // Create an instance of the font test

  printf("cglyphbutton_main: Create CGlyphButtonTest instance\n");
  CGlyphButtonTest *test = new CGlyphButtonTest();
  updateMemoryUsage(g_mmprevious, "After creating CGlyphButtonTest");

  // Connect the NX server

  printf("cglyphbutton_main: Connect the CGlyphButtonTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("cglyphbutton_main: Failed to connect the CGlyphButtonTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "After connecting to the server");

  // Create a window to draw into

  printf("cglyphbutton_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("cglyphbutton_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "After creating a window");

  // Create a CGlyphButton instance

  CGlyphButton *button = test->createButton(&g_arrowDown, &g_arrowUp);
  if (!button)
    {
      printf("cglyphbutton_main: Failed to create a button\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "After creating the glyph button");

  // Show the button

  printf("cglyphbutton_main: Show the button\n");
  test->showButton(button);
  updateMemoryUsage(g_mmprevious, "After showing the glyph button");

  // Wait two seconds, then perform a simulated mouse click on the button

  sleep(2);
  printf("cglyphbutton_main: Click the button\n");
  test->click();
  updateMemoryUsage(g_mmprevious, "After clicking glyph button");

  // Poll for the mouse click event (of course this can hang if something fails)

  bool clicked = test->poll(button);
  printf("cglyphbutton_main: Button is %s\n", clicked ? "clicked" : "released");

  // Wait a second, then release the mouse buttone

  sleep(1);
  test->release();
  updateMemoryUsage(g_mmprevious, "After releasing glyph button");

  // Poll for the mouse release event (of course this can hang if something fails)

  clicked = test->poll(button);
  printf("cglyphbutton_main: Button is %s\n", clicked ? "clicked" : "released");

  // Wait a few more seconds so that the tester can ponder the result

  sleep(3);

  // Clean up and exit

  printf("cglyphbutton_main: Clean-up and exit\n");
  delete button;
  updateMemoryUsage(g_mmprevious, "After deleting the glyph button");
  delete test;
  updateMemoryUsage(g_mmprevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  return 0;
}
