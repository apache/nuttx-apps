/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CLatchButton/clatchbutton_main.cxx
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
#include <unistd.h>
#include <debug.h>

#include <nuttx/nx/nx.h>

#include "graphics/nxwidgets/clatchbuttontest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Classes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

static const char g_pushme[] = "Push Me";

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Suppress name-mangling

extern "C" int main(int argc, char *argv[]);

/////////////////////////////////////////////////////////////////////////////
// Private Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Name: showButtonState
/////////////////////////////////////////////////////////////////////////////

static void showButtonState(CLatchButton *button, bool &clicked, bool &latched)
{
  bool nowClicked = button->isClicked();
  bool nowLatched = button->isLatched();

  printf("showButtonState: Button state: %s and %s\n",
    nowClicked ? "clicked" : "released",
    nowLatched ? "latched" : "unlatched");

  if (clicked != nowClicked || latched != nowLatched)
    {
      printf("showButtonState: ERROR: Expected %s and %s\n",
        clicked ? "clicked" : "released",
        latched ? "latched" : "unlatched");

      clicked = nowClicked;
      latched = nowLatched;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// nxheaders_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Create an instance of the font test

  printf("clatchbutton_main: Create CLatchButtonTest instance\n");
  CLatchButtonTest *test = new CLatchButtonTest();

  // Connect the NX server

  printf("clatchbutton_main: Connect the CLatchButtonTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("clatchbutton_main: Failed to connect the CLatchButtonTest instance to the NX server\n");
      delete test;
      return 1;
    }

  // Create a window to draw into

  printf("clatchbutton_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("clatchbutton_main: Failed to create a window\n");
      delete test;
      return 1;
    }

  // Create a CLatchButton instance

  CLatchButton *button = test->createButton(g_pushme);
  if (!button)
    {
      printf("clatchbutton_main: Failed to create a button\n");
      delete test;
      return 1;
    }

  // Show the button

  printf("clatchbutton_main: Show the button\n");
  test->showButton(button);

  bool clicked = false;
  bool latched = false;
  showButtonState(button, clicked, latched);

  // Toggle the button state a few times

  for (int i = 0; i < 8; i++)
    {
      // Wait two seconds, then perform a simulated mouse click on the button

      sleep(2);
      printf("clatchbutton_main: Click the button\n");
      test->click();
      test->poll(button);

      // Test the button state it should be clicked with the latch state
      // toggled

      clicked = true;
      latched = !latched;
      showButtonState(button, clicked, latched);

      // And release the button after 0.5 seconds

      usleep(500 * 1000);
      printf("clatchbutton_main: Release the button\n");
      test->release();
      test->poll(button);

      // Test the button state it should be unclicked with the latch state
      // unchanged

      clicked = false;
      showButtonState(button, clicked, latched);
      fflush(stdout);
    }

  // Wait a few more seconds so that the tester can ponder the result

  sleep(3);

  // Clean up and exit

  printf("clatchbutton_main: Clean-up and exit\n");
  delete button;
  delete test;
  return 0;
}
