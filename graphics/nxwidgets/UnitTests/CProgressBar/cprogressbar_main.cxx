/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CProgressBar/cprogressbar_main.cxx
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

#include "graphics/nxwidgets/cprogressbartest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

#define MAX_PROGRESSBAR 50

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

  printf("cprogressbar_main: Create CProgressBarTest instance\n");
  CProgressBarTest *test = new CProgressBarTest();
  updateMemoryUsage(g_mmprevious, "After creating CProgressBarTest");

  // Connect the NX server

  printf("cprogressbar_main: Connect the CProgressBarTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("cprogressbar_main: Failed to connect the CProgressBarTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cprogressbar_main: After connecting to the server");

  // Create a window to draw into

  printf("cprogressbar_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("cprogressbar_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cprogressbar_main: After creating a window");

  // Create a progress bar

  printf("cprogressbar_main: Create a ProgressBar\n");
  CProgressBar *bar = test->createProgressBar();
  if (!bar)
    {
      printf("cprogressbar_main: Failed to create a progress bar\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cprogressbar_main: After creating a progress bar");

  // Set the progress bar minimum and maximum values

  bar->setMinimumValue(0);
  bar->setMaximumValue(MAX_PROGRESSBAR);
  bar->setValue(0);
  bar->hidePercentageText();
  printf("cprogressbar_main: ProgressBar range %d->%d Initial value %d\n",
         bar->getMinimumValue(), bar->getMaximumValue(),
         bar->getValue());

  // Show the initial state of the checkbox

  test->showProgressBar(bar);
  sleep(1);

  // Now move the progress bar up from 0 to 100% (with percentages off)

  for (int i = 0; i <= MAX_PROGRESSBAR; i++)
    {
      bar->setValue(i);
      test->showProgressBar(bar);
      printf("cprogressbar_main: %d. New value %d\n", i, bar->getValue());
      usleep(1000); // The simulation needs this to let the X11 event loop run
    }
  updateMemoryUsage(g_mmprevious, "cprogressbar_main: After moving the progress bar up #1");
  usleep(500*1000);

  // Now move the progress bar up from 0 to 100% (with percentages off)

  bar->showPercentageText();
  bar->setValue(0);
  test->showProgressBar(bar);
  usleep(500*1000);

  for (int i = 0; i <= MAX_PROGRESSBAR; i++)
    {
      bar->setValue(i);
      test->showProgressBar(bar);
      printf("cprogressbar_main: %d. New value %d\n", i, bar->getValue());
      usleep(1000); // The simulation needs this to let the X11 event loop run
    }
  updateMemoryUsage(g_mmprevious, "cprogressbar_main: After moving the progress bar up #2");
  sleep(1);

  // Clean up and exit

  printf("cprogressbar_main: Clean-up and exit\n");
  delete bar;
  updateMemoryUsage(g_mmprevious, "After deleting the progress bar");
  delete test;
  updateMemoryUsage(g_mmprevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  return 0;
}
