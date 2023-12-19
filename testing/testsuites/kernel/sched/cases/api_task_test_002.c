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
 * Name: schedTask02Routine
 ****************************************************************************/
static int schedTask02Routine(int argc, char *argv[])
{
    int i;
    for (i = 0; i < 10; i++)
    {
        sleep(1);
    }
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTask02
 ****************************************************************************/
void TestNuttxSchedTask02(FAR void **state)
{
    pid_t pid;
    int status, res;

    pid = task_create("schedTask02Routine", TASK_PRIORITY, DEFAULT_STACKSIZE, schedTask02Routine, NULL);
    assert_true(pid > 0);
    for (int i = 0; i < 2; i++)
    {
        sleep(2);
        res = task_restart(pid);
        assert_int_equal(res, OK);
    }

    waitpid(pid, &status, 0);
}