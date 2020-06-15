/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CKeypad/ckeypad_main.cxx
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
#include <malloc.h>
#include <unistd.h>
#include <debug.h>

#include <nuttx/nx/nx.h>

#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/ckeypadtest.hxx"

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
static unsigned int g_mmPrevious;
static unsigned int g_mmPeak;

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

  printf("%s: Before: %8d After: %8d Change: %8d\n",
         msg, previous, mmcurrent.uordblks, mmcurrent.uordblks - previous);

  /* Set up for the next test */

  g_mmPrevious = mmcurrent.uordblks;
  if ((unsigned int)mmcurrent.uordblks > g_mmPeak)
    {
      g_mmPeak = mmcurrent.uordblks;
    }
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
  g_mmPrevious = mmcurrent.uordblks;
  g_mmPeak     = mmcurrent.uordblks;
}

/////////////////////////////////////////////////////////////////////////////
// Name: clickButtons
/////////////////////////////////////////////////////////////////////////////

static void clickButtons(CKeypadTest *test, CKeypad *keypad)
{
  // Perform a simulated mouse click on a button in the keypad

  for (int j = 0; j < KEYPAD_NROWS; j++)
    {
      for (int i = 0; i < KEYPAD_NCOLUMNS; i++)
        {
          printf("clickButtons: Click the button (%d,%d)\n", i, j);
          test->click(keypad, i, j);

          // Poll for the mouse click event

          test->poll(keypad);

          // Is anything clicked?

          int clickColumn;
          int clickRow;
          if (keypad->isButtonClicked(clickColumn, clickRow))
            {
              printf("clickButtons: %s: Button (%d, %d) is clicked\n",
                     clickColumn == i && clickRow == j ? "OK" : "ERROR",
                     clickColumn, clickRow);
            }
          else
            {
              printf("clickButtons: ERROR: No button is clicked\n");
            }

          // Wait a bit, then release the mouse button

          usleep(250*1000);
          test->release(keypad, i, j);

          // Poll for the mouse release event (of course this can hang if something fails)

          test->poll(keypad);
          if (keypad->isButtonClicked(clickColumn, clickRow))
            {
              printf("clickButtons: ERROR: Button (%d, %d) is clicked\n",
                     clickColumn, clickRow);
            }

          usleep(500*1000);
        }
    }
  updateMemoryUsage(g_mmPrevious, "After pushing buttons");
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

  // Create an instance of the keypad test

  printf("ckeypad_main: Create CKeypadTest instance\n");
  CKeypadTest *test = new CKeypadTest();
  updateMemoryUsage(g_mmPrevious, "After creating CKeypadTest");

  // Connect the NX server

  printf("ckeypad_main: Connect the CKeypadTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("ckeypad_main: Failed to connect the CKeypadTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmPrevious, "After connecting to the server");

  // Create a window to draw into

  printf("ckeypad_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("ckeypad_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmPrevious, "After creating a window");

  // Create a CKeypad instance

  CKeypad *keypad = test->createKeypad();
  if (!keypad)
    {
      printf("ckeypad_main: Failed to create a keypad\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmPrevious, "After creating CKeypad");

  // Show the keypad in alphabetic mode

  printf("ckeypad_main: Show the keypad in alphabetic mode\n");
  keypad->setKeypadMode(false);
  test->showKeypad(keypad);
  sleep(1);

  // Then click some buttons

  clickButtons(test, keypad);
  sleep(1);

  // Show the keypad in numeric mode

  printf("ckeypad_main: Show the keypad in numeric mode\n");
  keypad->setKeypadMode(true);
  sleep(1);

  // Then click some buttons

  clickButtons(test, keypad);
  sleep(1);

  // Clean up and exit

  printf("ckeypad_main: Clean-up and exit\n");
  delete keypad;
  updateMemoryUsage(g_mmPrevious, "After deleting the keypad");
  delete test;
  updateMemoryUsage(g_mmPrevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  printf("Peak memory usage: %8d\n", g_mmPeak - g_mmInitial);
  return 0;
}
