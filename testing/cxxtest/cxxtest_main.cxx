//***************************************************************************
// testing/main.cxx
//
//   Copyright (C) 2012, 2017 Gregory Nutt. All rights reserved.
//   Author: Qiang Yu, http://rgmp.sourceforge.net/wiki/index.php/Main_Page
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
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
