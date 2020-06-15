/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CCheckBox/ccheckbox_main.cxx
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
#include "graphics/nxwidgets/ccheckboxtest.hxx"

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
// Name: nxheaders_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Initialize memory monitor logic

  initMemoryUsage();

  // Create an instance of the checkbox test

  printf("ccheckbox_main: Create CCheckBoxTest instance\n");
  CCheckBoxTest *test = new CCheckBoxTest();
  updateMemoryUsage(g_mmprevious, "After creating CCheckBoxTest");

  // Connect the NX server

  printf("ccheckbox_main: Connect the CCheckBoxTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("ccheckbox_main: Failed to connect the CCheckBoxTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "ccheckbox_main: After connecting to the server");

  // Create a window to draw into

  printf("ccheckbox_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("ccheckbox_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "ccheckbox_main: After creating a window");

  // Show the initial state of the checkbox

  test->showCheckBox();
  test->showCheckBoxState();
  sleep(1);

  // Now click the checkbox

  printf("ccheckbox_main: Click 1\n");
  test->clickCheckBox();
  usleep(500*1000);
  test->showCheckBoxState();
  updateMemoryUsage(g_mmprevious, "After click 1");
  usleep(500*1000);

  printf("ccheckbox_main: Click 2\n");
  test->clickCheckBox();
  usleep(500*1000);
  test->showCheckBoxState();
  updateMemoryUsage(g_mmprevious, "After click 2");
  usleep(500*1000);

  printf("ccheckbox_main: Click 3\n");
  test->clickCheckBox();
  usleep(500*1000);
  test->showCheckBoxState();
  updateMemoryUsage(g_mmprevious, "After click 3");
  sleep(2);

  // Clean up and exit

  printf("ccheckbox_main: Clean-up and exit\n");
  delete test;
  updateMemoryUsage(g_mmprevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  return 0;
}
