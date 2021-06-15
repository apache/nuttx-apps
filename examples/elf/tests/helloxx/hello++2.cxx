/////////////////////////////////////////////////////////////////////////////
// apps/examples/elf/tests/helloxx/hello++2.cxx
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
/////////////////////////////////////////////////////////////////////////////
//
// This is an another trivial version of "Hello, World" design.  It illustrates
//
// - Building a C++ program to use the C library
// - Basic class creation
// - NO Streams
// - NO Static constructor and destructors
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdio>

/////////////////////////////////////////////////////////////////////////////
// Classes
/////////////////////////////////////////////////////////////////////////////

class CThingSayer
{
  const char *szWhatToSay;
public:
  CThingSayer(void)
    {
      printf("CThingSayer::CThingSayer: I am!\n");
      szWhatToSay = (const char*)NULL;
    }

  ~CThingSayer(void)
    {
      printf("CThingSayer::~CThingSayer: I cease to be\n");
      if (szWhatToSay)
	{
	  printf("CThingSayer::~CThingSayer: I will never say '%s' again\n",
		 szWhatToSay);
	}
      szWhatToSay = (const char*)NULL;
    }

  void Initialize(const char *czSayThis)
    {
      printf("CThingSayer::Initialize: When told, I will say '%s'\n",
	     czSayThis);
      szWhatToSay = czSayThis;
    }

  void SayThing(void)
    {
      printf("CThingSayer::SayThing: I am now saying '%s'\n", szWhatToSay);
    }
};

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  CThingSayer *MyThingSayer;

  printf("main: Started.  Creating MyThingSayer\n");

  // Create an instance of the CThingSayer class
  // We should see the message from constructor, CThingSayer::CThingSayer(),

  MyThingSayer = new CThingSayer;
  printf("main: Created MyThingSayer=0x%08lx\n", (long)MyThingSayer);

  // Tell MyThingSayer that "Hello, World!" is the string to be said

  printf("main: Calling MyThingSayer->Initialize\n");
  MyThingSayer->Initialize("Hello, World!");

  // Tell MyThingSayer to say the thing we told it to say

  printf("main: Calling MyThingSayer->SayThing\n");
  MyThingSayer->SayThing();

  // We should see the message from the destructor,
  // CThingSayer::~CThingSayer(), AFTER we see the following

  printf("main: Destroying MyThingSayer\n");
  delete MyThingSayer;

  printf("main: Returning\n");
  return 0;
}
