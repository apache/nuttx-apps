//***************************************************************************
// apps/testing/cxx/cxxsize/multiset.cxx
//
// SPDX-License-Identifier: Apache-2.0
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

#include <set>

//***************************************************************************
// Private Classes
//***************************************************************************

//***************************************************************************
// Private Data
//***************************************************************************

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: multiset_main
//***************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  std::multiset<int> s;

  s = { 1, 2, 2 };

  if (s.find(1) != s.end())
    {
      s.erase(1);
    }

  while (!s.empty())
    {
      s.erase(s.begin());
    }

  s.insert(4);
  s.clear();

  return 0;
}
