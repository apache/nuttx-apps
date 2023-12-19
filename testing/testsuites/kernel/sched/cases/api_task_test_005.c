/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdio.h>
#include <syslog.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SchedTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedTask05Routine
 ****************************************************************************/
static int schedTask05Routine(int argc, char *argv[])
{
    int ret;
    int i;
    for (i = 1; i <= 10; i++)
    {
        if (i >= 4 && i <= 7)
        {
            ret = sched_yield();
            assert_int_equal(ret, OK);
        }
    }
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTask05
 ****************************************************************************/
void TestNuttxSchedTask05(FAR void **state)
{
    pid_t pid;
    int status;

    pid = task_create("schedTask05Routine", TASK_PRIORITY, DEFAULT_STACKSIZE, schedTask05Routine, NULL);
    assert_true(pid > 0);
    waitpid(pid, &status, 0);
}