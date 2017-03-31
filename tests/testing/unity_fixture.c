//- Copyright (c) 2010 James Grenning and Contributed to Unity Project
/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <testing/unity_fixture.h>
#include <testing/unity_internals.h>

UNITY_FIXTURE_T UnityFixture;

static char const* emptyList[] = {NULL};

int verbose = 0;

void setUp(void);
void tearDown(void);
void setUp(void)    { /*does nothing*/ }
void tearDown(void) { /*does nothing*/ }

static void announceTestRun(unsigned int runNumber)
{
    UnityPrint("Unity test run ");
    UnityPrintNumber(runNumber+1);
    UnityPrint(" of ");
    UnityPrintNumber(UnityFixture.RepeatCount);
    UNITY_OUTPUT_CHAR('\n');
}

int UnityMain(int argc, const char* argv[], void (*runAllTests)(void))
{
    int result = UnityGetCommandLineOptions(argc, argv);
    unsigned int r;
    if (result != 0)
        return result;

    for (r = 0; r < UnityFixture.RepeatCount; r++)
    {
        UnityBegin(argv[0]);
        announceTestRun(r);
        runAllTests();
        UNITY_OUTPUT_CHAR('\n');
        UnityEnd();
    }

    return UnityFailureCount();
}

static int selected(const char * filter, const char * name)
{
    if (filter == 0)
        return 1;
    return strstr(name, filter) ? 1 : 0;
}

static int testSelected(const char* test)
{
    return selected(UnityFixture.NameFilter, test);
}

static int groupSelected(const char* group)
{
    return selected(UnityFixture.GroupFilter, group);
}

static int plainSelected(const char* group, const char* test)
{
    if (UnityFixture.PlainListOfTests[0] == NULL)
    {
        return 1;
    }
    else
    {
        const char** plain = UnityFixture.PlainListOfTests;
        while (plain[0] != NULL)
        {
            const char* selector = plain[0];
            int groupLen = strlen(group);
            int testLen = strlen(test);
            if (groupLen + testLen + 1 == strlen(selector) &&
                    selector[groupLen] == '/' &&
                    strstr(selector, group) == selector &&
                    strstr(selector + groupLen + 1, test) == selector + groupLen + 1)
            {
                return 1;
            }
            plain++;
        }
        return 0;
    }
}

static void runTestCase(void)
{

}

void UnityTestRunner(unityfunction* setup, _U_UINT setupTimeoutMsec,
                     unityfunction* testBody, _U_UINT bodyTimeoutMsec,
                     unityfunction* teardown, _U_UINT teardownTimeoutMsec,
                     const char * printableName,
                     const char * group,
                     const char * name,
                     const char * file, int line)
{
    if (testSelected(name) && groupSelected(group) && plainSelected(group, name))
    {
        Unity.CurrentTestFailed = 0;
        Unity.TestFile = file;
        Unity.CurrentTestName = printableName;
        Unity.CurrentTestLineNumber = line;
        if (UnityFixture.ListOnly)
        {
            UnityPrint(group);
            UNITY_OUTPUT_CHAR('/');
            UnityPrint(name);
            UNITY_OUTPUT_CHAR('\n');
            Unity.NumberOfTests++;
        }
        else
        {
            if (!UnityFixture.Verbose)
                UNITY_OUTPUT_CHAR('.');
            else
            {
                UnityPrint(printableName);
                UNITY_OUTPUT_CHAR(' ');
            }

            Unity.NumberOfTests++;
            UnityMalloc_StartTest();
            UnityPointer_Init();

            runTestCase();
            if (TEST_PROTECTED_WITH_TIMEOUT(setup, setupTimeoutMsec) == 0)
            {
                TEST_PROTECTED_WITH_TIMEOUT(testBody, bodyTimeoutMsec);
            }
            TEST_PROTECTED_WITH_TIMEOUT(teardown, teardownTimeoutMsec);

            UnityPointer_UndoAllSets();
            if (!Unity.CurrentTestFailed && !Unity.CurrentTestTimeout)
                TEST_PROTECTED(UnityMalloc_EndTest);

            if (Unity.CurrentTestFailed && UnityFixture.Verbose)
            {
                UNITY_OUTPUT_CHAR('\n');
            }
        }
        UnityConcludeFixtureTest();
    }
}

void UnityIgnoreTest(const char * printableName)
{
    Unity.NumberOfTests++;
    Unity.CurrentTestIgnored = 1;
    if (!UnityFixture.Verbose)
        UNITY_OUTPUT_CHAR('!');
    else
        UnityPrint(printableName);
    UnityConcludeFixtureTest();
}


//-------------------------------------------------
//Malloc and free stuff
//
#define MALLOC_DONT_FAIL -1
static int malloc_count;
static int malloc_fail_countdown = MALLOC_DONT_FAIL;

void UnityMalloc_StartTest(void)
{
    malloc_count = 0;
    malloc_fail_countdown = MALLOC_DONT_FAIL;
}

void UnityMalloc_EndTest(void)
{
    malloc_fail_countdown = MALLOC_DONT_FAIL;
    if (malloc_count != 0)
    {
        TEST_FAIL_MESSAGE("This test leaks!");
    }
}

void UnityMalloc_MakeMallocFailAfterCount(int countdown)
{
    malloc_fail_countdown = countdown;
}

#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

#ifdef calloc
#undef calloc
#endif

#ifdef realloc
#undef realloc
#endif

#include <stdlib.h>
#include <string.h>

typedef struct GuardBytes
{
    size_t size;
    char guard[sizeof(size_t)];
} Guard;


static const char * end = "END";

void * unity_malloc(size_t size)
{
    char* mem;
    Guard* guard;

    if (malloc_fail_countdown != MALLOC_DONT_FAIL)
    {
        if (malloc_fail_countdown == 0)
            return 0;
        malloc_fail_countdown--;
    }

    malloc_count++;

    guard = (Guard*)malloc(size + sizeof(Guard) + 4);
    guard->size = size;
    mem = (char*)&(guard[1]);
    memcpy(&mem[size], end, strlen(end) + 1);

    return (void*)mem;
}

static int isOverrun(void * mem)
{
    Guard* guard = (Guard*)mem;
    char* memAsChar = (char*)mem;
    guard--;

    return strcmp(&memAsChar[guard->size], end) != 0;
}

static void release_memory(void * mem)
{
    Guard* guard = (Guard*)mem;
    guard--;

    malloc_count--;
    free(guard);
}

void unity_free(void * mem)
{
    if (!mem)
    {
        /* Freeing NULL should be always OK */
        return;
    }
    int overrun = isOverrun(mem);//strcmp(&memAsChar[guard->size], end) != 0;
    release_memory(mem);
    if (overrun)
    {
        TEST_FAIL_MESSAGE("Buffer overrun detected during free()");
    }
}

void* unity_calloc(size_t num, size_t size)
{
    void* mem = unity_malloc(num * size);
    memset(mem, 0, num*size);
    return mem;
}

void* unity_realloc(void * oldMem, size_t size)
{
    Guard* guard = (Guard*)oldMem;
//    char* memAsChar = (char*)oldMem;
    void* newMem;

    if (oldMem == 0)
        return unity_malloc(size);

    guard--;
    if (isOverrun(oldMem))
    {
        release_memory(oldMem);
        TEST_FAIL_MESSAGE("Buffer overrun detected during realloc()");
        return 0; /* Above will not abort the test, if the execution is skipped */
    }

    if (size == 0)
    {
        release_memory(oldMem);
        return 0;
    }

    if (guard->size >= size)
        return oldMem;

    newMem = unity_malloc(size);
    memcpy(newMem, oldMem, guard->size);
    unity_free(oldMem);
    return newMem;
}


//--------------------------------------------------------
//Automatic pointer restoration functions
typedef struct _PointerPair
{
    struct _PointerPair * next;
    void ** pointer;
    void * old_value;
} PointerPair;

enum {MAX_POINTERS=50};
static PointerPair pointer_store[MAX_POINTERS];
static int pointer_index = 0;

void UnityPointer_Init(void)
{
    pointer_index = 0;
}

void UnityPointer_Set(void ** pointer, void * newValue)
{
    if (pointer_index >= MAX_POINTERS)
        TEST_FAIL_MESSAGE("Too many pointers set");

    pointer_store[pointer_index].pointer = pointer;
    pointer_store[pointer_index].old_value = *pointer;
    *pointer = newValue;
    pointer_index++;
}

void UnityPointer_UndoAllSets(void)
{
    while (pointer_index > 0)
    {
        pointer_index--;
        *(pointer_store[pointer_index].pointer) =
            pointer_store[pointer_index].old_value;

    }
}

int UnityFailureCount(void)
{
    return Unity.TestFailures;
}

int UnityGetCommandLineOptions(int argc, const char* argv[])
{
    int i;
    UnityFixture.Verbose = 0;
    UnityFixture.ListOnly = 0;
    UnityFixture.GroupFilter = 0;
    UnityFixture.NameFilter = 0;
    UnityFixture.RepeatCount = 1;
    UnityFixture.PlainListOfTests = emptyList;

    if (argc == 1)
        return 0;

    for (i = 1; i < argc; )
    {
        if (strcmp(argv[i], "-l") == 0)
        {
            UnityFixture.ListOnly = 1;
            i++;
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            UnityFixture.Verbose = 1;
            i++;
        }
        else if (strcmp(argv[i], "-g") == 0)
        {
            i++;
            if (i >= argc)
                return 1;
            UnityFixture.GroupFilter = argv[i];
            i++;
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            i++;
            if (i >= argc)
                return 1;
            UnityFixture.NameFilter = argv[i];
            i++;
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            UnityFixture.RepeatCount = 2;
            i++;
            if (i < argc)
            {
                if (*(argv[i]) >= '0' && *(argv[i]) <= '9')
                {
                    UnityFixture.RepeatCount = atoi(argv[i]);
                    i++;
                }
            }
        }
        else if (strcmp(argv[i], "--") == 0)
        {
            UnityFixture.PlainListOfTests = argv + i + 1;
            break;
        }
        else {
            // ignore unknown parameter
            i++;
        }
    }
    return 0;
}

void UnityConcludeFixtureTest(void)
{
    if (Unity.CurrentTestIgnored)
    {
        if (UnityFixture.Verbose)
        {
            UNITY_OUTPUT_CHAR('\n');
        }
        Unity.TestIgnores++;
    }
    else if (Unity.CurrentTestTimeout)
    {
	if (UnityFixture.Verbose)
	{
	    UnityPrint("  TIMEOUT");
	    UNITY_OUTPUT_CHAR('\n');
	}
	Unity.TestFailures++;
    }
    else if (!Unity.CurrentTestFailed)
    {
        if (UnityFixture.Verbose)
        {
            UnityPrint(" PASS");
            UNITY_OUTPUT_CHAR('\n');
        }
    }
    else if (Unity.CurrentTestFailed)
    {
        Unity.TestFailures++;
    }

    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}
