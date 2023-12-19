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
 * Name: schedTask04Routine
 ****************************************************************************/
static int schedTask04Routine(int argc, char *argv[])
{
    for (int i = 0; i < 5; i++)
    {
        /* Run some simulated tasks */
        sleep(1);
    }
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedTask04
 ****************************************************************************/
void TestNuttxSchedTask04(FAR void **state)
{
    pid_t pid;
    int status, ret;

    struct sched_param task_entry_param;
    /* create a test task */
    pid = task_create("schedTask04Routine", TASK_PRIORITY, DEFAULT_STACKSIZE, schedTask04Routine, NULL);
    assert_true(pid > 0);
    ret = sched_getparam(pid, &task_entry_param);
    assert_int_equal(ret, 0);
#if CONFIG_RR_INTERVAL > 0
    ret = sched_setscheduler(pid, SCHED_RR, &task_entry_param);
    assert_int_equal(ret, OK);
    if (ret != OK)
    {
        syslog(LOG_ERR, "RR scheduling is not supported !\n");
    }

    switch (sched_getscheduler(pid))
    {
    case SCHED_FIFO:
        syslog(LOG_INFO, "[%s]:scheduling policy is FIFO!\n", __func__);
        break;
    case SCHED_RR:
        syslog(LOG_INFO, "[%s]:scheduling policy is RR!\n", __func__);
        break;
    case SCHED_OTHER:
        syslog(LOG_INFO, "[%s]:scheduling policy is OTHER!\n", __func__);
        break;
    default:
        break;
    }
#endif
    task_entry_param.sched_priority = 100;
    ret = sched_setparam(pid, &task_entry_param);
    assert_int_equal(ret, OK);
    waitpid(pid, &status, 0);
}