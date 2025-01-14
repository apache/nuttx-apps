//***************************************************************************
// apps/testing/cxx/cxxsize/forward_list.cxx
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

#include <forward_list>

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
// Name: forward_list_main
//***************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  std::forward_list<int> l;

  l.push_front(1);
  l.push_front(2);
  l.push_front(3);
  l.pop_front();

  l.insert_after(l.begin(), 4);
  l.erase_after(l.begin());

  l.clear();

  return 0;
}
