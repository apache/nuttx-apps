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
 * Private Data
 ****************************************************************************/
static sem_t schedTask09_sem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedPthread09Threadroutine
 ****************************************************************************/
static void *schedPthread09Threadroutine(void *arg)
{
    int i, res;
    for (i = 0; i < 10; i++)
    {
        res = sem_wait(&schedTask09_sem);
        assert_int_equal(res, OK);
        (*((int *)arg))++;
        res = sem_post(&schedTask09_sem);
        assert_int_equal(res, OK);
    }
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread09
 ****************************************************************************/
void TestNuttxSchedPthread09(FAR void **state)
{
    int res;
    pthread_t pthread_id[10];
    int run_flag = 0;

    res = sem_init(&schedTask09_sem, 0, 1);
    assert_int_equal(res, OK);

    int i;
    for (i = 0; i < 10; i++)
    {
        res = pthread_create(&pthread_id[i], NULL, (void *)schedPthread09Threadroutine, &run_flag);
        assert_int_equal(res, OK);
    }

    int j;
    for (j = 0; j < 10; j++)
        pthread_join(pthread_id[j], NULL);

    res = sem_destroy(&schedTask09_sem);
    assert_int_equal(res, OK);

    assert_int_equal(run_flag, 100);
}