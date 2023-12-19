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
 * Name: schedPthread03Threadroutine
 ****************************************************************************/
static void *schedPthread03Threadroutine(void *arg)
{
    int i = 0;
    for (i = 0; i <= 5; i++)
    {
        if (i == 3)
        {
            pthread_exit(0);
        }
    }
    /* This part of the code will not be executed */
    *((int *)arg) = 1;
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread03
 ****************************************************************************/
void TestNuttxSchedPthread03(FAR void **state)
{
    pthread_t pid_1;
    int ret;
    int run_flag = 0;

    /* create test thread */
    ret = pthread_create(&pid_1, NULL, (void *)schedPthread03Threadroutine, &run_flag);
    assert_int_equal(ret, 0);

    pthread_join(pid_1, NULL);

    assert_int_not_equal(run_flag, 1);
}