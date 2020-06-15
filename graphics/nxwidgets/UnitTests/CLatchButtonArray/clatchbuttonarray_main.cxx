/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CLatchButtonArray/clatchbuttonarry_main.cxx
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
#include "graphics/nxwidgets/clatchbuttonarraytest.hxx"

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

static FAR const char *g_buttonLabels[BUTTONARRAY_NCOLUMNS*BUTTONARRAY_NROWS] = {
 "=>", "A", "B", "<DEL",
 "C", "D", "E", "F",
 "G", "H", "I", "J",
 "K", "L", "M", "N",
 "O", "P", "Q", "R",
 "S", "T", "U", "V",
 "W", "X", "Y", "Z"
};

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

static void showButtonState(CLatchButtonArray *buttonArray, int i, int j,
                            bool &clicked, bool &latched)
{
  bool nowClicked = buttonArray->isThisButtonClicked(i,j);
  bool nowLatched = buttonArray->isThisButtonLatched(i,j);

  printf("showButtonState: Button(%d,%d) state: %s and %s\n",
    i, j,
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
// Public Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// nxheaders_main
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  // Initialize memory monitor logic

  initMemoryUsage();

  // Create an instance of the button array test

  printf("clatchbuttonarray_main: Create CLatchButtonArrayTest instance\n");
  CLatchButtonArrayTest *test = new CLatchButtonArrayTest();
  updateMemoryUsage(g_mmPrevious, "After creating CLatchButtonArrayTest");

  // Connect the NX server

  printf("clatchbuttonarray_main: Connect the CLatchButtonArrayTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("clatchbuttonarray_main: Failed to connect the CLatchButtonArrayTest instance to the NX server\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmPrevious, "After connecting to the server");

  // Create a window to draw into

  printf("clatchbuttonarray_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("clatchbuttonarray_main: Failed to create a window\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmPrevious, "After creating a window");

  // Create a CLatchButtonArray instance

  CLatchButtonArray *buttonArray = test->createButtonArray();
  if (!buttonArray)
    {
      printf("clatchbuttonarray_main: Failed to create a button array\n");
      delete test;
      return 1;
    }
  updateMemoryUsage(g_mmPrevious, "After creating CLatchButtonArray");

  // Add the labels to each button

  FAR const char **ptr = g_buttonLabels;
  for (int j = 0; j < BUTTONARRAY_NROWS; j++)
    {
      for (int i = 0; i < BUTTONARRAY_NCOLUMNS; i++)
        {
          printf("clatchbuttonarray_main: Label (%d,%d): %s\n", i, j, *ptr);
          CNxString string = *ptr++;
          buttonArray->setText(i, j, string);
        }
    }
  updateMemoryUsage(g_mmPrevious, "After adding labels to the buttons");

  // Show the button array

  printf("clatchbuttonarray_main: Show the button array\n");
  test->showButton(buttonArray);
  sleep(1);

  // Then perform a simulated mouse click on a button in the array

  bool clicked = false;
  bool latched = false;

  for (int j = 0; j < BUTTONARRAY_NROWS; j++)
    {
      for (int i = 0; i < BUTTONARRAY_NCOLUMNS; i++)
        {
          // Initially, this button should be neither clicked nor latched

          clicked = false;
          latched = false;
          showButtonState(buttonArray, i, j, clicked, latched);

          printf("clatchbuttonarray_main: Click the button (%d,%d)\n", i, j);
          test->click(buttonArray, i, j);

          // Poll for the mouse click event

          test->poll(buttonArray);

          // Now it should be clicked and latched

          clicked = true;
          latched = true;
          showButtonState(buttonArray, i, j, clicked, latched);

          // Wait a bit, then release the mouse button

          usleep(200*1000);
          test->release(buttonArray, i, j);

          // Poll for the mouse release event (of course this can hang if something fails)

          test->poll(buttonArray);

          // Now it should be un-clicked and latched

          clicked = false;
          latched = true;
          showButtonState(buttonArray, i, j, clicked, latched);

          usleep(300*1000);
        }
    }
  updateMemoryUsage(g_mmPrevious, "After pushing buttons");

  // Clean up and exit

  printf("clatchbuttonarray_main: Clean-up and exit\n");
  delete buttonArray;
  updateMemoryUsage(g_mmPrevious, "After deleting the button array");
  delete test;
  updateMemoryUsage(g_mmPrevious, "After deleting the test");
  updateMemoryUsage(g_mmInitial, "Final memory usage");
  printf("Peak memory usage: %8d\n", g_mmPeak - g_mmInitial);
  return 0;
}
