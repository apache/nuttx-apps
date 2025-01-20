//***************************************************************************
// apps/testing/cxx/cxxsize/weak_ptr.cxx
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

#include <memory>

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
// Name: weak_ptr_main
//***************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  std::weak_ptr<int> weak_ptr;
  std::shared_ptr<int> shared_ptr(new int(10));

  weak_ptr = shared_ptr;

  if (weak_ptr.expired())
    {
      return 1;
    }

  if (weak_ptr.lock() != shared_ptr)
    {
      return 1;
    }

  shared_ptr.reset();

  if (weak_ptr.use_count())
    {
      return 1;
    }

  return 0;
}
