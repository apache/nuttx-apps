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
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SchedTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedTask03Routine
 ****************************************************************************/
static int schedTask03Routine(int argc, char *argv[])
{
    int i;
    for (i = 0; i < 100; ++i)
    {
        usleep(20000);
        /* Run some simulated tasks */
    }
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTask03
 ****************************************************************************/
void TestNuttxSchedTask03(FAR void **state)
{
    pid_t pid;
    int res;
    pid = task_create("schedTask03Routine", TASK_PRIORITY, DEFAULT_STACKSIZE, schedTask03Routine, NULL);
    assert_true(pid > 0);

    int i;
    for (i = 0; i < 5; ++i)
    {
        usleep(2000);
    }
    res = task_delete(pid);
    assert_int_equal(res, 0);
}