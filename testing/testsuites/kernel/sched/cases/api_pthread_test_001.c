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
 * Name: schedPthread01Threadroutine
 ****************************************************************************/
static void *schedPthread01Threadroutine(void *arg)
{
    int flag;
    flag = 1;
    *((int *)arg) = flag;
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: TestNuttxSchedPthread01
 ****************************************************************************/

void TestNuttxSchedPthread01(FAR void **state)
{
    pthread_t p_t;
    int run_flag = 0;

    /* create thread */
    pthread_create(&p_t, NULL, schedPthread01Threadroutine, &run_flag);

    /* pthread_join Wait for the thread to end*/
    pthread_join(p_t, NULL);
    assert_int_equal(run_flag, 1);
}