/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdlib.h>
#include <syslog.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <malloc.h>
#include "MmTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define MEMORY_LIST_LENGTH 200
/* Random size range, we will apply the memory size in this range */
#define MALLOC_MIN_SIZE 32
#define MALLOC_MAX_SIZE 2048

static int TestNuttx08_routine_1(int argc, char *argv[])
{
    char *ptr = NULL, *tmp_ptr = NULL;
    int malloc_size, flag = 0;

    for (int n = 0; n < 1000; n++)
    {
        malloc_size = mmtest_get_rand_size(64, 512);
        tmp_ptr = ptr = malloc(sizeof(char) * malloc_size);
        if (ptr == NULL)
        {
            flag = 1;
            break;
        }

        for (int i = 0; i < malloc_size; i++)
            *tmp_ptr++ = 'X';
        tmp_ptr = ptr;
        for (int j = 0; j < malloc_size; j++)
        {
            if (*tmp_ptr++ != 'X')
            {
                flag = 1;
            }
        }
        free(ptr);
    }
    assert_int_equal(flag, 0);
    return 0;
}

static int TestNuttx08_routine_2(int argc, char *argv[])
{
    char *temp_ptr = NULL;
    int flag = 0;

    for (int n = 0; n < 1000; n++)
    {
        temp_ptr = memalign(64, 1024 * sizeof(char));
        if (temp_ptr == NULL)
        {
            flag = 1;
            break;
        }
        assert_non_null(temp_ptr);

        memset(temp_ptr, 0x33, 1024 * sizeof(char));
        free(temp_ptr);
    }
    assert_int_equal(flag, 0);
    return 0;
}

static int TestNuttx08_routine_3(int argc, char *argv[])
{
    char *pm;
    unsigned long memsize;
    for (int i = 0; i < 500; i++)
    {
        /* Apply for as much memory as a system allows */
        memsize = mmtest_get_rand_size(1024, 2048);
        pm = malloc(memsize);
        assert_non_null(pm);
        free(pm);
    }
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: TestNuttxMm08
 ****************************************************************************/
void TestNuttxMm08(FAR void **state)
{
    pid_t pid;
    int status;

    pid = task_create("TestNuttx08_routine_1", TASK_PRIORITY, DEFAULT_STACKSIZE, TestNuttx08_routine_1, NULL);
    assert_true(pid > 0);
    pid = task_create("TestNuttx08_routine_2", TASK_PRIORITY, DEFAULT_STACKSIZE, TestNuttx08_routine_2, NULL);
    assert_true(pid > 0);
    pid = task_create("TestNuttx08_routine_3", TASK_PRIORITY, DEFAULT_STACKSIZE, TestNuttx08_routine_3, NULL);
    assert_true(pid > 0);
    waitpid(pid, &status, 0);
}