/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CScrollbarHorizontal/cscrollbarhorizontal_main.cxx
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

#include "graphics/nxwidgets/cscrollbarhorizontaltest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

#define MAX_SCROLLBAR 50

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
// Name: nxheaders_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Initialize memory monitor logic

  initMemoryUsage();

  // Create an instance of the checkbox test

  printf("cscrollbarhorizontal_main: Create CScrollbarHorizontalTest instance\n");
  CScrollbarHorizontalTest *test = new CScrollbarHorizontalTest();
  updateMemoryUsage(g_mmprevious, "After creating CScrollbarHorizontalTest");

  // Connect the NX server

  printf("cscrollbarhorizontal_main: Connect the CScrollbarHorizontalTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("cscrollbarhorizontal_main: Failed to connect the CScrollbarHorizontalTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cscrollbarhorizontal_main: After connecting to the server");

  // Create a window to draw into

  printf("cscrollbarhorizontal_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("cscrollbarhorizontal_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cscrollbarhorizontal_main: After creating a window");

  // Create a scrollbar

  printf("cscrollbarhorizontal_main: Create a Scrollbar\n");
  CScrollbarHorizontal *scrollbar = test->createScrollbar();
  if (!scrollbar)
    {
      printf("cscrollbarhorizontal_main: Failed to create a scrollbar\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cscrollbarhorizontal_main: After creating a scrollbar");

  // Set the scrollbar minimum and maximum values

  scrollbar->setMinimumValue(0);
  scrollbar->setMaximumValue(MAX_SCROLLBAR);
  scrollbar->setValue(0);
  printf("cscrollbarhorizontal_main: Scrollbar range %d->%d Initial value %d\n",
         scrollbar->getMinimumValue(), scrollbar->getMaximumValue(),
         scrollbar->getValue());

  // Show the initial state of the checkbox

  test->showScrollbar(scrollbar);
  sleep(1);

  // Now move the scrollbar up

  for (int i = 0; i <= MAX_SCROLLBAR; i++)
    {
      scrollbar->setValue(i);
      test->showScrollbar(scrollbar);
      printf("cscrollbarhorizontal_main: %d. New value %d\n", i, scrollbar->getValue());
      usleep(1000); // The simulation needs this to let the X11 event loop run
    }
  updateMemoryUsage(g_mmprevious, "cscrollbarhorizontal_main: After moving the scrollbar up");

  // And move the scrollbar down

  for (int i = MAX_SCROLLBAR; i >= 0; i--)
    {
      scrollbar->setValue(i);
      test->showScrollbar(scrollbar);
      printf("cscrollbarhorizontal_main: %d. New value %d\n", i, scrollbar->getValue());
      usleep(1000); // The simulation needs this to let the X11 event loop run
    }
  updateMemoryUsage(g_mmprevious, "cscrollbarhorizontal_main: After moving the scrollbar down");
  sleep(1);

  // Clean up and exit

  printf("cscrollbarhorizontal_main: Clean-up and exit\n");
  delete scrollbar;
  updateMemoryUsage(g_mmprevious, "After deleting the scrollbar");
  delete test;
  updateMemoryUsage(g_mmprevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  return 0;
}
