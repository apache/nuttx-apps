README
^^^^^^

This folder hosts NuttX unit test framework source files.

INTRODUCTION
^^^^^^^^^^^^

NuttX unit test framework is based on Unity (https://github.com/ThrowTheSwitch/Unity).
The source code bears minimal changes and most of the additional functionality is brought
through the extension.

NUTTX-SPECIFIC ISSUES
^^^^^^^^^^^^^^^^^^^^^

Unity relies on setjmp/longjmp to be available, so a testcase can be aborted. While the
functionality exists in simulator builds, the hardware environment does not have it. To
provide the same baseline we run test bodies and other abortable parts in their own threads.
The main thread blocks until a spawn exits. Thus we create the same context as offered by
setjmp/longjmp.

RUNNING TESTS
^^^^^^^^^^^^^

Tests can be executed both in simulator and in the target. The test application can be built-in
into NSH or provide application entry point.

Test application supports command line interface:

    application [--list] [test [test...]]

        --list - Lists all available testcases from all included modules

Test is identified by its module name and function name.
