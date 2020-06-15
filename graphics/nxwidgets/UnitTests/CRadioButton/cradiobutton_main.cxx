/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CRadioButton/cradiobutton_main.cxx
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
#include "graphics/nxwidgets/cradiobuttontest.hxx"

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

  // Create an instance of the radio button test

  printf("cradiobutton_main: Create CRadioButtonTest instance\n");
  CRadioButtonTest *test = new CRadioButtonTest();
  updateMemoryUsage(g_mmprevious, "After creating CRadioButtonTest");

  // Connect the NX server

  printf("cradiobutton_main: Connect the CRadioButtonTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("cradiobutton_main: Failed to connect the CRadioButtonTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cradiobutton_main: After connecting to the server");

  // Create a window to draw into

  printf("cradiobutton_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("cradiobutton_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cradiobutton_main: After creating a window");

  // Create three radio buttons

  CRadioButton *button1 = test->newRadioButton();
  if (!button1)
    {
      printf("cradiobutton_main: Failed to create radio button 1\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cradiobutton_main: After creating radio button 1");

  CRadioButton *button2 = test->newRadioButton();
  if (!button2)
    {
      printf("cradiobutton_main: Failed to create radio button 2\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cradiobutton_main: After creating radio button 2");

  CRadioButton *button3 = test->newRadioButton();
  if (!button3)
    {
      printf("cradiobutton_main: Failed to create radio button 3\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmprevious, "cradiobutton_main: After creating radio button 3");

  // Show the initial state of the buttons

  test->showButtons();
  test->showButtonState();
  sleep(1);

  // Now push some buttons

  printf("cradiobutton_main: Pushing button 1\n");
  test->pushButton(button1);
  usleep(500*1000);
  test->showButtonState();
  updateMemoryUsage(g_mmprevious, "After pushing button 1");
  usleep(500*1000);

  printf("cradiobutton_main: Pushing button 2\n");
  test->pushButton(button2);
  usleep(500*1000);
  test->showButtonState();
  updateMemoryUsage(g_mmprevious, "After pushing button 2");
  usleep(500*1000);

  printf("cradiobutton_main: Pushing button 3\n");
  test->pushButton(button3);
  usleep(500*1000);
  test->showButtonState();
  updateMemoryUsage(g_mmprevious, "After pushing button 3");
  sleep(2);

  // Clean up and exit

  printf("cradiobutton_main: Clean-up and exit\n");
  delete test;
  updateMemoryUsage(g_mmprevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  return 0;
}
