//- Copyright (c) 2010 James Grenning and Contributed to Unity Project
/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include <testing/unity_fixture.h>

// Enable for manual examination - tests will fail
//#define MANUAL_CHECKING

static void runAllTests(void)
{
    RUN_TEST_GROUP(UnityFixture);
    RUN_TEST_GROUP(UnityCommandOptions);
    RUN_TEST_GROUP(LeakDetection);
    RUN_TEST_GROUP(Timeout);
#ifdef MANUAL_CHECKING
    RUN_TEST_GROUP(TimeoutInSetup);
    RUN_TEST_GROUP(TimeoutInBody);
    RUN_TEST_GROUP(TimeoutInTearDown);
#endif
}

int unity_main(int argc, const char* argv[])
{
    return UnityMain(argc, argv, runAllTests);
}

