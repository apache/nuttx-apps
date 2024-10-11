//***************************************************************************
// apps/testing/cxxsize/rtti.cxx
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

#include <typeinfo>

//***************************************************************************
// Private Classes
//***************************************************************************

class Base
{
public:
  virtual ~Base() {}
};

class Derived : public Base {};

//***************************************************************************
// Private Data
//***************************************************************************

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: rtti_main
//***************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  Base* basePtr = new Derived();
  Derived* derivedPtr = dynamic_cast<Derived*>(basePtr);

  if (derivedPtr)
    {
      delete basePtr;
      return 0;
    }
  else
    {
      delete basePtr;
      return 1;
    }
}
