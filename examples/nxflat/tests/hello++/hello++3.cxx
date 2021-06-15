/////////////////////////////////////////////////////////////////////////////
// apps/examples/nxflat/tests/hello++/hello++3.cxx
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
// - Building a C++ program to use the C library and stdio
// - Basic class creation with virtual methods.
// - Static constructor and destructors (in main program only)
// - NO Streams
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
  CThingSayer(void);
  virtual ~CThingSayer(void);
  virtual void Initialize(const char *czSayThis);
  virtual void SayThing(void);
};

// A static instance of the CThingSayer class.  This instance MUST
// be constructed by the system BEFORE the program is started at
// main() and must be destructed by the system AFTER the main()
// returns to the system

static CThingSayer MyThingSayer;

// These are implementations of the methods of the CThingSayer class

CThingSayer::CThingSayer(void)
{
  printf("CThingSayer::CThingSayer: I am!\n");
  szWhatToSay = (const char*)NULL;
}

CThingSayer::~CThingSayer(void)
{
  printf("CThingSayer::~CThingSayer: I cease to be\n");
  if (szWhatToSay)
    {
      printf("CThingSayer::~CThingSayer: I will never say '%s' again\n",
  	     szWhatToSay);
    }
  szWhatToSay = (const char*)NULL;
}

void CThingSayer::Initialize(const char *czSayThis)
{
  printf("CThingSayer::Initialize: When told, I will say '%s'\n",
	 czSayThis);
  szWhatToSay = czSayThis;
}

void CThingSayer::SayThing(void)
{
  printf("CThingSayer::SayThing: I am now saying '%s'\n", szWhatToSay);
}

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  // We should see the message from constructor, CThingSayer::CThingSayer(),
  // BEFORE we see the following messages.  That is proof that the
  // C++ static initializer is working

  printf("main: Started.  MyThingSayer should already exist\n");

  // Tell MyThingSayer that "Hello, World!" is the string to be said

  printf("main: Calling MyThingSayer.Initialize\n");
  MyThingSayer.Initialize("Hello, World!");

  // Tell MyThingSayer to say the thing we told it to say

  printf("main: Calling MyThingSayer.SayThing\n");
  MyThingSayer.SayThing();

  // We are finished, return.  We should see the message from the
  // destructor, CThingSayer::~CThingSayer(), AFTER we see the following
  // message.  That is proof that the C++ static destructor logic
  // is working

  printf("main: Returning.  MyThingSayer should be destroyed\n");
  return 0;
}
