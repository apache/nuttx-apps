//***************************************************************************
// apps/examples/etl/etl_main.cxx
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

#include <etl/vector.h>
#include <etl/numeric.h>

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

typedef etl::vector<int, 10> Data;

//***************************************************************************
// Private Classes
//***************************************************************************

class CETL
{
  public:
    CETL(void) : mSecret(42)
    {
      cxxinfo("Constructor: mSecret=%d\n", mSecret);
    }

    ~CETL(void)
    {
      cxxinfo("Destructor\n");
    }

    bool HelloWorld(void)
    {
        cxxinfo("HelloWorld: mSecret=%d\n", mSecret);

        // Declare the vector instances.
        etl::vector<int, 10> v1(10);
        etl::vector<int, 20> v2(20);

        etl::iota(v1.begin(), v1.end(), 0);  // Fill with 0 through 9
        etl::iota(v2.begin(), v2.end(), 10); // Fill with 10 through 29

        printf("v1 with 0 through 9\n");
        for (auto& it : v1) {
            printf("%i ", it);
        }    
        printf("\n");

        printf("v2 with 10 through 29\n");
        for (auto& it : v2) {
            printf("%i ", it);
        }
        printf("\n");

        return true;
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
static CETL g_HelloWorld;
#endif

//***************************************************************************
// Public Functions
//***************************************************************************

/****************************************************************************
 * Name: etl_main
 ****************************************************************************/


extern "C" int main(int argc, FAR char *argv[])
{
  // Exercise an explicitly instantiated C++ object

  CETL *pHelloWorld = new CETL;
  printf("etl_main: Testing ETL from the dynamically constructed instance\n");
  pHelloWorld->HelloWorld();

  // Exercise an C++ object instantiated on the stack

  CETL HelloWorld;

  printf("etl_main: Testing ETL from the instance constructed on the stack\n");
  HelloWorld.HelloWorld();

  // Exercise an statically constructed C++ object

#ifdef CONFIG_HAVE_CXXINITIALIZE
  printf("etl_main: Testing ETL from the statically constructed instance\n");
  g_HelloWorld.HelloWorld();
#endif

  delete pHelloWorld;
  return 0;
}
