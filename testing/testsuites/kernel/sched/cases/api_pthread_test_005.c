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
 * Name: schedPthread05Threadroutine
 ****************************************************************************/
static void *schedPthread05Threadroutine(void *arg)
{
    int i;
    for (i = 0; i <= 5; i++)
    {
        if (i == 2)
        {
            pthread_yield();
        }
    }
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread05
 ****************************************************************************/
void TestNuttxSchedPthread05(FAR void **state)
{
    int res;
    pthread_t p_t;

    /* create thread */
    res = pthread_create(&p_t, NULL, schedPthread05Threadroutine, NULL);

    assert_int_equal(res, 0);

    pthread_join(p_t, NULL);
}