/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CSliderHorizontal/csliderhorizontal_main.cxx
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

#include "graphics/nxwidgets/csliderhorizontaltest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

#define MAX_SLIDER 50

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

  printf("csliderhorizontal_main: Create CSliderHorizontalTest instance\n");
  CSliderHorizontalTest *test = new CSliderHorizontalTest();
  updateMemoryUsage(g_mmprevious, "After creating CSliderHorizontalTest");

  // Connect the NX server

  printf("csliderhorizontal_main: Connect the CSliderHorizontalTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("csliderhorizontal_main: Failed to connect the CSliderHorizontalTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "csliderhorizontal_main: After connecting to the server");

  // Create a window to draw into

  printf("csliderhorizontal_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("csliderhorizontal_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "csliderhorizontal_main: After creating a window");

  // Create a slider

  printf("csliderhorizontal_main: Create a Slider\n");
  CSliderHorizontal *slider = test->createSlider();
  if (!slider)
    {
      printf("csliderhorizontal_main: Failed to create a slider\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "csliderhorizontal_main: After creating a slider");

  // Set the slider minimum and maximum values

  slider->setMinimumValue(0);
  slider->setMaximumValue(MAX_SLIDER);
  slider->setValue(0);
  printf("csliderhorizontal_main: Slider range %d->%d Initial value %d\n",
         slider->getMinimumValue(), slider->getMaximumValue(),
         slider->getValue());

  // Show the initial state of the checkbox

  test->showSlider(slider);
  sleep(1);

  // Now move the slider up

  for (int i = 0; i <= MAX_SLIDER; i++)
    {
      slider->setValue(i);
      test->showSlider(slider);
      printf("csliderhorizontal_main: %d. New value %d\n", i, slider->getValue());
      usleep(1000); // The simulation needs this to let the X11 event loop run
    }
  updateMemoryUsage(g_mmprevious, "csliderhorizontal_main: After moving the slider up");

  // And move the slider down

  for (int i = MAX_SLIDER; i >= 0; i--)
    {
      slider->setValue(i);
      test->showSlider(slider);
      printf("csliderhorizontal_main: %d. New value %d\n", i, slider->getValue());
      usleep(1000); // The simulation needs this to let the X11 event loop run
    }
  updateMemoryUsage(g_mmprevious, "csliderhorizontal_main: After moving the slider down");
  sleep(1);

  // Clean up and exit

  printf("csliderhorizontal_main: Clean-up and exit\n");
  delete slider;
  updateMemoryUsage(g_mmprevious, "After deleting the slider");
  delete test;
  updateMemoryUsage(g_mmprevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  return 0;
}
