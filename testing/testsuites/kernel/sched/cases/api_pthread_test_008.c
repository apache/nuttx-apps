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

static sem_t schedTask08_sem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedPthread08Threadroutine
 ****************************************************************************/
static void *schedPthread08Threadroutine(void *arg)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        sem_wait(&schedTask08_sem);
        (*((int *)arg))++;
    }
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread08
 ****************************************************************************/

void TestNuttxSchedPthread08(FAR void **state)
{
    int res;
    pthread_t pthread_id;
    int run_flag = 0;

    res = sem_init(&schedTask08_sem, 0, 0);
    assert_int_equal(res, OK);

    /* create pthread */
    res = pthread_create(&pthread_id, NULL, (void *)schedPthread08Threadroutine, &run_flag);
    assert_int_equal(res, OK);

    while (1)
    {
        sleep(2);
        res = sem_post(&schedTask08_sem);
        assert_int_equal(res, OK);
        if (run_flag == 5)
            break;
    }
    res = sem_destroy(&schedTask08_sem);
    assert_int_equal(res, OK);
    assert_int_equal(run_flag, 5);
}