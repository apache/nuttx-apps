//***************************************************************************
// apps/examples/hello_d/hello_d_main.d
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
module examples.hello_d_main;

//***************************************************************************
// Imports module
//***************************************************************************
/// Type issues, need make own D runtime (object.d file)
// import core.stdc.stdio : printf;
// import core.stdc.stdlib : malloc, free;

version (D_BetterC) // no DRT
{
    /// if betterC and LDC, disable moduleinfo and typeinfo
    version (LDC)
    {
        pragma(LDC_no_moduleinfo);
        pragma(LDC_no_typeinfo);
    }
}

/// use --d-version=NuttX_ImportC to define
version (NuttX_ImportC)
{
    /// D compiler will not import C++ symbols
    /// so we need to import them manually
    /// @nogc, @trusted, @safe, nothrow - not allowed
    /// https://issues.dlang.org/show_bug.cgi?id=23812
    import nuttx_std : malloc, free, printf; // by default: @system (unsafe)
}
else
{
    /// Correctly FFI-import C/C++ symbols (@safe compatibility)
    ///
    pragma(printf)
    extern (C) int printf(scope const(char)* fmt, scope...) @nogc nothrow @trusted;
    ///
    extern (C) void* malloc(size_t size) @nogc nothrow @trusted;
    ///
    extern (C) void free(scope void* ptr) @nogc nothrow @trusted;
}

//***************************************************************************
// Private module content (default is public)
//***************************************************************************
private:

// based heloxx class layout
extern (C++,class)
struct DHelloWorld
{
    @disable this();
    @disable this(this);

    /// use --d-version=NuttX_ImportC to define
    version (NuttX_ImportC)
    {
        this(int secret)
        {
            mSecret = secret;
            printf("Constructor\n");
        }

        ~this()
        {
            printf("Destructor\n");
        }

        bool HelloWorld()
        {
            printf("HelloWorld: mSecret=%d\n", mSecret);

            if (mSecret != 42)
            {
                printf("DHelloWorld.HelloWorld: CONSTRUCTION FAILED!\n");
                return false;
            }
            else
            {
                printf("DHelloWorld.HelloWorld: Hello, World!!\n");
                return true;
            }
        }
    }
    else
    {
        this(int secret) @safe nothrow @nogc
        {
            mSecret = secret;
            printf("Constructor\n");
        }

        ~this() @safe nothrow @nogc
        {
            printf("Destructor\n");
        }

        bool HelloWorld() @safe nothrow @nogc
        {
            printf("HelloWorld: mSecret=%d\n", mSecret);

            if (mSecret != 42)
            {
                printf("DHelloWorld.HelloWorld: CONSTRUCTION FAILED!\n");
                return false;
            }
            else
            {
                printf("DHelloWorld.HelloWorld: Hello, World!!\n");
                return true;
            }
        }
    }
    private int mSecret = 0;
}

/****************************************************************************
 * Name: hello_d_main
 ****************************************************************************/
// betterC need main function no-mangle.
extern (C) int hello_d_main(int argc, char*[] argv)
{
    version (LDC)
    {
        /// need LLVM targetinfo
        printf("Hello World, [%s]!\n", __traits(targetCPU).ptr);
    }
    // Exercise an explicitly instantiated C++ object
    auto pHelloWorld = cast(DHelloWorld*) malloc(DHelloWorld.sizeof);
    scope (exit)
        free(pHelloWorld);

    printf("hello_d_main: Saying hello from the dynamically constructed instance\n");
    pHelloWorld.HelloWorld();

    // Exercise an D object instantiated on the stack
    auto HelloWorld = DHelloWorld(42);

    printf("hello_d_main: Saying hello from the instance constructed on the stack\n");
    HelloWorld.HelloWorld();

    return 0;
}
