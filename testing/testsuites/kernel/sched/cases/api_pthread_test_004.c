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
 * Name: schedPthread04Threadroutine
 ****************************************************************************/
static void *schedPthread04Threadroutine(void *arg)
{
    /* set enable */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    sleep(1);

    /* cancel point */
    pthread_testcancel();

    /* It can not be executed here */
    *((int *)arg) = 1;
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread04
 ****************************************************************************/

void TestNuttxSchedPthread04(FAR void **state)
{
    int res;
    pthread_t p_t_1;

    /* int flag */
    int schedPthreadTest04_run_flag = 0;

    /* create thread_1 */
    res = pthread_create(&p_t_1, NULL, schedPthread04Threadroutine, &schedPthreadTest04_run_flag);
    assert_int_equal(res, OK);

    res = pthread_cancel(p_t_1);
    assert_int_equal(res, OK);

    /* join thread_1 */
    pthread_join(p_t_1, NULL);
    assert_int_equal(schedPthreadTest04_run_flag, 0);
}