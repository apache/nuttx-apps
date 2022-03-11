/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CButton/cbutton_main.cxx
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

#include "graphics/nxwidgets/cbuttontest.hxx"

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
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// nxheaders_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Create an instance of the font test

  printf("cbutton_main: Create CButtonTest instance\n");
  CButtonTest *test = new CButtonTest();

  // Connect the NX server

  printf("cbutton_main: Connect the CButtonTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("cbutton_main: Failed to connect the CButtonTest instance to the NX server\n");
      delete test;
      return 1;
    }

  // Create a window to draw into

  printf("cbutton_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("cbutton_main: Failed to create a window\n");
      delete test;
      return 1;
    }

  // Create a CButton instance

  CButton *button = test->createButton(g_pushme);
  if (!button)
    {
      printf("cbutton_main: Failed to create a button\n");
      delete test;
      return 1;
    }

  // Show the button

  printf("cbutton_main: Show the button\n");
  test->showButton(button);

  // Wait two seconds, then perform a simulated mouse click on the button

  sleep(2);
  printf("cbutton_main: Click the button\n");
  test->click();

  // Poll for the mouse click event (of course this can hang if something fails)

  bool clicked = test->poll(button);
  printf("cbutton_main: Button is %s\n", clicked ? "clicked" : "released");

  // Wait a second, then release the mouse buttone

  sleep(1);
  test->release();

  // Poll for the mouse release event (of course this can hang if something fails)

  clicked = test->poll(button);
  printf("cbutton_main: Button is %s\n", clicked ? "clicked" : "released");

  // Wait a few more seconds so that the tester can ponder the result

  sleep(3);

  // Clean up and exit

  printf("cbutton_main: Clean-up and exit\n");
  delete button;
  delete test;
  return 0;
}
