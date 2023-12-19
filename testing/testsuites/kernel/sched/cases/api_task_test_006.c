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
 * Name: schedTask06Routine
 ****************************************************************************/
static int schedTask06Routine(int argc, char *argv[])
{
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTask06
 ****************************************************************************/
void TestNuttxSchedTask06(FAR void **state)
{
    pid_t pid;
    int status;
    int priority_max;
    int priority_min;

    /* struct sched_param task_entry_param */
    pid = task_create("schedTask06Routine", TASK_PRIORITY, DEFAULT_STACKSIZE, schedTask06Routine, NULL);
    assert_true(pid > 0);
    priority_max = sched_get_priority_max(SCHED_FIFO);
    priority_min = sched_get_priority_min(SCHED_FIFO);
    assert_int_equal(priority_max, SCHED_PRIORITY_MAX);
    assert_int_equal(priority_min, SCHED_PRIORITY_MIN);
    waitpid(pid, &status, 0);
}