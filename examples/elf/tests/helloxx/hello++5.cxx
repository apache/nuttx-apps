/////////////////////////////////////////////////////////////////////////////
// apps/examples/elf/tests/helloxx/hello++5.cxx
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
// - Exception handling
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <iostream>
#include <string>

#ifndef NULL
# define NULL ((void*)0L)
#endif

/////////////////////////////////////////////////////////////////////////////
// Private Classes
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
  virtual void ThrowThing(void);
  virtual void ThrowMyThing(const char *czSayThis);
};

class MyException: public std::exception
{
  std::string reason;

public:
  virtual ~MyException() throw();
  MyException();
  MyException(std::string r);

  virtual const char *what() const throw();
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
// MyException Method Implementations
/////////////////////////////////////////////////////////////////////////////

MyException::MyException(std::string r = NULL) : reason(r)
{
  if( r.length() > 0 )
    {
      cout << "MyException(" << r << "):  constructing with reason." << endl;
    }
  else
    {
      cout << "MyException():  constructing." << endl;
    }
}

MyException::~MyException() throw()
{
  cout << "~MyException():  destructing." << endl;
}

const char *MyException::what() const throw()
{
  return reason.c_str();
}

/////////////////////////////////////////////////////////////////////////////
// CThingSayer Method Implementations
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

void CThingSayer::ThrowThing(void)
{
  cout << "CThingSayer::ThrowThing: I am now throwing an exception." << endl;
  throw exception();
}

void CThingSayer::ThrowMyThing(const char *czSayThis = NULL)
{
  cout << "CThingSayer::ThrowMyThing: I am now throwing an MyException (with reason)." << endl;
  throw MyException( string(czSayThis) );
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

  // Test Static object throwing std::exception located in flash

  try
    {
      cout << "main: Calling MyThingSayer.ThrowThing (and catching it)" << endl;
      MyThingSayer.ThrowThing();
    }

  catch (const exception& e)
    {
      cout << "main: Caught exception: " << e.what() << endl;
    }

  // Test Static object throwing MyException shipped in the ELF and located in RAM.

  try
    {
      cout << "main: Calling MyThingSayer.ThrowMyThing (and catching it)" << endl;
      MyThingSayer.ThrowMyThing();
    }

  catch (const MyException& e)
    {
      cout << "main: Caught MyException: " << e.what() << endl;
    }

  // Testing with a local object

  CThingSayer localMyThingSayer;

  cout << "main: Calling localMyThingSayer.Initialize" << endl;
  localMyThingSayer.Initialize("Repeat in heap.");
  cout << "main: Calling localMyThingSayer.SayThing" << endl;
  localMyThingSayer.SayThing();

  // Test local object throwing std::exception located in flash

  try
    {
      cout << "main: Calling localMyThingSayer.ThrowThing (and catching it)" << endl;
      localMyThingSayer.ThrowThing();
    }

  catch (const exception& e)
    {
      cout << "main: Caught exception: " << e.what() << endl;
    }

  catch (const MyException& e)
    {
      cout << "main: Caught MyException: " << e.what() << endl;
    }

  // Test local object throwing MyException shipped in the ELF and located in RAM.
  // Also testing the action record selection logic trying different sorting of the
  // catch clauses

  try
    {
      cout << "main: Calling localMyThingSayer.ThrowMyThing (and catching it)" << endl;
      localMyThingSayer.ThrowMyThing();
    }

  catch (const exception& e)
    {
      cout << "main: Caught exception: " << e.what() << endl;
    }

  catch (const MyException& e)
    {
      cout << "main: Caught MyException: " << e.what() << endl;
    }

  // AGAING: Test local object throwing MyException shipped in the ELF and located in RAM.
  // Also testing the action record selection logic trying different sorting of the
  // catch clauses

  try
    {
      cout << "main: Calling localMyThingSayer.ThrowMyThing (and catching it)" << endl;
      localMyThingSayer.ThrowMyThing("Custom Reason.");
    }

  catch (const MyException& e)
    {
      cout << "main: Caught MyException: " << e.what() << endl;
    }

  catch (const exception& e)
    {
      cout << "main: Caught exception: " << e.what() << endl;
    }

  // We are finished, return.  We should see the message from the
  // destructor, CThingSayer::~CThingSayer(), AFTER we see the following
  // message.  That is proof that the C++ static destructor logic
  // is working

  cout << "main: Returning" << endl;
  return 0;
}
