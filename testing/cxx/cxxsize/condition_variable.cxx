//***************************************************************************
// apps/testing/cxx/cxxsize/condition_variable.cxx
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

#include <condition_variable>
#include <mutex>

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
// Name: condition_variable_main
//***************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  bool ready = false;
  std::condition_variable cv;
  std::mutex mtx;

  {
    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
  }

  cv.notify_one();

  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [&ready] { return ready; });

  return 0;
}
