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
 * Name: schedPthread02Threadroutine
 ****************************************************************************/
static void *schedPthread02Threadroutine(void *arg)
{
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread02
 ****************************************************************************/
void TestNuttxSchedPthread02(FAR void **state)
{
    int res;
    pthread_t p_t;
    pthread_attr_t attr;
    size_t statck_size;
    struct sched_param param, o_param;

    /* Initializes thread attributes object (attr) */
    res = pthread_attr_init(&attr);
    assert_int_equal(res, OK);

    res = pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);
    assert_int_equal(res, OK);

    res = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    assert_int_equal(res, OK);

    param.sched_priority = TASK_PRIORITY + 1;

    res = pthread_attr_setschedparam(&attr, &param);
    assert_int_equal(res, OK);

    /* create thread */
    pthread_create(&p_t, &attr, schedPthread02Threadroutine, NULL);

    /* Wait for the child thread finish */
    pthread_join(p_t, NULL);

    /* get schedparam */
    res = pthread_attr_getschedparam(&attr, &o_param);
    assert_int_equal(res, OK);

    /* get stack size */
    res = pthread_attr_getstacksize(&attr, &statck_size);
    assert_int_equal(res, OK);
}