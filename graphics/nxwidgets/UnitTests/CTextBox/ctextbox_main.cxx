/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CTextBox/ctextbox_main.cxx
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

#include "graphics/nxwidgets/ctextboxtest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Classes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

static const char string1[] = "Johhn ";
static const char string2[] = "\b\b\bn Doe\r";

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

  printf("ctextbox_main: Create CTextBoxTest instance\n");
  CTextBoxTest *test = new CTextBoxTest();

  // Connect the NX server

  printf("ctextbox_main: Connect the CTextBoxTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("ctextbox_main: Failed to connect the CTextBoxTest instance to the NX server\n");
      delete test;
      return 1;
    }

  // Create a window to draw into

  printf("ctextbox_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("ctextbox_main: Failed to create a window\n");
      delete test;
      return 1;
    }

  // Create a CTextBox instance

  CTextBox *textbox = test->createTextBox();
  if (!textbox)
    {
      printf("ctextbox_main: Failed to create a text box\n");
      delete test;
      return 1;
    }

  // Show the text box

  test->showTextBox(textbox);

  // Wait a bit, then inject a string with a typo

  sleep(1);
  test->injectChars(textbox, sizeof(string1), (FAR const uint8_t*)string1);

  // Now fix the string with backspaces and finish it correctly

  usleep(500*1000);
  test->injectChars(textbox, sizeof(string2), (FAR const uint8_t*)string2);

  // Clean up and exit

  sleep(2);
  printf("ctextbox_main: Clean-up and exit\n");
  delete textbox;
  delete test;
  return 0;
}
