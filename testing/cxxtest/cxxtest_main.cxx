//***************************************************************************
// apps/testing/main.cxx
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
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <stdexcept>
#include <cassert>

using namespace std;

//***************************************************************************
// Private Classes
//***************************************************************************

class Base
{
public:
  virtual void printBase(void) {}
  virtual ~Base() {}
};

class Extend : public Base
{
public:
  void printExtend(void)
  {
    std::cout << "extend" << std::endl;
  }
};

//***************************************************************************
// Private Data
//***************************************************************************

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: test_ostream
//***************************************************************************/

static void test_ofstream(void)
{
  std::ofstream ttyOut;

  std::cout << "test ofstream===========================" << std::endl;
  std::printf("printf: Starting test_ostream\n");
  ttyOut.open ("/dev/console");
  if (!ttyOut.good())
    {
      std::printf("printf: Failed opening /dev/console\n");
      std::cout << "cout: Failed opening /dev/console" << std::endl;
      std::cout << " good()=" << ttyOut.good();
      std::cout << " eof()=" << ttyOut.eof();
      std::cout << " fail()=" << ttyOut.fail();
      std::cout << " bad()=" << ttyOut.bad() << std::endl;
    }
  else
    {
      std::printf("printf: Successfully opened /dev/console\n");
      std::cout << "cout: Successfully opened /dev/console" << std::endl;
      ttyOut << "Writing this to /dev/console\n";
      ttyOut.close();
    }
}

//***************************************************************************
// Name: test_iostream
//***************************************************************************/

static void test_iostream(void)
{
  std::cout << "test iostream===========================" << std::endl;
  std::cout << "Hello, this is only a test" << std::endl;
  std::cout << "Print an int: "  <<  190  <<  std::endl;
  std::cout <<  "Print a char: "  <<  'd'  <<  std::endl;

#if 0
  int a;
  string s;

  std::cout << "Please type in an int:" << std::endl;
  std::cin >> a;
  std::cout << "You type in: " << a << std::endl;
  std::cout << "Please type in a string:" << std::endl;
  std::cin >> s;
  std::cout << "You type in: " << s << std::endl;
#endif
}

//***************************************************************************
// Name: test_stl
//***************************************************************************/

static void test_stl(void)
{
  std::cout << "test vector=============================" << std::endl;

  vector<int> v1;
  assert(v1.empty());

  v1.push_back(1);
  assert(!v1.empty());

  v1.push_back(2);
  v1.push_back(3);
  v1.push_back(4);
  assert(v1.size() == 4);

  v1.pop_back();
  assert(v1.size() == 3);

  std::cout << "v1=" << v1[0] << ' ' << v1[1] << ' ' << v1[2] << std::endl;
  assert(v1[2] == 3);

  vector<int> v2 = v1;
  assert(v2 == v1);

  string words[4] = {"Hello", "World", "Good", "Luck"};
  vector<string> v3(words, words + 4);
  vector<string>::iterator it;
  for (it = v3.begin(); it != v3.end(); ++it)
    {
      std::cout << *it << ' ';
    }

  std::cout << std::endl;
  assert(v3[1] == "World");

  std::cout << "test map================================" << std::endl;

  map<int,string> m1;
  m1[12] = "Hello";
  m1[24] = "World";
  assert(m1.size() == 2);
  assert(m1[24] == "World");
}

//***************************************************************************
// Name: test_rtti
//***************************************************************************/

static void test_rtti(void)
{
  std::cout << "test rtti===============================" << std::endl;
  Base *a = new Base();
  Base *b = new Extend();
  assert(a);
  assert(b);

  Extend *t = dynamic_cast<Extend *>(a);
  assert(t == NULL);

  t = dynamic_cast<Extend *>(b);
  assert(t);
  t->printExtend();

  delete a;
  delete b;
}

//***************************************************************************
// Name: test_exception
//***************************************************************************/

#ifdef CONFIG_CXX_EXCEPTION
static void test_exception(void)
{
  std::cout << "test exception==========================" << std::endl;
  try
  {
    throw runtime_error("runtime error");
  }

  catch (runtime_error &e)
  {
    std::cout << "Catch exception: " << e.what() << std::endl;
  }
}
#endif

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: cxxtest_main
//***************************************************************************/

extern "C"
{
  int main(int argc, char *argv[])
  {
    test_ofstream();
    test_iostream();
    test_stl();
    test_rtti();
#ifdef CONFIG_CXX_EXCEPTION
    test_exception();
#endif

    return 0;
  }
}
