/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CLabel/clabel_main.cxx
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

#include "graphics/nxwidgets/clabeltest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Classes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

static const char g_hello[] = "Hello, World!";

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

  printf("clabel_main: Create CLabelTest instance\n");
  CLabelTest *test = new CLabelTest();

  // Connect the NX server

  printf("clabel_main: Connect the CLabelTest instance to the NX server\n");
  if (!test->connect())
    {
      printf("clabel_main: Failed to connect the CLabelTest instance to the NX server\n");
      delete test;
      return 1;
    }

  // Create a window to draw into

  printf("clabel_main: Create a Window\n");
  if (!test->createWindow())
    {
      printf("clabel_main: Failed to create a window\n");
      delete test;
      return 1;
    }

  // Create a CLabel instance

  CLabel *label = test->createLabel(g_hello);
  if (!label)
    {
      printf("clabel_main: Failed to create a label\n");
      delete test;
      return 1;
    }

  // Show the label

  test->showLabel(label);
  sleep(5);

  // Clean up and exit

  printf("clabel_main: Clean-up and exit\n");
  delete label;
  delete test;
  return 0;
}
