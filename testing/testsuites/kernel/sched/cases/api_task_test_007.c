/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdio.h>
#include <syslog.h>
#include <sched.h>
#include <sys/types.h>
#include <nuttx/sched.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SchedTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedTask07Routine
 ****************************************************************************/
static int schedTask07Routine(int argc, char *argv[])
{
    /* lock */
    sched_lock();
    int i;
    for (i = 0; i < 10; i++)
    {
        usleep(2000);
        /* Run some simulated tasks */
    }
    if (sched_lockcount() != 1)
    {
        fail_msg("test fail !");
        return 0;
    }
    /* unlock */
    sched_unlock();
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTask07
 ****************************************************************************/
void TestNuttxSchedTask07(FAR void **state)
{
    pid_t pid;
    int status;

    pid = task_create("schedTask07Routine", TASK_PRIORITY, DEFAULT_STACKSIZE, schedTask07Routine, NULL);
    assert_true(pid > 0);

    waitpid(pid, &status, 0);
}