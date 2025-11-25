/****************************************************************************
 * testing/cxx-oot-build/src/HelloWorld.cpp
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#include <cstdio>
#include <string>

#include "HelloWorld.hpp"

CHelloWorld::CHelloWorld()
{
    mSecret = 42;
    std::printf("Constructor: mSecret=%d\n",mSecret);
}


bool CHelloWorld::HelloWorld()
{
    std::printf("HelloWorld: mSecret=%d\n",mSecret);

    std::string sentence = "Hello";
    std::printf("TEST=%s\n",sentence.c_str());

    if (mSecret == 42)
    {
            std::printf("CHelloWorld: HelloWorld: Hello, world!\n");
            return true;
    }
    else
    {
            std::printf("CHelloWorld: HelloWorld: CONSTRUCTION FAILED!\n");
            return false;
    }
}
