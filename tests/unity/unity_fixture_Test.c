//- Copyright (c) 2010 James Grenning and Contributed to Unity Project
/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include <testing/unity_fixture.h>
#include "unity_output_Spy.h"
#include <stdlib.h>
#include <string.h>

extern UNITY_FIXTURE_T UnityFixture;

TEST_GROUP(UnityFixture);

TEST_SETUP(UnityFixture)
{
}

TEST_TEAR_DOWN(UnityFixture)
{
}

int my_int;
int* pointer1 = 0;
int* pointer2 = (int*)2;
int* pointer3 = (int*)3;
int int1;
int int2;
int int3;
int int4;

TEST(UnityFixture, PointerSetting)
{
    TEST_ASSERT_POINTERS_EQUAL(pointer1, 0);
    UT_PTR_SET(pointer1, &int1);
    UT_PTR_SET(pointer2, &int2);
    UT_PTR_SET(pointer3, &int3);
    TEST_ASSERT_POINTERS_EQUAL(pointer1, &int1);
    TEST_ASSERT_POINTERS_EQUAL(pointer2, &int2);
    TEST_ASSERT_POINTERS_EQUAL(pointer3, &int3);
    UT_PTR_SET(pointer1, &int4);
    UnityPointer_UndoAllSets();
    TEST_ASSERT_POINTERS_EQUAL(pointer1, 0);
    TEST_ASSERT_POINTERS_EQUAL(pointer2, (int*)2);
    TEST_ASSERT_POINTERS_EQUAL(pointer3, (int*)3);
}

TEST(UnityFixture, ForceMallocFail)
{
    void* m;
    void* mfails;
    UnityMalloc_MakeMallocFailAfterCount(1);
    m = malloc(10);
    CHECK(m);
    mfails = malloc(10);
    TEST_ASSERT_POINTERS_EQUAL(0, mfails);
    free(m);
}

TEST(UnityFixture, ReallocSmallerIsUnchanged)
{
    void* m1 = malloc(10);
    void* m2 = realloc(m1, 5);
    TEST_ASSERT_POINTERS_EQUAL(m1, m2);
    free(m2);
}

TEST(UnityFixture, ReallocSameIsUnchanged)
{
    void* m1 = malloc(10);
    void* m2 = realloc(m1, 10);
    TEST_ASSERT_POINTERS_EQUAL(m1, m2);
    free(m2);
}

TEST(UnityFixture, ReallocLargerNeeded)
{
    void* m1 = malloc(10);
    void* m2;
    strcpy((char*)m1, "123456789");
    m2 = realloc(m1, 15);
    CHECK(m1 != m2);
    STRCMP_EQUAL("123456789", m2);
    free(m2);
}

TEST(UnityFixture, ReallocNullPointerIsLikeMalloc)
{
    void* m = realloc(0, 15);
    CHECK(m != 0);
    free(m);
}

TEST(UnityFixture, ReallocSizeZeroFreesMemAndReturnsNullPointer)
{
    void* m1 = malloc(10);
    void* m2 = realloc(m1, 0);
    TEST_ASSERT_POINTERS_EQUAL(0, m2);
}

TEST(UnityFixture, CallocFillsWithZero)
{
    void* m = calloc(3, sizeof(char));
    char* s = (char*)m;
    TEST_ASSERT_BYTES_EQUAL(0, s[0]);
    TEST_ASSERT_BYTES_EQUAL(0, s[1]);
    TEST_ASSERT_BYTES_EQUAL(0, s[2]);
    free(m);
}

char *p1;
char *p2;

TEST(UnityFixture, PointerSet)
{
    char c1;
    char c2;
    char newC1;
    char newC2;
    p1 = &c1;
    p2 = &c2;

    UnityPointer_Init();
    UT_PTR_SET(p1, &newC1);
    UT_PTR_SET(p2, &newC2);
    TEST_ASSERT_POINTERS_EQUAL(&newC1, p1);
    TEST_ASSERT_POINTERS_EQUAL(&newC2, p2);
    UnityPointer_UndoAllSets();
    TEST_ASSERT_POINTERS_EQUAL(&c1, p1);
    TEST_ASSERT_POINTERS_EQUAL(&c2, p2);
}

//------------------------------------------------------------

TEST_GROUP(UnityCommandOptions);

static int savedVerbose;
static int savedRepeat;
static int savedListOnly;
static const char* savedName;
static const char* savedGroup;
static const char** savedList;

TEST_SETUP(UnityCommandOptions)
{
    savedVerbose = UnityFixture.Verbose;
    savedRepeat = UnityFixture.RepeatCount;
    savedListOnly = UnityFixture.ListOnly;
    savedName = UnityFixture.NameFilter;
    savedGroup = UnityFixture.GroupFilter;
    savedList = UnityFixture.PlainListOfTests;
}

TEST_TEAR_DOWN(UnityCommandOptions)
{
    UnityFixture.Verbose = savedVerbose;
    UnityFixture.RepeatCount= savedRepeat;
    UnityFixture.ListOnly = savedListOnly;
    UnityFixture.NameFilter = savedName;
    UnityFixture.GroupFilter = savedGroup;
    UnityFixture.PlainListOfTests = savedList;
}


static const char* noOptions[] = {
        "testrunner.exe"
};

TEST(UnityCommandOptions, DefaultOptions)
{
    UnityGetCommandLineOptions(1, noOptions);
    TEST_ASSERT_EQUAL(0, UnityFixture.Verbose);
    TEST_ASSERT_POINTERS_EQUAL(0, UnityFixture.GroupFilter);
    TEST_ASSERT_POINTERS_EQUAL(0, UnityFixture.NameFilter);
    TEST_ASSERT_EQUAL(1, UnityFixture.RepeatCount);
}

static const char* verbose[] = {
        "testrunner.exe",
        "-v"
};

TEST(UnityCommandOptions, OptionVerbose)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(2, verbose));
    TEST_ASSERT_EQUAL(1, UnityFixture.Verbose);
}

static const char* group[] = {
        "testrunner.exe",
        "-g", "groupname"
};

TEST(UnityCommandOptions, OptionSelectTestByGroup)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(3, group));
    STRCMP_EQUAL("groupname", UnityFixture.GroupFilter);
}

static const char* name[] = {
        "testrunner.exe",
        "-n", "testname"
};

TEST(UnityCommandOptions, OptionSelectTestByName)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(3, name));
    STRCMP_EQUAL("testname", UnityFixture.NameFilter);
}

static const char* repeat[] = {
        "testrunner.exe",
        "-r", "99"
};

TEST(UnityCommandOptions, OptionSelectRepeatTestsDefaultCount)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(2, repeat));
    TEST_ASSERT_EQUAL(2, UnityFixture.RepeatCount);
}

TEST(UnityCommandOptions, OptionSelectRepeatTestsSpecificCount)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(3, repeat));
    TEST_ASSERT_EQUAL(99, UnityFixture.RepeatCount);
}

static const char* multiple[] = {
        "testrunner.exe",
        "-v",
        "-g", "groupname",
        "-n", "testname",
        "-r", "98"
};

TEST(UnityCommandOptions, MultipleOptions)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(8, multiple));
    TEST_ASSERT_EQUAL(1, UnityFixture.Verbose);
    STRCMP_EQUAL("groupname", UnityFixture.GroupFilter);
    STRCMP_EQUAL("testname", UnityFixture.NameFilter);
    TEST_ASSERT_EQUAL(98, UnityFixture.RepeatCount);
}

static const char* dashRNotLast[] = {
        "testrunner.exe",
        "-v",
        "-g", "gggg",
        "-r",
        "-n", "tttt",
};

TEST(UnityCommandOptions, MultipleOptionsDashRNotLastAndNoValueSpecified)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(7, dashRNotLast));
    TEST_ASSERT_EQUAL(1, UnityFixture.Verbose);
    STRCMP_EQUAL("gggg", UnityFixture.GroupFilter);
    STRCMP_EQUAL("tttt", UnityFixture.NameFilter);
    TEST_ASSERT_EQUAL(2, UnityFixture.RepeatCount);
}

static const char* unknownCommand[] = {
        "testrunner.exe",
        "-v",
        "-g", "groupname",
        "-n", "testname",
        "-r", "98",
        "-z"
};
TEST(UnityCommandOptions, UnknownCommandIsIgnored)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(9, unknownCommand));
    TEST_ASSERT_EQUAL(1, UnityFixture.Verbose);
    STRCMP_EQUAL("groupname", UnityFixture.GroupFilter);
    STRCMP_EQUAL("testname", UnityFixture.NameFilter);
    TEST_ASSERT_EQUAL(98, UnityFixture.RepeatCount);
}

static const char* listing[] = {
        "testrunner.exe",
        "-l",
};
TEST(UnityCommandOptions, Listing)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(2, listing));
    TEST_ASSERT_EQUAL(1, UnityFixture.ListOnly);
}

static const char* exactListVerbose[] = {
        "testrunner.exe",
        "-v",
        "--",
        "MyTest/TestOne",
        "MyTest/TestTwo",
        NULL
};
TEST(UnityCommandOptions, ExactList)
{
    TEST_ASSERT_EQUAL(0, UnityGetCommandLineOptions(5, exactListVerbose));
    TEST_ASSERT_EQUAL(1, UnityFixture.Verbose);
    TEST_ASSERT_EQUAL_STRING("MyTest/TestOne", UnityFixture.PlainListOfTests[0]);
    TEST_ASSERT_EQUAL_STRING("MyTest/TestTwo", UnityFixture.PlainListOfTests[1]);
    TEST_ASSERT_NULL(UnityFixture.PlainListOfTests[2]);
}

//------------------------------------------------------------

TEST_GROUP(LeakDetection);

TEST_SETUP(LeakDetection)
{
    UnityOutputCharSpy_Create(1000);
}

TEST_TEAR_DOWN(LeakDetection)
{
    UnityOutputCharSpy_Destroy();
}

TEST(LeakDetection, DetectsLeak)
{
    void* m = malloc(10);
    UnityOutputCharSpy_Enable(1);
    TEST_PROTECTED(UnityMalloc_EndTest);
    UnityOutputCharSpy_Enable(0);
    CHECK(strstr(UnityOutputCharSpy_Get(), "This test leaks!"));
    free(m);
    Unity.CurrentTestFailed = 0;
}

static void  BufferOverrunFoundDuringFree_worker(void)
{
    void* m = malloc(10);
    char* s = (char*)m;
    s[10] = (char)0xFF;
    free(m);
}

TEST(LeakDetection, BufferOverrunFoundDuringFree)
{
    UnityOutputCharSpy_Enable(1);
    TEST_PROTECTED(BufferOverrunFoundDuringFree_worker);
    UnityOutputCharSpy_Enable(0);
    CHECK(strstr(UnityOutputCharSpy_Get(), "Buffer overrun detected during free()"));
    Unity.CurrentTestFailed = 0;
}

static void BufferOverrunFoundDuringRealloc_worker(void)
{
    void* m = malloc(10);
    char* s = (char*)m;
    s[10] = (char)0xFF;
    m = realloc(m, 100);
}

TEST(LeakDetection, BufferOverrunFoundDuringRealloc)
{
    UnityOutputCharSpy_Enable(1);
    TEST_PROTECTED(BufferOverrunFoundDuringRealloc_worker);
    UnityOutputCharSpy_Enable(0);
    CHECK(strstr(UnityOutputCharSpy_Get(), "Buffer overrun detected during realloc()"));
    Unity.CurrentTestFailed = 0;
}

//------------------------------------------------------------

TEST_GROUP(Timeout);

TEST_SETUP(Timeout)
{
}

TEST_TEAR_DOWN(Timeout)
{
}

static void SleepSome(void)
{
    usleep(100000);
}

TEST(Timeout, Triggers)
{
    int result = TEST_PROTECTED_WITH_TIMEOUT(SleepSome, 10);
    TEST_ASSERT_TRUE(Unity.CurrentTestTimeout);
    TEST_ASSERT_NOT_EQUAL(0, result);
    Unity.CurrentTestTimeout = false;
}

TEST(Timeout, DoesNotTrigger)
{
    int result = TEST_PROTECTED_WITH_TIMEOUT(SleepSome, 200);
    UNITY_COUNTER_TYPE isTimeout = Unity.CurrentTestTimeout;
    Unity.CurrentTestTimeout = 0;
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FALSE(isTimeout);
}

//------------------------------------------------------------
// Manual checking
//------------------------------------------------------------

TEST_GROUP(TimeoutInSetup);

TEST_SETUP_WITH_TIMEOUT(TimeoutInSetup, 100)
{
    usleep(300000);
}

TEST_TEAR_DOWN_WITH_TIMEOUT(TimeoutInSetup, 100)
{
    usleep(2);
}

TEST_WITH_TIMEOUT(TimeoutInSetup, Test, 100)
{
    usleep(2);
}

TEST_GROUP(TimeoutInBody);

TEST_SETUP_WITH_TIMEOUT(TimeoutInBody, 100)
{
    usleep(2);
}

TEST_TEAR_DOWN_WITH_TIMEOUT(TimeoutInBody, 100)
{
    usleep(2);
}

TEST_WITH_TIMEOUT(TimeoutInBody, Test, 100)
{
    usleep(300000);
}

TEST_GROUP(TimeoutInTearDown);

TEST_SETUP_WITH_TIMEOUT(TimeoutInTearDown, 100)
{
    usleep(2);
}

TEST_TEAR_DOWN_WITH_TIMEOUT(TimeoutInTearDown, 100)
{
    usleep(300000);
}

TEST_WITH_TIMEOUT(TimeoutInTearDown, Test, 100)
{
    usleep(2);
}
