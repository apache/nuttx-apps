/////////////////////////////////////////////////////////////////////////////
// apps/examples/elf/tests/helloxx/hello++4.cxx
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
// This is an excessively complex version of "Hello, World" design to
// illustrate some basic properties of C++:
//
// - Building a C++ program
// - Streams / statically linked libstdc++
// - Static constructor and destructors (in main program only)
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <iostream>

#ifndef NULL
# define NULL ((void*)0L)
#endif

/////////////////////////////////////////////////////////////////////////////
// Classes
/////////////////////////////////////////////////////////////////////////////

using namespace std;

// A hello world sayer class

class CThingSayer
{
  const char *szWhatToSay;
public:
  CThingSayer(void);
  virtual ~CThingSayer(void);
  virtual void Initialize(const char *czSayThis);
  virtual void SayThing(void);
};

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

// A static instance of the CThingSayer class.  This instance MUST
// be constructed by the system BEFORE the program is started at
// main() and must be destructed by the system AFTER the main()
// returns to the system

static CThingSayer MyThingSayer;

/////////////////////////////////////////////////////////////////////////////
// Method Implementations
/////////////////////////////////////////////////////////////////////////////

// These are implementations of the methods of the CThingSayer class

CThingSayer::CThingSayer(void)
{
  cout << "CThingSayer::CThingSayer: I am!" << endl;
  szWhatToSay = (const char*)NULL;
}

CThingSayer::~CThingSayer(void)
{
  cout << "CThingSayer::~CThingSayer: I cease to be" << endl;
  if (szWhatToSay)
    {
      cout << "CThingSayer::~CThingSayer: I will never say '"
	   << szWhatToSay << "' again" << endl;
    }
  szWhatToSay = (const char*)NULL;
}

void CThingSayer::Initialize(const char *czSayThis)
{
  cout << "CThingSayer::Initialize: When told, I will say '"
       << czSayThis << "'" << endl;
  szWhatToSay = czSayThis;
}

void CThingSayer::SayThing(void)
{
  cout << "CThingSayer::SayThing: I am now saying '"
       << szWhatToSay << "'" << endl;
}

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  // We should see the message from constructor, CThingSayer::CThingSayer(),
  // BEFORE we see the following messages.  That is proof that the
  // C++ static initializer is working

  cout << "main: Started" << endl;

  // Tell MyThingSayer that "Hello, World!" is the string to be said

  cout << "main: Calling MyThingSayer.Initialize" << endl;
  MyThingSayer.Initialize("Hello, World!");

  // Tell MyThingSayer to say the thing we told it to say

  cout << "main: Calling MyThingSayer.SayThing" << endl;
  MyThingSayer.SayThing();

  // We are finished, return.  We should see the message from the
  // destructor, CThingSayer::~CThingSayer(), AFTER we see the following
  // message.  That is proof that the C++ static destructor logic
  // is working

  cout << "main: Returning" << endl;
  return 0;
}
