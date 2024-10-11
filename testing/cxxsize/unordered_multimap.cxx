//***************************************************************************
// apps/testing/cxxsize/unordered_multimap.cxx
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

#include <unordered_map>

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
// Name: unordered_multimap_main
//***************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  std::unordered_multimap<int, int> m;

  m = {{ 1, 1 }, { 2, 2 }, { 2, 3 }};

  if (m.find(1) != m.end())
    {
      m.erase(1);
    }

  while (!m.empty())
    {
      m.erase(m.begin());
    }

  m.insert({4, 4});
  m.clear();

  return 0;
}
