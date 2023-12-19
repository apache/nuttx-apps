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
 * Name: schedPthread07Threadroutine
 ****************************************************************************/
static void *schedPthread07Threadroutine(void *arg)
{
    int i;
    pthread_mutex_t schedPthreadTest07_mutex = PTHREAD_MUTEX_INITIALIZER;
    for (i = 0; i < 100; i++)
    {
        pthread_mutex_lock(&schedPthreadTest07_mutex);
        (*((int *)arg))++;
        pthread_mutex_unlock(&schedPthreadTest07_mutex);
    }

    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread07
 ****************************************************************************/

void TestNuttxSchedPthread07(FAR void **state)
{
    int res;
    pthread_t pt_1, pt_2, pt_3;
    int run_flag = 0;

    res = pthread_create(&pt_1, NULL, (void *)schedPthread07Threadroutine, &run_flag);
    assert_int_equal(res, OK);
    res = pthread_create(&pt_2, NULL, (void *)schedPthread07Threadroutine, &run_flag);
    assert_int_equal(res, OK);
    res = pthread_create(&pt_3, NULL, (void *)schedPthread07Threadroutine, &run_flag);
    assert_int_equal(res, OK);

    pthread_join(pt_1, NULL);
    pthread_join(pt_2, NULL);
    pthread_join(pt_3, NULL);
    sleep(5);

    assert_int_equal(run_flag, 300);
}