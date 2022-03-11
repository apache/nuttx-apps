//***************************************************************************
// apps/examples/helloxx/helloxx_main.cxx
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
//***************************************************************************

//***************************************************************************
// Included Files
//***************************************************************************

#include <nuttx/config.h>

#include <cstdio>
#include <debug.h>

//***************************************************************************
// Definitions
//***************************************************************************
// Configuration ************************************************************

// Debug ********************************************************************
// Non-standard debug that may be enabled just for testing the constructors

#ifndef CONFIG_DEBUG_FEATURES
#  undef CONFIG_DEBUG_CXX
#endif

#ifdef CONFIG_DEBUG_CXX
#  define cxxinfo     _info
#else
#  define cxxinfo(x...)
#endif

//***************************************************************************
// Private Classes
//***************************************************************************

class CHelloWorld
{
  public:
    CHelloWorld(void) : mSecret(42)
    {
      cxxinfo("Constructor: mSecret=%d\n", mSecret);
    }

    ~CHelloWorld(void)
    {
      cxxinfo("Destructor\n");
    }

    bool HelloWorld(void)
    {
        cxxinfo("HelloWorld: mSecret=%d\n", mSecret);

        if (mSecret != 42)
          {
            printf("CHelloWorld::HelloWorld: CONSTRUCTION FAILED!\n");
            return false;
          }
        else
          {
            printf("CHelloWorld::HelloWorld: Hello, World!!\n");
            return true;
          }
    }

  private:
    int mSecret;
};

//***************************************************************************
// Private Data
//***************************************************************************

// Define a statically constructed CHellowWorld instance if C++ static
// initializers are supported by the platform

#ifdef CONFIG_HAVE_CXXINITIALIZE
static CHelloWorld g_HelloWorld;
#endif

//***************************************************************************
// Public Functions
//***************************************************************************

/****************************************************************************
 * Name: helloxx_main
 ****************************************************************************/


extern "C" int main(int argc, FAR char *argv[])
{
  // Exercise an explicitly instantiated C++ object

  CHelloWorld *pHelloWorld = new CHelloWorld;
  printf("helloxx_main: Saying hello from the dynamically constructed instance\n");
  pHelloWorld->HelloWorld();

  // Exercise an C++ object instantiated on the stack

  CHelloWorld HelloWorld;

  printf("helloxx_main: Saying hello from the instance constructed on the stack\n");
  HelloWorld.HelloWorld();

  // Exercise an statically constructed C++ object

#ifdef CONFIG_HAVE_CXXINITIALIZE
  printf("helloxx_main: Saying hello from the statically constructed instance\n");
  g_HelloWorld.HelloWorld();
#endif

  delete pHelloWorld;
  return 0;
}
