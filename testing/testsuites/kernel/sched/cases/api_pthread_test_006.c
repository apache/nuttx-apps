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
#include <pthread.h>
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
 * Name: schedPthread06Threadroutine
 ****************************************************************************/
static void *schedPthread06Threadroutine(void *arg)
{
    int pid;
    /* call pthread_self() */
    pid = pthread_self();
    assert_true(pid > 0);
    if (pid > 0)
    {
        *((int *)arg) = 0;
    }
    else
    {
        *((int *)arg) = 1;
    }
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread06
 ****************************************************************************/

void TestNuttxSchedPthread06(FAR void **state)
{
    pthread_t p_t;
    int run_flag = 0;
    int res;

    /* create thread */
    res = pthread_create(&p_t, NULL, schedPthread06Threadroutine, &run_flag);
    assert_int_equal(res, OK);

    pthread_join(p_t, NULL);

    assert_int_equal(run_flag, 0);
}